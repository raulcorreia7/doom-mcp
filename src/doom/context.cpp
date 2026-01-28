#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "dmcp/doom/api.h"
#include "internal.hpp"
#include "mcp/generic/constants.h"
#include "mcp/json/json.hpp"

namespace dmcp {

using json_builder  = ::mcp::json::Builder;
using json_document = ::mcp::json::Document;
using json_value    = ::mcp::json::Value;

// ============================================================================
// Method Handlers
// ============================================================================

static bool handle_tools_list(void* user_data, const char* /*method*/,
                              const char* /*request_json*/,
                              char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);

  json_builder tools;
  tools.start_array();

  // get_game_state tool
  {
    json_builder tool;
    tool.start_object();
    tool.add("name", "get_game_state");
    tool.add(
	"description",
	"Get current game state including player position, health, enemies");

    json_builder schema;
    schema.start_object();
    schema.add("type", "object");

    json_builder props;
    props.start_object();
    schema.add("properties", std::move(props));

    tool.add("inputSchema", std::move(schema));
    tools.push(std::move(tool));
  }

  // get_screenshot tool
  if (ctx->screenshot.enabled) {
    json_builder tool;
    tool.start_object();
    tool.add("name", "get_screenshot");
    tool.add("description", "Capture a screenshot of the current game state");

    json_builder schema;
    schema.start_object();
    schema.add("type", "object");

    json_builder props;
    props.start_object();
    schema.add("properties", std::move(props));

    tool.add("inputSchema", std::move(schema));
    tools.push(std::move(tool));
  }

  // execute_command tool (for bidirectional control)
  {
    json_builder tool;
    tool.start_object();
    tool.add("name", "execute_command");
    tool.add("description",
             "Execute a game command (spawn enemy, change level, etc.)");

    json_builder schema;
    schema.start_object();
    schema.add("type", "object");

    json_builder props;
    props.start_object();

    json_builder type_prop;
    type_prop.start_object();
    type_prop.add("type", "string");
    type_prop.add("description",
                  "Command type: spawn_entity, change_level, give_item, etc.");
    props.add("type", std::move(type_prop));

    json_builder params_prop;
    params_prop.start_object();
    params_prop.add("type", "object");
    params_prop.add("description", "Command-specific parameters");
    props.add("params", std::move(params_prop));

    schema.add("properties", std::move(props));

    json_builder required;
    required.start_array();
    required.push("type");
    schema.add("required", std::move(required));

    tool.add("inputSchema", std::move(schema));
    tools.push(std::move(tool));
  }

  json_builder result;
  result.start_object();
  result.add("tools", std::move(tools));

  const std::string resp = result.finish();
  if (resp.size() >= response_size) return false;
  std::strcpy(response_buffer, resp.c_str());
  return true;
}

static bool handle_tools_call(void*       user_data, const char* /*method*/,
                              const char* request_json, char* response_buffer,
                              size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);

  // Parse params to get tool name
  json_document doc;
  if (!doc.parse(request_json)) return false;

  json_value        params = doc.root();
  const std::string tool_name{params["name"].get_string()};

  auto build_response = [](const char* text) {
    json_builder result;
    result.start_object();

    json_builder content;
    content.start_array();

    json_builder item;
    item.start_object();
    item.add("type", "text");
    item.add("text", text);
    content.push(std::move(item));

    result.add("content", std::move(content));
    return result.finish();
  };

  auto build_error = [](const char* text) {
    json_builder result;
    result.start_object();
    result.add("isError", true);

    json_builder content;
    content.start_array();

    json_builder item;
    item.start_object();
    item.add("type", "text");
    item.add("text", text);
    content.push(std::move(item));

    result.add("content", std::move(content));
    return result.finish();
  };

  if (tool_name == "get_game_state") {
    const std::string resp =
	build_response("Use SSE stream for real-time game state");
    if (resp.size() >= response_size) return false;
    std::strcpy(response_buffer, resp.c_str());
    return true;
  }

  if (tool_name == "get_screenshot") {
    if (!ctx->screenshot.enabled) return false;

    ctx->screenshot.pending_requests.fetch_add(1);

    const std::string resp = build_response("Screenshot capture queued");
    if (resp.size() >= response_size) return false;
    std::strcpy(response_buffer, resp.c_str());
    return true;
  }

  if (tool_name == "execute_command") {
    json_value command_params = params["params"];

    // Serialize command_params to JSON string for parsing
    // TODO: Add to_json method to JsonValue
    dmcp_command_t cmd{};
    // For now, assume empty params

    dmcp_result_t parse_result = dmcp_parse_command_json("{}", &cmd);

    if (parse_result.code == DMCP_RESULT_CODE_OK) {
      dmcp_push_command(reinterpret_cast<dmcp_context_t*>(ctx), &cmd);

      const std::string resp = build_response("Command queued");
      if (resp.size() >= response_size) return false;
      std::strcpy(response_buffer, resp.c_str());
      return true;
    } else {
      const std::string resp = build_error("Invalid command");
      if (resp.size() >= response_size) return false;
      std::strcpy(response_buffer, resp.c_str());
      return true;
    }
  }

  return false;
}

// ============================================================================
// Snapshot Pool
// ============================================================================

dmcp_snapshot_t* acquire_snapshot(context* ctx) {
  std::lock_guard<std::mutex> lock{ctx->pool_mutex};

  for (auto& entry : ctx->pool) {
    if (!entry.in_use) {
      entry.in_use = true;
      dmcp_snapshot_clear(&entry.data);
      return &entry.data;
    }
  }

  return nullptr;  // Pool exhausted
}

void release_snapshot(context* ctx, dmcp_snapshot_t* snapshot) {
  if (!snapshot) return;

  std::lock_guard<std::mutex> lock{ctx->pool_mutex};

  for (auto& entry : ctx->pool) {
    if (&entry.data == snapshot) {
      entry.in_use = false;
      dmcp_snapshot_clear(&entry.data);
      return;
    }
  }
}

// ============================================================================
// JSON Serialization
// ============================================================================

static void AppendString(char*& buf, size_t& remaining, const char* str) {
  size_t len = std::strlen(str);
  if (len >= remaining) len = remaining - 1;
  std::memcpy(buf, str, len);
  buf += len;
  remaining -= len;
  *buf = '\0';
}

static void AppendFloat(char*& buf, size_t& remaining, float val) {
  char temp[32];
  std::snprintf(temp, sizeof(temp), "%.2f", val);
  AppendString(buf, remaining, temp);
}

static void AppendInt(char*& buf, size_t& remaining, int val) {
  char temp[32];
  std::snprintf(temp, sizeof(temp), "%d", val);
  AppendString(buf, remaining, temp);
}

std::string snapshot_to_json(const dmcp_snapshot_t& snapshot) {
  char   buffer[MCP_MAX_JSON_SIZE];
  char*  p         = buffer;
  size_t remaining = sizeof(buffer);

  AppendString(p, remaining, "{");

  // Player
  AppendString(p, remaining, "\"player\":{");
  AppendString(p, remaining, "\"hp\":");
  AppendFloat(p, remaining, snapshot.player.hp);
  AppendString(p, remaining, ",\"armor\":");
  AppendFloat(p, remaining, snapshot.player.armor);
  AppendString(p, remaining, ",\"ammo\":");
  AppendInt(p, remaining, snapshot.player.ammo);
  AppendString(p, remaining, ",\"position\":{");
  AppendString(p, remaining, "\"x\":");
  AppendFloat(p, remaining, snapshot.player.position.x);
  AppendString(p, remaining, ",\"y\":");
  AppendFloat(p, remaining, snapshot.player.position.y);
  AppendString(p, remaining, "}}");

  // Inventory
  AppendString(p, remaining, ",\"inventory\":[");
  for (std::uint32_t i = 0; i < snapshot.inventory_count; i++) {
    if (i > 0) AppendString(p, remaining, ",");
    AppendString(p, remaining, "{\"name\":\"");
    // Escape quotes in name
    for (const char* c = snapshot.inventory[i].name; *c; c++) {
      if (*c == '"' || *c == '\\') {
	if (remaining > 1) {
	  *p++ = '\\';
	  remaining--;
	}
      }
      if (remaining > 1) {
	*p++ = *c;
	remaining--;
      }
    }
    AppendString(p, remaining, "\",\"amount\":");
    AppendInt(p, remaining, snapshot.inventory[i].amount);
    AppendString(p, remaining, "}");
  }
  AppendString(p, remaining, "]");

  // Level
  AppendString(p, remaining, ",\"level\":{");
  AppendString(p, remaining, "\"tic\":");
  AppendInt(p, remaining, snapshot.level.tic);
  AppendString(p, remaining, ",\"id\":\"");
  AppendString(p, remaining, snapshot.level.level_id);
  AppendString(p, remaining, "\",\"name\":\"");
  AppendString(p, remaining, snapshot.level.level_name);
  AppendString(p, remaining, "\",\"kill_count\":");
  AppendInt(p, remaining, snapshot.level.kill_count);
  AppendString(p, remaining, ",\"item_count\":");
  AppendInt(p, remaining, snapshot.level.item_count);
  AppendString(p, remaining, ",\"secret_count\":");
  AppendInt(p, remaining, snapshot.level.secret_count);
  AppendString(p, remaining, "}");

  // Enemies
  AppendString(p, remaining, ",\"enemies\":[");
  for (std::uint32_t i = 0; i < snapshot.enemy_count; i++) {
    if (i > 0) AppendString(p, remaining, ",");
    AppendString(p, remaining, "{\"id\":");
    AppendInt(p, remaining, snapshot.enemies[i].id);
    AppendString(p, remaining, ",\"hp\":");
    AppendFloat(p, remaining, snapshot.enemies[i].hp);
    AppendString(p, remaining, ",\"max_hp\":");
    AppendFloat(p, remaining, snapshot.enemies[i].max_hp);
    AppendString(p, remaining, ",\"position\":{");
    AppendString(p, remaining, "\"x\":");
    AppendFloat(p, remaining, snapshot.enemies[i].position.x);
    AppendString(p, remaining, ",\"y\":");
    AppendFloat(p, remaining, snapshot.enemies[i].position.y);
    AppendString(p, remaining, "},\"type\":\"");
    // Escape type string
    for (const char* c = snapshot.enemies[i].type; *c; c++) {
      if (*c == '"' || *c == '\\') {
	if (remaining > 1) {
	  *p++ = '\\';
	  remaining--;
	}
      }
      if (remaining > 1) {
	*p++ = *c;
	remaining--;
      }
    }
    AppendString(p, remaining, "\"}");
  }
  AppendString(p, remaining, "]");

  AppendString(p, remaining, "}");

  return std::string{buffer, static_cast<size_t>(p - buffer)};
}

}  // namespace dmcp

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

dmcp_context_t* dmcp_create(const dmcp_config_t* config) {
  auto ctx = std::make_unique<dmcp::context>();

  ctx->config = config ? *config : dmcp_default_config();

  // Initialize pool
  ctx->pool.resize(ctx->config.snapshot_pool_size);
  for (auto& entry : ctx->pool) {
    entry.in_use = false;
    dmcp_snapshot_clear(&entry.data);
  }

  // Setup rate limiting
  ctx->min_interval =
      std::chrono::nanoseconds{1'000'000'000ull / ctx->config.target_hz};
  ctx->last_snapshot_time =
      std::chrono::steady_clock::now() - ctx->min_interval;

  // Setup screenshot
  ctx->screenshot.enabled.store(ctx->config.screenshot.enable);

  // Create command queue
  ctx->cmd_queue = std::make_unique<dmcp::command_queue>();

  // Create underlying MCP server
  mcp_server_config_t server_config = mcp_default_config();
  server_config.port                = ctx->config.port;
  server_config.on_log              = ctx->config.on_log;
  server_config.log_user_data       = ctx->config.user_data;

  ctx->server = mcp_server_create(&server_config);
  if (!ctx->server) {
    return nullptr;
  }

  // Register method handlers
  mcp_server_register_method(ctx->server, "tools/list", dmcp::handle_tools_list,
                             ctx.get());
  mcp_server_register_method(ctx->server, "tools/call", dmcp::handle_tools_call,
                             ctx.get());

  return reinterpret_cast<dmcp_context_t*>(ctx.release());
}

void dmcp_destroy(dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (ctx->server) {
    mcp_server_destroy(ctx->server);
  }

  delete ctx;
}

bool dmcp_is_running(const dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return false;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);
  return mcp_server_is_running(ctx->server);
}

void dmcp_tick(dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  const auto now = std::chrono::steady_clock::now();
  if (now - ctx->last_snapshot_time < ctx->min_interval) {
    return;  // Rate limited
  }

  // Acquire snapshot from pool
  dmcp_snapshot_t* snapshot = dmcp::acquire_snapshot(ctx);
  if (!snapshot) {
    ctx->dropped_snapshots.fetch_add(1);
    if (!ctx->drop_warning_emitted.exchange(true)) {
      if (ctx->config.on_log) {
	ctx->config.on_log(ctx->config.user_data, DMCP_LOG_WARN,
	                   "DMCP: Snapshot pool exhausted, dropping frames");
      }
    }
    return;
  }

  // Fill snapshot via callback
  if (ctx->config.on_snapshot) {
    ctx->config.on_snapshot(ctx->config.user_data, snapshot);
  }

  // Serialize and broadcast
  const std::string json = dmcp::snapshot_to_json(*snapshot);
  if (!json.empty()) {
    mcp_server_broadcast(ctx->server, "state", json.c_str());
  }

  // Release snapshot back to pool
  dmcp::release_snapshot(ctx, snapshot);

  ctx->last_snapshot_time = now;
}

bool dmcp_screenshot_requested(const dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return false;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) return false;
  return ctx->screenshot.pending_requests.load() > 0;
}

dmcp_result_t dmcp_submit_screenshot(dmcp_context_t*                ctx_handle,
                                     const dmcp_screenshot_frame_t* frame) {
  if (!ctx_handle || !frame) {
    return DMCP_ERROR_INVALID_ARGS;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) {
    return DMCP_ERROR_DISABLED;
  }

  // TODO: Implement PNG encoding and storage
  // For now, just decrement pending counter

  const auto prev = ctx->screenshot.pending_requests.fetch_sub(1);
  if (prev == 0) {
    ctx->screenshot.pending_requests.fetch_add(1);
  }

  return DMCP_OK;
}

void dmcp_get_stats(const dmcp_context_t* ctx_handle, dmcp_stats_t* stats) {
  if (!ctx_handle || !stats) return;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);

  stats->dropped_snapshots   = ctx->dropped_snapshots.load();
  stats->dropped_screenshots = ctx->dropped_screenshots.load();

  mcp_server_stats_t server_stats{};
  mcp_server_get_stats(ctx->server, &server_stats);
  stats->connected_clients = server_stats.connected_clients;
}

int dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot, char* buffer,
                          size_t buffer_size) {
  if (!snapshot || !buffer || buffer_size == 0) {
    return -1;
  }

  const std::string json = dmcp::snapshot_to_json(*snapshot);
  if (json.size() >= buffer_size) {
    return -1;
  }

  std::strcpy(buffer, json.c_str());
  return static_cast<int>(json.size());
}

}  // extern "C"
