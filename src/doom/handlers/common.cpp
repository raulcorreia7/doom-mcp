#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include "dmcp/doom/api.h"
#include "dmcp/doom/content.h"
#include "dmcp/doom/protocol.h"
#include "doom/handlers/tools/tools.hpp"
#include "doom/internal/content_catalog.hpp"
#include "doom/internal/context.hpp"
#include "doom/internal/json_types.hpp"
#include "mcp/generic/constants.h"

namespace dmcp {

static const command_tool_definition k_command_tools[] = {
    {DMCP_TOOL_SPAWN_ENTITY, DMCP_CMD_NAME_SPAWN_ENTITY, "Spawn an entity in the current level"},
    {DMCP_TOOL_CHANGE_LEVEL, DMCP_CMD_NAME_CHANGE_LEVEL, "Change to another map/level"},
    {DMCP_TOOL_GIVE_ITEM, DMCP_CMD_NAME_GIVE_ITEM, "Give an item to the player"},
    {DMCP_TOOL_SET_PLAYER_HEALTH, DMCP_CMD_NAME_SET_PLAYER_HEALTH, "Set player health value"},
    {DMCP_TOOL_SET_PLAYER_POSITION, DMCP_CMD_NAME_SET_PLAYER_POSITION,
     "Set player position and facing"},
    {DMCP_TOOL_EXECUTE_CONSOLE, DMCP_CMD_NAME_EXECUTE_CONSOLE, "Execute an engine console command"},
    {DMCP_TOOL_PAUSE_GAME, DMCP_CMD_NAME_PAUSE_GAME, "Pause or unpause game simulation"},
    {DMCP_TOOL_DAMAGE_ENTITY, DMCP_CMD_NAME_DAMAGE_ENTITY, "Apply damage to a target entity"},
    {DMCP_TOOL_KILL_ENTITY, DMCP_CMD_NAME_KILL_ENTITY, "Kill a target entity"},
};

dmcp_gamemode_t parse_game_mode_from_string(std::string_view mode_name) {
  if (mode_name == mode::shareware) return DMCP_GAMEMODE_SHAREWARE;
  if (mode_name == mode::registered) return DMCP_GAMEMODE_REGISTERED;
  if (mode_name == mode::commercial || mode_name == mode::doom2) return DMCP_GAMEMODE_COMMERCIAL;
  if (mode_name == mode::retail || mode_name == mode::ultimate) return DMCP_GAMEMODE_RETAIL;
  return DMCP_GAMEMODE_UNKNOWN;
}

namespace {

dmcp_snapshot_t snapshot_for_validation(context* ctx) {
  dmcp_snapshot_t snapshot{};
  if (!ctx) {
    return snapshot;
  }

  std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
  snapshot = ctx->last_snapshot;
  return snapshot;
}

bool validate_command_for_mode(context* ctx, const dmcp_command_t& cmd, std::string* out_error) {
  if (!ctx || !out_error) {
    return false;
  }

  const dmcp_snapshot_t snapshot = snapshot_for_validation(ctx);
  const dmcp_gamemode_t mode     = resolve_game_mode(snapshot, {}).mode;
  if (mode == DMCP_GAMEMODE_UNKNOWN) {
    return true;
  }

  switch (cmd.type) {
    case DMCP_CMD_SPAWN_ENTITY:
      if (!dmcp_is_enemy_spawnable(cmd.data.spawn.entity_class, mode) &&
          !dmcp_is_item_available(cmd.data.spawn.entity_class, mode)) {
        *out_error =
            dmcp_content_unavailable_message("Entity/Item", cmd.data.spawn.entity_class, mode);
        return false;
      }
      break;
    case DMCP_CMD_GIVE_ITEM:
      if (!dmcp_is_item_available(cmd.data.give_item.item_class, mode)) {
        *out_error = dmcp_content_unavailable_message("Item", cmd.data.give_item.item_class, mode);
        return false;
      }
      break;
    case DMCP_CMD_CHANGE_LEVEL:
      if (!map_is_available_for_snapshot(snapshot, cmd.data.change_level.map_name, mode)) {
        *out_error = dmcp_content_unavailable_message("Map", cmd.data.change_level.map_name, mode);
        return false;
      }
      break;
    default:
      break;
  }

  return true;
}

}  // namespace

bool parse_size_from_number(const json_value& value, size_t* out) {
  if (!out || !value || !value.is_number()) {
    return false;
  }

  const std::string number_text = value.dump();
  if (number_text.empty()) {
    return false;
  }

  char* end_ptr                   = nullptr;
  errno                           = 0;
  const unsigned long long parsed = std::strtoull(number_text.c_str(), &end_ptr, 10);
  if (!end_ptr || end_ptr == number_text.c_str() || *end_ptr != '\0' || errno == ERANGE) {
    return false;
  }

  *out = static_cast<size_t>(parsed);
  return true;
}

const command_tool_definition* find_command_tool(std::string_view tool_name) {
  for (const auto& tool : k_command_tools) {
    if (tool_name == tool.tool_name) {
      return &tool;
    }
  }
  return nullptr;
}

const char* resolve_command_type_for_method(std::string_view method_name) {
  const command_tool_definition* tool = find_command_tool(method_name);
  return tool ? tool->command_type : nullptr;
}

void dmcp_log(const context* ctx, int level, const char* fmt, ...) {
  if (!ctx || !ctx->config.on_log || !fmt) {
    return;
  }

  char    buffer[512];
  va_list args;
  va_start(args, fmt);
  int result = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  if (result >= static_cast<int>(sizeof(buffer))) {
    std::strcpy(buffer + sizeof(buffer) - 4, "...");
  }

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

dmcp_snapshot_t copy_latest_snapshot(context* ctx) {
  dmcp_snapshot_t snapshot_copy{};
  if (!ctx) {
    return snapshot_copy;
  }

  std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
  snapshot_copy = ctx->last_snapshot;
  return snapshot_copy;
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

json_builder build_command_result_object(const dmcp_command_result_t& result) {
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

  return payload;
}

std::string build_command_result_json(const dmcp_command_result_t& result) {
  return build_command_result_object(result).finish();
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

  const std::string command_payload(command_json);
  dmcp_command_t    cmd{};
  mcp_status_t      parse_result = dmcp_parse_command_json(command_payload.c_str(), &cmd);

  if (parse_result.code != MCP_STATUS_CODE_OK) {
    if (parse_result.message && parse_result.message[0] != '\0') {
      *error_message = parse_result.message;
    }
    return false;
  }

  if (!validate_command_for_mode(ctx, cmd, error_message)) {
    return false;
  }

  mcp_status_t push_result = dmcp_push_command(reinterpret_cast<dmcp_context_t*>(ctx), &cmd);
  if (push_result.code != MCP_STATUS_CODE_OK) {
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
  if (params.has_member("arguments") && args.is_object()) {
    return args;
  }
  return {};
}

std::string extract_command_json(const json_value& params) {
  json_value command_args = extract_tool_arguments(params);
  if (!command_args.is_object() || !command_args["type"].is_string() ||
      !command_args["params"].is_object()) {
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

    json_builder payload;
    payload.start_object();
    payload.add("status", "queued");
    payload.add("sequence", static_cast<int64_t>(cmd.sequence));
    payload.add("command_type", static_cast<int64_t>(cmd.type));
    if (!command_name.empty()) {
      payload.add("command_name", std::string(command_name));
    }

    const std::string resp = build_content_response(payload.finish(), false);
    return write_json_response(resp, response_buffer, response_size);
  }

  if (!command_name.empty()) {
    dmcp_log(ctx, MCP_LOG_WARN, "%.*s failed: %s", static_cast<int>(command_name.size()),
             command_name.data(), error_message.c_str());
  }

  json_builder error_payload;
  error_payload.start_object();
  error_payload.add("status", "error");
  error_payload.add("message", error_message);
  const std::string resp = build_content_response(error_payload.finish(), true);
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

bool handle_tool_command(context* ctx, const command_tool_definition* command_tool,
                         const json_value& params, char* response_buffer, size_t response_size) {
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
