#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "dmcp/doom/api.h"
#include "internal.hpp"
#include "internal/mcp_handlers.hpp"
#include "internal/serialization.hpp"
#include "mcp/generic/server.h"

// Bring dmcp_log into scope for use inside extern "C" block
using dmcp::dmcp_log;

extern "C" {

dmcp_context_t* dmcp_context_create(const dmcp_config_t* config) {
  auto ctx = std::make_unique<dmcp::context>();

  ctx->config = config ? *config : dmcp_config_default();

  ctx->pool.resize(ctx->config.snapshot_pool_size);
  for (auto& entry : ctx->pool) {
    entry.in_use = false;
    dmcp_snapshot_clear(&entry.data);
  }

  ctx->min_interval       = std::chrono::nanoseconds{1'000'000'000ull / ctx->config.target_hz};
  ctx->last_snapshot_time = std::chrono::steady_clock::now() - ctx->min_interval;

  ctx->screenshot.enabled.store(ctx->config.screenshot.enable);

  ctx->cmd_queue = std::make_unique<dmcp::command_queue>();

  mcp_server_config_t server_config = mcp_default_config();
  server_config.port                = ctx->config.port;
  server_config.on_log              = ctx->config.on_log;
  server_config.log_user_data       = ctx->config.user_data;

  ctx->server = mcp_server_create(&server_config);
  if (!ctx->server) {
    dmcp_log(ctx.get(), MCP_LOG_ERROR, "Failed to create MCP server on port=%u", ctx->config.port);
    return nullptr;
  }

  auto register_method = [&](const char* method_name, mcp_method_handler_t handler) {
    mcp_result_t result = mcp_server_method_register(ctx->server, method_name, handler, ctx.get());
    if (result.code != MCP_RESULT_CODE_OK) {
      dmcp_log(ctx.get(), MCP_LOG_ERROR, "Failed to register method %s: %s", method_name,
               result.message ? result.message : "unknown error");
    }
  };

  register_method("tools/list", dmcp::handle_tools_list);
  register_method("tools/call", dmcp::handle_tools_call);

  // Agent compatibility aliases: allow direct JSON-RPC method calls without
  // requiring tools/call wrappers.
  register_method("get_player", dmcp::handle_method_get_state_section);
  register_method("get_enemies", dmcp::handle_method_get_state_section);
  register_method("get_entities", dmcp::handle_method_get_state_section);
  register_method("get_map", dmcp::handle_method_get_state_section);
  register_method("get_level", dmcp::handle_method_get_state_section);
  register_method("get_inventory", dmcp::handle_method_get_state_section);
  register_method("get_game_info", dmcp::handle_method_get_state_section);
  register_method("get_game", dmcp::handle_method_get_state_section);
  register_method("get_state", dmcp::handle_method_get_state_section);
  register_method("get_screenshot", dmcp::handle_method_get_screenshot);
  register_method("execute_command", dmcp::handle_method_execute_command);
  register_method("get_command_result", dmcp::handle_method_get_command_result);

  const char* command_method_aliases[] = {
      "spawn_entity",    "change_level",        "give_item",       "set_player_health",
      "teleport_player", "set_player_position", "execute_console", "pause_game",
      "damage_entity",   "kill_entity",
  };
  for (const char* method_name : command_method_aliases) {
    register_method(method_name, dmcp::handle_method_execute_command);
  }

  mcp_result_t route_state_result = mcp_server_route_register(
      ctx->server, "GET", "/game/state", dmcp::handle_route_game_state, ctx.get());
  if (route_state_result.code != MCP_RESULT_CODE_OK) {
    dmcp_log(ctx.get(), MCP_LOG_ERROR, "Failed to register route GET /game/state: %s",
             route_state_result.message);
  }

  mcp_result_t route_screenshot_result = mcp_server_route_register(
      ctx->server, "GET", "/game/screenshot", dmcp::handle_route_game_screenshot, ctx.get());
  if (route_screenshot_result.code != MCP_RESULT_CODE_OK) {
    dmcp_log(ctx.get(), MCP_LOG_ERROR, "Failed to register route GET /game/screenshot: %s",
             route_screenshot_result.message);
  }

  dmcp_log(ctx.get(), MCP_LOG_INFO, "DMCP context created (port=%u target_hz=%u screenshot=%s)",
           ctx->config.port, ctx->config.target_hz,
           ctx->config.screenshot.enable ? "enabled" : "disabled");

  return reinterpret_cast<dmcp_context_t*>(ctx.release());
}

void dmcp_context_destroy(dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  dmcp_log(ctx, MCP_LOG_INFO, "DMCP context destroying");

  if (ctx->server) {
    mcp_server_destroy(ctx->server);
  }

  delete ctx;
}

bool dmcp_context_is_running(const dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return false;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);
  return mcp_server_is_running(ctx->server);
}

void dmcp_context_tick(dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  const auto now = std::chrono::steady_clock::now();
  if (now - ctx->last_snapshot_time < ctx->min_interval) {
    return;
  }

  dmcp::pool_entry* snapshot = dmcp::acquire_snapshot(ctx->pool);
  if (!snapshot) {
    ctx->dropped_snapshots.fetch_add(1);
    if (!ctx->drop_warning_emitted.exchange(true)) {
      if (ctx->config.on_log) {
        ctx->config.on_log(ctx->config.user_data, MCP_LOG_WARN,
                           "DMCP: Snapshot pool exhausted, dropping frames");
      }
    }
    return;
  }

  if (ctx->config.on_snapshot) {
    ctx->config.on_snapshot(ctx->config.user_data, &snapshot->data);
  }

  {
    std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
    std::memcpy(&ctx->last_snapshot, &snapshot->data, sizeof(dmcp_snapshot_t));
  }

  const std::string json = dmcp::snapshot_to_json(snapshot->data);
  if (!json.empty()) {
    mcp_server_event_broadcast(ctx->server, "state", json.c_str());
  }

  dmcp::release_snapshot(ctx->pool, snapshot);

  ctx->last_snapshot_time = now;
}

bool dmcp_screenshot_is_requested(const dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return false;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) return false;
  return ctx->screenshot.pending_requests.load() > 0;
}

mcp_result_generic_t dmcp_screenshot_submit(dmcp_context_t*                ctx_handle,
                                            const dmcp_screenshot_frame_t* frame) {
  if (!ctx_handle || !frame) {
    return MCP_ERROR_INVALID_ARGS;
  }

  if (!frame->pixels || frame->width == 0 || frame->height == 0) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) {
    dmcp_log(ctx, MCP_LOG_WARN, "Screenshot submit rejected: screenshot feature disabled");
    return MCP_ERROR_DISABLED;
  }

  {
    std::lock_guard<std::mutex> lock(ctx->screenshot.mutex);
    ctx->screenshot.width  = frame->width;
    ctx->screenshot.height = frame->height;
    ctx->screenshot.latest_pixels.assign(frame->pixels,
                                         frame->pixels + (frame->width * frame->height * 4));
  }

  if (ctx->screenshot.pending_requests.load() > 0) {
    ctx->screenshot.pending_requests.fetch_sub(1);
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "Screenshot submitted (%ux%u)", frame->width, frame->height);

  return MCP_OK;
}

const char* dmcp_screenshot_get_ascii(dmcp_context_t* ctx_handle, uint32_t target_width) {
  if (!ctx_handle) return nullptr;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) return nullptr;
  if (ctx->screenshot.latest_pixels.empty()) return nullptr;

  ctx->screenshot.convert_to_ascii(target_width);
  dmcp_log(ctx, MCP_LOG_DEBUG, "ASCII screenshot generated (target_width=%u)", target_width);
  return ctx->screenshot.get_ascii().c_str();
}

int dmcp_screenshot_to_json(dmcp_context_t* ctx_handle, char* buffer, size_t buffer_size,
                            uint32_t target_width) {
  if (!ctx_handle || !buffer || buffer_size == 0) {
    return -1;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load() || ctx->screenshot.latest_pixels.empty()) {
    return -1;
  }

  ctx->screenshot.convert_to_ascii(target_width);
  const std::string& ascii = ctx->screenshot.get_ascii();

  std::string json = "{";
  json += "\"width\":" + std::to_string(ctx->screenshot.width) + ",";
  json += "\"height\":" + std::to_string(ctx->screenshot.height) + ",";
  json += "\"ascii\":";

  json += "\"";
  for (char c : ascii) {
    if (c == '"') {
      json += "\\\"";
    } else if (c == '\\') {
      json += "\\\\";
    } else if (c == '\n') {
      json += "\\n";
    } else if (c == '\r') {
      json += "\\r";
    } else {
      json += c;
    }
  }
  json += "\"";

  json += "}";

  if (json.size() >= buffer_size) {
    return -1;
  }

  std::strcpy(buffer, json.c_str());
  return static_cast<int>(json.size());
}

void dmcp_stats_get(const dmcp_context_t* ctx_handle, dmcp_stats_t* stats) {
  if (!ctx_handle || !stats) return;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);

  stats->dropped_snapshots   = ctx->dropped_snapshots.load();
  stats->dropped_screenshots = ctx->dropped_screenshots.load();

  mcp_server_stats_t server_stats{};
  mcp_server_stats_get(ctx->server, &server_stats);
  stats->connected_clients = server_stats.connected_clients;
}

int dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot, char* buffer, size_t buffer_size) {
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
}
