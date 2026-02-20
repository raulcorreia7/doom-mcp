#include <cstring>
#include <string>
#include <string_view>

#include "dmcp/doom/api.h"
#include "doom/handlers/tools/tools.hpp"
#include "doom/internal/context.hpp"
#include "doom/internal/serialization.hpp"
#include "mcp/generic/constants.h"
#include "mcp/json/json.hpp"

namespace dmcp {

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
