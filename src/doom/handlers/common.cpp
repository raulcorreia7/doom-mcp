#include <cerrno>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include "dmcp/doom/api.h"
#include "doom/handlers/tools/tools.hpp"
#include "doom/internal/context.hpp"
#include "doom/internal/json_types.hpp"
#include "doom/internal/serialization.hpp"
#include "mcp/generic/constants.h"

namespace dmcp {

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

void dmcp_log(const context* ctx, int level, const char* fmt, ...) {
  if (!ctx || !ctx->config.on_log || !fmt) {
    return;
  }

  // Note: messages longer than 511 chars are silently truncated
  char    buffer[512];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  ctx->config.on_log(ctx->config.user_data, level, buffer);
}

std::string build_content_response(std::string_view text, bool is_error) {
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

bool write_json_response(std::string_view payload, char* response_buffer, size_t response_size) {
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

std::string get_game_state_json(context* ctx) {
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

bool parse_sequence_field(const json_value& value, uint64_t* out_sequence) {
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

bool parse_sequence_from_params(const char* request_json, uint64_t* out_sequence,
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

std::string build_command_result_json(const dmcp_command_result_t& result) {
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

bool queue_command_from_json(context* ctx, std::string_view command_json, dmcp_command_t* out_cmd,
                             std::string* error_message) {
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

json_value extract_tool_arguments(const json_value& params) {
  json_value args = params["arguments"];
  if (!args.is_object()) {
    args = params["params"];
  }
  if (!args.is_object()) {
    args = params;
  }
  return args;
}

std::string extract_command_json(const json_value& params) {
  json_value command_args = extract_tool_arguments(params);
  if (!command_args.is_object() && params["type"].is_string()) {
    command_args = params;
  }
  if (!command_args.is_object()) {
    return {};
  }
  return command_args.dump();
}

bool queue_command_and_respond(context* ctx, std::string_view command_json,
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

bool write_route_response(std::string_view json, int status, char* response_buffer,
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

std::string build_route_error(std::string_view code, std::string_view message) {
  json_builder error;
  error.start_object();
  error.add("code", code);
  error.add("message", message);

  json_builder root;
  root.start_object();
  root.add("error", std::move(error));
  return root.finish();
}

const command_tool_definition* get_command_tools_array() { return k_command_tools; }

size_t get_command_tools_count() { return sizeof(k_command_tools) / sizeof(k_command_tools[0]); }

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

bool handle_tool_command_alias(context* ctx, const command_tool_definition* command_tool,
                               const json_value& params, char* response_buffer,
                               size_t response_size) {
  if (!command_tool) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call %s", command_tool->tool_name);

  json_value        command_args = extract_tool_arguments(params);
  const std::string params_json  = command_args.is_object() ? command_args.dump() : "{}";
  const std::string command_json = std::string("{\"type\":\"") + command_tool->command_type +
                                   "\",\"params\":" + params_json + "}";

  return queue_command_and_respond(ctx, command_json, command_tool->tool_name, response_buffer,
                                   response_size);
}

}  // namespace dmcp
