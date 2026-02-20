#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include "dmcp/doom/api.h"
#include "internal/context.hpp"
#include "internal/mcp_handlers.hpp"
#include "internal/serialization.hpp"
#include "mcp/generic/constants.h"
#include "mcp/json/json.hpp"

namespace dmcp {

using json_builder  = ::mcp::json::Builder;
using json_document = ::mcp::json::Document;
using json_value    = ::mcp::json::Value;

namespace {

struct command_tool_definition {
  const char* tool_name;
  const char* command_type;
  const char* description;
};

static const command_tool_definition k_command_tools[] = {
    {"spawn_entity", "spawn_entity", "Spawn an entity in the current level"},
    {"change_level", "change_level", "Change to another map/level"},
    {"give_item", "give_item", "Give an item to the player"},
    {"set_player_health", "set_player_health", "Set player health value"},
    {"teleport_player", "teleport_player", "Teleport player to coordinates"},
    {"set_player_position", "set_player_position", "Set player position and facing"},
    {"execute_console", "execute_console", "Execute an engine console command"},
    {"pause_game", "pause_game", "Pause or unpause game simulation"},
    {"set_timescale", "set_timescale", "Adjust simulation speed"},
    {"damage_entity", "damage_entity", "Apply damage to a target entity"},
    {"kill_entity", "kill_entity", "Kill a target entity"},
};

const command_tool_definition* find_command_tool(std::string_view tool_name) {
  for (const auto& tool : k_command_tools) {
    if (tool_name == tool.tool_name) {
      return &tool;
    }
  }
  return nullptr;
}

const char* resolve_command_type_for_method(std::string_view method_name) {
  if (method_name == "execute_command") {
    return "execute_command";
  }

  const command_tool_definition* tool = find_command_tool(method_name);
  return tool ? tool->command_type : nullptr;
}

void add_empty_object_schema(json_builder* schema) {
  if (!schema) {
    return;
  }

  schema->start_object();
  schema->add("type", "object");

  json_builder props;
  props.start_object();
  schema->add("properties", std::move(props));
}

void add_property(json_builder* props, const char* key, const char* type, const char* description) {
  if (!props || !key || !type || !description) {
    return;
  }

  json_builder prop;
  prop.start_object();
  prop.add("type", type);
  prop.add("description", description);
  props->add(key, std::move(prop));
}

void add_command_tool_schema(const char* tool_name, json_builder* schema) {
  if (!tool_name || !schema) {
    return;
  }

  schema->start_object();
  schema->add("type", "object");

  json_builder props;
  props.start_object();

  json_builder required;
  required.start_array();
  bool has_required = false;

  if (std::strcmp(tool_name, "spawn_entity") == 0) {
    add_property(&props, "entity_class", "string", "Entity class, eg DoomImp or Zombieman");
    add_property(&props, "x", "number", "Spawn X coordinate");
    add_property(&props, "y", "number", "Spawn Y coordinate");
    add_property(&props, "angle", "number", "Facing angle in degrees");
    required.push("entity_class");
    has_required = true;
  } else if (std::strcmp(tool_name, "change_level") == 0) {
    add_property(&props, "map_name", "string", "Map name, eg E1M2 or MAP01");
    add_property(&props, "skill_level", "integer", "Difficulty level 1-5");
    add_property(&props, "reset_inventory", "boolean", "Reset inventory on map load");
    required.push("map_name");
    has_required = true;
  } else if (std::strcmp(tool_name, "give_item") == 0) {
    add_property(&props, "item_class", "string", "Item class, eg Shotgun or Medikit");
    add_property(&props, "amount", "integer", "Quantity to give");
    required.push("item_class");
    has_required = true;
  } else if (std::strcmp(tool_name, "set_player_health") == 0) {
    add_property(&props, "health", "number", "Health value (alias: value)");
    required.push("health");
    has_required = true;
  } else if (std::strcmp(tool_name, "teleport_player") == 0 ||
             std::strcmp(tool_name, "set_player_position") == 0) {
    add_property(&props, "x", "number", "Destination X coordinate");
    add_property(&props, "y", "number", "Destination Y coordinate");
    add_property(&props, "angle", "number", "Facing angle in degrees");
    required.push("x");
    required.push("y");
    has_required = true;
  } else if (std::strcmp(tool_name, "execute_console") == 0) {
    add_property(&props, "command", "string", "Raw console command to execute");
    required.push("command");
    has_required = true;
  } else if (std::strcmp(tool_name, "pause_game") == 0) {
    add_property(&props, "paused", "boolean", "Pause state (alias: pause)");
    required.push("paused");
    has_required = true;
  } else if (std::strcmp(tool_name, "set_timescale") == 0) {
    add_property(&props, "scale", "number", "Time scale multiplier");
    required.push("scale");
    has_required = true;
  } else if (std::strcmp(tool_name, "damage_entity") == 0) {
    add_property(&props, "target_tid", "integer", "Target enemy id from game state");
    add_property(&props, "damage", "number", "Damage amount");
    add_property(&props, "damage_type", "string", "Damage type label");
    required.push("target_tid");
    required.push("damage");
    has_required = true;
  } else if (std::strcmp(tool_name, "kill_entity") == 0) {
    add_property(&props, "target_tid", "integer", "Target enemy id from game state");
    required.push("target_tid");
    has_required = true;
  }

  schema->add("properties", std::move(props));
  if (has_required) {
    schema->add("required", std::move(required));
  }
}

json_builder build_execute_command_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();

  json_builder type_prop;
  type_prop.start_object();
  type_prop.add("type", "string");
  type_prop.add("description", "Command type: spawn_entity, change_level, give_item, etc.");
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
  return schema;
}

json_builder build_get_command_result_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();

  json_builder sequence_prop;
  sequence_prop.start_object();
  sequence_prop.add("type", "integer");
  sequence_prop.add("description", "Command sequence id returned by execute_command");
  props.add("sequence", std::move(sequence_prop));

  schema.add("properties", std::move(props));

  json_builder required;
  required.start_array();
  required.push("sequence");
  schema.add("required", std::move(required));
  return schema;
}

void add_command_tool(json_builder* tools, const char* name, const char* description,
                      json_builder schema) {
  if (!tools || !name || !description) {
    return;
  }

  json_builder tool;
  tool.start_object();
  tool.add("name", name);
  tool.add("description", description);
  tool.add("inputSchema", std::move(schema));
  tools->push(std::move(tool));
}

}  // namespace

static void dmcp_log(context* ctx, int level, const char* fmt, ...) {
  if (!ctx || !ctx->config.on_log || !fmt) {
    return;
  }

  char    buffer[512];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  ctx->config.on_log(ctx->config.user_data, level, buffer);
}

static std::string build_content_response(std::string_view text, bool is_error = false) {
  json_builder result;
  result.start_object();
  if (is_error) {
    result.add("isError", true);
  }
  json_builder content;
  content.start_array();
  json_builder item;
  item.start_object();
  item.add("type", "text");
  item.add("text", text);
  content.push(std::move(item));
  result.add("content", std::move(content));
  return result.finish();
}

static bool write_json_response(std::string_view payload, char* response_buffer,
                                size_t response_size) {
  if (!response_buffer || response_size == 0) {
    return false;
  }
  if (payload.size() >= response_size) {
    return false;
  }

  std::memcpy(response_buffer, payload.data(), payload.size());
  response_buffer[payload.size()] = '\0';
  return true;
}

static std::string get_game_state_json(context* ctx) {
  dmcp_snapshot_t snapshot_copy;
  {
    std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
    snapshot_copy = ctx->last_snapshot;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "get_game_state_json: player.hp=%d level.tic=%d enemy_count=%u",
           snapshot_copy.player.hp, snapshot_copy.level.tic, snapshot_copy.enemy_count);

  const std::string payload = dmcp::snapshot_to_json(snapshot_copy);
  return payload.empty() ? "{}" : payload;
}

static bool parse_sequence_field(const json_value& value, uint64_t* out_sequence) {
  if (!out_sequence || !value || !value.is_number()) {
    return false;
  }

  const std::string number_text = value.dump();
  if (number_text.empty()) {
    return false;
  }

  char* end_ptr                   = nullptr;
  errno                           = 0;
  const unsigned long long parsed = std::strtoull(number_text.c_str(), &end_ptr, 10);
  if (!end_ptr || end_ptr == number_text.c_str() || *end_ptr != '\0' || errno == ERANGE ||
      parsed == 0) {
    return false;
  }

  *out_sequence = static_cast<uint64_t>(parsed);
  return true;
}

static bool parse_sequence_from_params(const char* request_json, uint64_t* out_sequence,
                                       std::string* out_error) {
  if (!out_sequence || !out_error) {
    return false;
  }

  *out_sequence = 0;
  *out_error    = "Invalid command sequence";

  json_document doc;
  if (!doc.parse(request_json ? request_json : "{}")) {
    *out_error = "Invalid request params";
    return false;
  }

  json_value root = doc.root();
  if (!root.is_object()) {
    *out_error = "Request params must be an object";
    return false;
  }

  if (!parse_sequence_field(root["sequence"], out_sequence)) {
    *out_error = "sequence must be a positive integer";
    return false;
  }

  return true;
}

static std::string build_command_result_json(const dmcp_command_result_t& result) {
  json_builder payload;
  payload.start_object();
  payload.add("sequence", static_cast<int64_t>(result.sequence));
  payload.add("command_type", static_cast<int64_t>(result.command_type));

  if (!result.completed) {
    payload.add("status", "pending");
  } else if (result.success) {
    payload.add("status", "success");
  } else {
    payload.add("status", "failed");
  }

  payload.add("completed", result.completed);
  payload.add("success", result.success);
  if (result.entity_id >= 0) {
    payload.add("entity_id", static_cast<int64_t>(result.entity_id));
  }
  if (result.message[0] != '\0') {
    payload.add("message", result.message);
  }

  return payload.finish();
}

static bool queue_command_from_json(context* ctx, std::string_view command_json,
                                    dmcp_command_t* out_cmd, std::string* error_message) {
  if (!ctx || !out_cmd || !error_message) {
    return false;
  }

  *error_message = "Invalid command";
  if (command_json.empty()) {
    return false;
  }

  const std::string    command_payload(command_json);
  dmcp_command_t       cmd{};
  mcp_result_generic_t parse_result = dmcp_parse_command_json_ex(
      reinterpret_cast<dmcp_context_t*>(ctx), command_payload.c_str(), &cmd);

  if (parse_result.code != MCP_RESULT_CODE_OK) {
    if (parse_result.message && parse_result.message[0] != '\0') {
      *error_message = parse_result.message;
    }
    return false;
  }

  mcp_result_generic_t push_result =
      dmcp_push_command(reinterpret_cast<dmcp_context_t*>(ctx), &cmd);
  if (push_result.code != MCP_RESULT_CODE_OK) {
    if (push_result.message && push_result.message[0] != '\0') {
      *error_message = push_result.message;
    } else {
      *error_message = "Failed to queue command";
    }
    return false;
  }

  dmcp_command_result_mark_queued(reinterpret_cast<dmcp_context_t*>(ctx), &cmd);

  *out_cmd = cmd;
  return true;
}

static std::string extract_command_json(const json_value& params) {
  json_value command_args = params["arguments"];
  if (!command_args.is_object()) {
    command_args = params["params"];
  }
  if (!command_args.is_object() && params["type"].is_string()) {
    command_args = params;
  }
  if (!command_args.is_object()) {
    return {};
  }
  return command_args.dump();
}

static bool queue_command_and_respond(context* ctx, std::string_view command_json,
                                      std::string_view command_name, char* response_buffer,
                                      size_t response_size) {
  dmcp_command_t cmd{};
  std::string    error_message = "Invalid command";
  if (queue_command_from_json(ctx, command_json, &cmd, &error_message)) {
    if (!command_name.empty()) {
      dmcp_log(ctx, MCP_LOG_INFO, "Command queued via %.*s (type=%d)",
               static_cast<int>(command_name.size()), command_name.data(), cmd.type);
    }

    char queue_msg[128];
    std::snprintf(queue_msg, sizeof(queue_msg), "Command queued (execution pending), sequence=%llu",
                  static_cast<unsigned long long>(cmd.sequence));
    const std::string resp = build_content_response(queue_msg, false);
    return write_json_response(resp, response_buffer, response_size);
  }

  if (!command_name.empty()) {
    dmcp_log(ctx, MCP_LOG_WARN, "%.*s failed: %s", static_cast<int>(command_name.size()),
             command_name.data(), error_message.c_str());
  }

  const std::string resp = build_content_response(error_message, true);
  return write_json_response(resp, response_buffer, response_size);
}

static bool handle_tool_get_game_state(context* ctx, char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_state: entering");
  const std::string payload = get_game_state_json(ctx);
  if (payload.empty()) {
    dmcp_log(ctx, MCP_LOG_WARN, "tools/call get_game_state: payload empty");
  } else {
    dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_state: payload size=%zu", payload.size());
  }
  const std::string resp = build_content_response(payload, false);
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_state: wrapped size=%zu buffer=%zu",
           resp.size(), response_size);
  bool success = write_json_response(resp, response_buffer, response_size);
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_state: write_json_response=%d", success);
  return success;
}

static bool handle_tool_get_screenshot(context* ctx, char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_screenshot");
  if (!ctx->screenshot.enabled) {
    const std::string resp = build_content_response("Screenshot feature disabled", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  if (ctx->screenshot.latest_pixels.empty()) {
    const std::string resp = build_content_response("No screenshot available", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  const char* ascii = dmcp_screenshot_get_ascii(reinterpret_cast<dmcp_context_t*>(ctx), 160);
  if (!ascii) {
    const std::string resp = build_content_response("Screenshot conversion failed", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  const std::string resp = build_content_response(ascii, false);
  return write_json_response(resp, response_buffer, response_size);
}

static bool handle_tool_execute_command(context* ctx, const json_value& params,
                                        char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call execute_command");

  const std::string command_json = extract_command_json(params);
  if (command_json.empty()) {
    dmcp_log(ctx, MCP_LOG_WARN, "execute_command missing arguments/type payload");
    const std::string resp = build_content_response("Invalid command payload", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  return queue_command_and_respond(ctx, command_json, "execute_command", response_buffer,
                                   response_size);
}

static bool handle_tool_get_command_result(context* ctx, const json_value& params,
                                           char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_command_result");

  json_value request = params["arguments"];
  if (!request.is_object()) {
    request = params["params"];
  }
  if (!request.is_object()) {
    request = params;
  }

  uint64_t sequence = 0;
  if (!parse_sequence_field(request["sequence"], &sequence)) {
    const std::string resp = build_content_response("sequence must be a positive integer", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  dmcp_command_result_t result = {};
  mcp_result_generic_t  get_result =
      dmcp_command_result_get(reinterpret_cast<dmcp_context_t*>(ctx), sequence, &result);
  if (get_result.code != MCP_RESULT_CODE_OK) {
    const std::string resp = build_content_response("Command result not found", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  const std::string result_json = build_command_result_json(result);
  const std::string resp        = build_content_response(result_json, false);
  return write_json_response(resp, response_buffer, response_size);
}

static bool handle_tool_command_alias(context* ctx, const command_tool_definition* command_tool,
                                      const json_value& params, char* response_buffer,
                                      size_t response_size) {
  if (!command_tool) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call %s", command_tool->tool_name);

  json_value command_args = params["arguments"];
  if (!command_args.is_object()) {
    command_args = params["params"];
  }
  if (!command_args.is_object()) {
    command_args = params;
  }

  const std::string params_json  = command_args.is_object() ? command_args.dump() : "{}";
  const std::string command_json = std::string("{\"type\":\"") + command_tool->command_type +
                                   "\",\"params\":" + params_json + "}";

  return queue_command_and_respond(ctx, command_json, command_tool->tool_name, response_buffer,
                                   response_size);
}

static bool write_route_response(std::string_view json, int status, char* response_buffer,
                                 size_t response_size, int* http_status) {
  if (!response_buffer || response_size == 0 || !http_status) {
    return false;
  }
  if (json.size() >= response_size) {
    return false;
  }

  std::memcpy(response_buffer, json.data(), json.size());
  response_buffer[json.size()] = '\0';
  *http_status                 = status;
  return true;
}

static std::string build_route_error(std::string_view code, std::string_view message) {
  json_builder error;
  error.start_object();
  error.add("code", code);
  error.add("message", message);

  json_builder root;
  root.start_object();
  root.add("error", std::move(error));
  return root.finish();
}

bool handle_route_game_state(void* user_data, const char* method, const char* path,
                             const char* body, char* response_buffer, size_t response_size,
                             int* http_status) {
  (void)path;
  (void)body;

  auto* ctx = static_cast<context*>(user_data);
  if (!ctx || !method || std::strcmp(method, "GET") != 0) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "route GET /game/state");
  dmcp_snapshot_t snapshot_copy;
  {
    std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
    snapshot_copy = ctx->last_snapshot;
  }

  const std::string payload = dmcp::snapshot_to_json(snapshot_copy);
  return write_route_response(payload.empty() ? "{}" : payload, 200, response_buffer, response_size,
                              http_status);
}

bool handle_route_game_screenshot(void* user_data, const char* method, const char* path,
                                  const char* body, char* response_buffer, size_t response_size,
                                  int* http_status) {
  (void)path;
  (void)body;

  auto* ctx = static_cast<context*>(user_data);
  if (!ctx || !method || std::strcmp(method, "GET") != 0) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "route GET /game/screenshot");

  if (!ctx->screenshot.enabled.load()) {
    const std::string payload = build_route_error("not_found", "Screenshot route disabled");
    return write_route_response(payload, 404, response_buffer, response_size, http_status);
  }

  if (ctx->screenshot.latest_pixels.empty()) {
    const std::string payload = build_route_error("not_found", "Screenshot not available");
    return write_route_response(payload, 404, response_buffer, response_size, http_status);
  }

  int written = dmcp_screenshot_to_json(reinterpret_cast<dmcp_context_t*>(ctx), response_buffer,
                                        response_size, 160);
  if (written < 0) {
    const std::string payload = build_route_error("internal_error", "Screenshot encoding failed");
    return write_route_response(payload, 500, response_buffer, response_size, http_status);
  }

  *http_status = 200;
  return true;
}

bool handle_tools_list(void* user_data, const char* /*method*/, const char* /*request_json*/,
                       char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/list requested");

  json_builder tools;
  tools.start_array();

  json_builder get_game_state_schema;
  add_empty_object_schema(&get_game_state_schema);
  add_command_tool(&tools, "get_game_state",
                   "Get current game state including player position, health, enemies",
                   std::move(get_game_state_schema));

  if (ctx->screenshot.enabled) {
    json_builder screenshot_schema;
    add_empty_object_schema(&screenshot_schema);
    add_command_tool(&tools, "get_screenshot", "Capture a screenshot of the current game state",
                     std::move(screenshot_schema));
  }

  add_command_tool(&tools, "execute_command",
                   "Execute a game command (spawn enemy, change level, etc.)",
                   build_execute_command_schema());

  add_command_tool(&tools, "get_command_result",
                   "Get asynchronous execution status for a queued command sequence",
                   build_get_command_result_schema());

  for (const auto& command_tool : k_command_tools) {
    json_builder schema;
    add_command_tool_schema(command_tool.tool_name, &schema);
    add_command_tool(&tools, command_tool.tool_name, command_tool.description, std::move(schema));
  }

  json_builder result;
  result.start_object();
  result.add("tools", std::move(tools));

  const std::string resp = result.finish();
  return write_json_response(resp, response_buffer, response_size);
}

bool handle_tools_call(void* user_data, const char* /*method*/, const char* request_json,
                       char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);

  json_document doc;
  if (!doc.parse(request_json)) return false;

  json_value        params = doc.root();
  const std::string tool_name{params["name"].get_string()};

  if (tool_name == "get_game_state") {
    return handle_tool_get_game_state(ctx, response_buffer, response_size);
  }

  if (tool_name == "get_screenshot") {
    return handle_tool_get_screenshot(ctx, response_buffer, response_size);
  }

  if (tool_name == "execute_command") {
    return handle_tool_execute_command(ctx, params, response_buffer, response_size);
  }

  if (tool_name == "get_command_result") {
    return handle_tool_get_command_result(ctx, params, response_buffer, response_size);
  }

  if (const command_tool_definition* command_tool = find_command_tool(tool_name)) {
    return handle_tool_command_alias(ctx, command_tool, params, response_buffer, response_size);
  }

  return false;
}

bool handle_method_get_game_state(void* user_data, const char* method, const char* request_json,
                                  char* response_buffer, size_t response_size) {
  (void)request_json;

  auto* ctx = static_cast<context*>(user_data);
  if (!ctx) {
    return false;
  }
  if (!method || std::strcmp(method, "get_game_state") != 0) {
    dmcp_log(ctx, MCP_LOG_WARN, "method get_game_state: invalid method param");
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "method get_game_state: entering");
  const std::string payload = get_game_state_json(ctx);
  if (payload.empty()) {
    dmcp_log(ctx, MCP_LOG_WARN, "method get_game_state: payload empty");
  }
  bool success = write_json_response(payload, response_buffer, response_size);
  dmcp_log(ctx, MCP_LOG_DEBUG, "method get_game_state: success=%d", success);
  return success;
}

bool handle_method_get_screenshot(void* user_data, const char* method, const char* request_json,
                                  char* response_buffer, size_t response_size) {
  (void)request_json;

  auto* ctx = static_cast<context*>(user_data);
  if (!ctx || !method || std::strcmp(method, "get_screenshot") != 0) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "method get_screenshot");
  if (!ctx->screenshot.enabled.load() || ctx->screenshot.latest_pixels.empty()) {
    const std::string payload = build_route_error("not_found", "Screenshot not available");
    return write_json_response(payload, response_buffer, response_size);
  }

  int written = dmcp_screenshot_to_json(reinterpret_cast<dmcp_context_t*>(ctx), response_buffer,
                                        response_size, 160);
  return written >= 0;
}

bool handle_method_get_command_result(void* user_data, const char* method, const char* request_json,
                                      char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);
  if (!ctx || !method || std::strcmp(method, "get_command_result") != 0) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "method get_command_result");

  uint64_t    sequence = 0;
  std::string error;
  if (!parse_sequence_from_params(request_json, &sequence, &error)) {
    json_builder payload;
    payload.start_object();
    payload.add("status", "error");
    payload.add("message", error);
    return write_json_response(payload.finish(), response_buffer, response_size);
  }

  dmcp_command_result_t result = {};
  mcp_result_generic_t  get_result =
      dmcp_command_result_get(reinterpret_cast<dmcp_context_t*>(ctx), sequence, &result);
  if (get_result.code == MCP_RESULT_CODE_NOT_FOUND) {
    json_builder payload;
    payload.start_object();
    payload.add("status", "not_found");
    payload.add("sequence", static_cast<int64_t>(sequence));
    payload.add("message", "Command result not found");
    return write_json_response(payload.finish(), response_buffer, response_size);
  }

  if (get_result.code != MCP_RESULT_CODE_OK) {
    json_builder payload;
    payload.start_object();
    payload.add("status", "error");
    payload.add("message", get_result.message ? get_result.message : "Failed to read result");
    return write_json_response(payload.finish(), response_buffer, response_size);
  }

  return write_json_response(build_command_result_json(result), response_buffer, response_size);
}

bool handle_method_execute_command(void* user_data, const char* method, const char* request_json,
                                   char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);
  if (!ctx || !method) {
    return false;
  }

  const std::string_view method_name(method);
  const char*            command_type = resolve_command_type_for_method(method_name);
  if (!command_type) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "method %.*s", static_cast<int>(method_name.size()),
           method_name.data());

  std::string command_json;
  if (method_name == "execute_command") {
    command_json = request_json ? request_json : "{}";
  } else {
    const std::string params_json = request_json ? request_json : "{}";
    command_json =
        std::string("{\"type\":\"") + command_type + "\",\"params\":" + params_json + "}";
  }

  dmcp_command_t cmd{};
  std::string    error_message = "Invalid command";
  if (!queue_command_from_json(ctx, command_json, &cmd, &error_message)) {
    json_builder error;
    error.start_object();
    error.add("status", "error");
    error.add("message", error_message);
    return write_json_response(error.finish(), response_buffer, response_size);
  }

  json_builder result;
  result.start_object();
  result.add("status", "queued");
  result.add("command_type", static_cast<int64_t>(cmd.type));
  result.add("sequence", static_cast<int64_t>(cmd.sequence));
  return write_json_response(result.finish(), response_buffer, response_size);
}

}  // namespace dmcp
