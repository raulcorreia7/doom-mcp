#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "dmcp/doom/api.h"
#include "dmcp/doom/protocol.h"
#include "doom/commands/types/command_parsers.hpp"
#include "doom/handlers/tools/tools.hpp"
#include "doom/internal/context.hpp"
#include "doom/internal/serialization.hpp"
#include "mcp/generic/constants.h"
#include "doom/internal/json_types.hpp"

namespace dmcp {

namespace {

bool parse_json_object(const char* request_json, json_document* out_doc, json_value* out_root,
                       std::string* out_error) {
  if (!out_doc || !out_root || !out_error) {
    return false;
  }

  if (!out_doc->parse(request_json ? request_json : "{}")) {
    *out_error = "Invalid request params";
    return false;
  }

  *out_root = out_doc->root();
  if (!out_root->is_object()) {
    *out_error = "Request params must be an object";
    return false;
  }

  return true;
}

bool parse_command_result_sequences(const char* request_json, std::vector<uint64_t>* out_sequences,
                                    std::string* out_error) {
  if (!out_sequences || !out_error) {
    return false;
  }

  out_sequences->clear();
  *out_error = "Invalid command sequence";

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

  json_value sequences_val = root["sequences"];
  if (sequences_val.is_array()) {
    if (sequences_val.size() == 0) {
      *out_error = "sequences must be a non-empty array";
      return false;
    }

    for (size_t i = 0; i < sequences_val.size(); ++i) {
      uint64_t sequence = 0;
      if (!parse_sequence_field(sequences_val[i], &sequence)) {
        *out_error = "sequences must contain positive integers";
        return false;
      }
      out_sequences->push_back(sequence);
    }
    return true;
  }

  uint64_t sequence = 0;
  if (!parse_sequence_field(root["sequence"], &sequence)) {
    *out_error = "sequence must be a positive integer";
    return false;
  }

  out_sequences->push_back(sequence);
  return true;
}

}  // namespace

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

  std::vector<uint64_t> sequences;
  std::string           error;
  if (!parse_command_result_sequences(request_json, &sequences, &error)) {
    json_builder payload;
    payload.start_object();
    payload.add("status", "error");
    payload.add("message", error);
    return write_json_response(payload.finish(), response_buffer, response_size);
  }

  if (sequences.size() == 1) {
    dmcp_command_result_t result = {};
    mcp_result_generic_t  get_result =
        dmcp_command_result_get(reinterpret_cast<dmcp_context_t*>(ctx), sequences[0], &result);
    if (get_result.code == MCP_RESULT_CODE_NOT_FOUND) {
      json_builder payload;
      payload.start_object();
      payload.add("status", "not_found");
      payload.add("sequence", static_cast<int64_t>(sequences[0]));
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

  json_builder payload;
  payload.start_object();
  payload.add("status", "ok");
  payload.add("requested", static_cast<int64_t>(sequences.size()));

  json_builder results;
  results.start_array();

  json_builder not_found;
  not_found.start_array();

  for (uint64_t sequence : sequences) {
    dmcp_command_result_t result = {};
    mcp_result_generic_t  get_result =
        dmcp_command_result_get(reinterpret_cast<dmcp_context_t*>(ctx), sequence, &result);
    if (get_result.code == MCP_RESULT_CODE_OK) {
      results.push(build_command_result_object(result));
    } else {
      not_found.push(static_cast<int64_t>(sequence));
    }
  }

  payload.add("results", std::move(results));
  payload.add("not_found", std::move(not_found));
  return write_json_response(payload.finish(), response_buffer, response_size);
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
  if (method_name == tools::execute_command) {
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

bool handle_method_get_state_section(void* user_data, const char* method, const char* request_json,
                                     char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);
  if (!ctx || !method) {
    return false;
  }

  const std::string_view method_name(method);

  json_document doc;
  json_value    root;
  std::string   error;
  if (!parse_json_object(request_json, &doc, &root, &error)) {
    json_builder payload;
    payload.start_object();
    payload.add("status", "error");
    payload.add("message", error);
    return write_json_response(payload.finish(), response_buffer, response_size);
  }

  std::string_view section;
  if (method_name == tools::get_player) {
    section = section::player;
  } else if (method_name == tools::get_map || method_name == tools::get_level) {
    section = section::map;
  } else if (method_name == tools::get_game_info || method_name == tools::get_game) {
    section = section::game;
  } else if (method_name == tools::get_enemies) {
    section = section::enemies;
  } else if (method_name == tools::get_entities) {
    section = section::entities;
  } else if (method_name == tools::get_inventory) {
    section = section::inventory;
  } else if (method_name == tools::get_state) {
    json_value section_val = root["section"];
    if (!section_val.is_string()) {
      json_builder payload;
      payload.start_object();
      payload.add("status", "error");
      payload.add("message",
                  "section is required (player, enemies, entities, map, inventory, game)");
      return write_json_response(payload.finish(), response_buffer, response_size);
    }
    section = section_val.get_string();
  } else {
    return false;
  }

  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  std::string           payload;
  if (!build_state_section_payload(snapshot, section, root, &payload, &error)) {
    json_builder error_payload;
    error_payload.start_object();
    error_payload.add("status", "error");
    error_payload.add("message", error);
    return write_json_response(error_payload.finish(), response_buffer, response_size);
  }

  return write_json_response(payload, response_buffer, response_size);
}

bool handle_method_input(void* user_data, const char* method, const char* request_json,
                         char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);
  if (!ctx || !method || std::strcmp(method, DMCP_TOOL_PLAYER_INPUT) != 0) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "method input");

  json_document doc;
  if (!doc.parse(request_json ? request_json : "{}")) {
    json_builder error;
    error.start_object();
    error.add("status", "error");
    error.add("message", "Invalid request params");
    return write_json_response(error.finish(), response_buffer, response_size);
  }

  json_value root = doc.root();
  if (!root.is_object()) {
    json_builder error;
    error.start_object();
    error.add("status", "error");
    error.add("message", "Request params must be an object");
    return write_json_response(error.finish(), response_buffer, response_size);
  }

  dmcp_command_t cmd{};
  if (!parse_player_input_command(root, &cmd)) {
    json_builder error;
    error.start_object();
    error.add("status", "error");
    error.add("message", "Invalid input action");
    return write_json_response(error.finish(), response_buffer, response_size);
  }

  mcp_result_generic_t push_result = dmcp_push_input(reinterpret_cast<dmcp_context_t*>(ctx), &cmd);
  if (push_result.code != MCP_RESULT_CODE_OK) {
    json_builder error;
    error.start_object();
    error.add("status", "error");
    error.add("message", push_result.message ? push_result.message : "Failed to queue input");
    return write_json_response(error.finish(), response_buffer, response_size);
  }

  dmcp_log(ctx, MCP_LOG_INFO, "Input queued (action=%d seq=%llu)", cmd.data.input.action,
           static_cast<unsigned long long>(cmd.sequence));

  json_builder result;
  result.start_object();
  result.add("status", "executed");
  result.add("sequence", static_cast<int64_t>(cmd.sequence));
  result.add("action", static_cast<int64_t>(cmd.data.input.action));
  return write_json_response(result.finish(), response_buffer, response_size);
}

}  // namespace dmcp
