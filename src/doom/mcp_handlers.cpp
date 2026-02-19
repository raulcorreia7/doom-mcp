#include <cstring>
#include <cstdarg>
#include <cstdio>
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

static std::string build_content_response(const char* text, bool is_error = false) {
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

  {
    json_builder tool;
    tool.start_object();
    tool.add("name", "get_game_state");
    tool.add("description", "Get current game state including player position, health, enemies");

    json_builder schema;
    schema.start_object();
    schema.add("type", "object");

    json_builder props;
    props.start_object();
    schema.add("properties", std::move(props));

    tool.add("inputSchema", std::move(schema));
    tools.push(std::move(tool));
  }

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

  {
    json_builder tool;
    tool.start_object();
    tool.add("name", "execute_command");
    tool.add("description", "Execute a game command (spawn enemy, change level, etc.)");

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

bool handle_tools_call(void* user_data, const char* /*method*/, const char* request_json,
                       char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);

  json_document doc;
  if (!doc.parse(request_json)) return false;

  json_value        params = doc.root();
  const std::string tool_name{params["name"].get_string()};

  if (tool_name == "get_game_state") {
    dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_state");
    dmcp_snapshot_t snapshot_copy;
    {
      std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
      snapshot_copy = ctx->last_snapshot;
    }
    const std::string json = dmcp::snapshot_to_json(snapshot_copy);
    const std::string resp = build_content_response(json.c_str(), false);
    if (resp.size() >= response_size) return false;
    std::strcpy(response_buffer, resp.c_str());
    return true;
  }

  if (tool_name == "get_screenshot") {
    dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_screenshot");
    if (!ctx->screenshot.enabled) return false;

    if (ctx->screenshot.latest_pixels.empty()) {
      const std::string resp = build_content_response("No screenshot available", true);
      if (resp.size() >= response_size) return false;
      std::strcpy(response_buffer, resp.c_str());
      return true;
    }

    const char* ascii = dmcp_screenshot_get_ascii(reinterpret_cast<dmcp_context_t*>(ctx), 160);
    if (!ascii) {
      const std::string resp = build_content_response("Screenshot conversion failed", true);
      if (resp.size() >= response_size) return false;
      std::strcpy(response_buffer, resp.c_str());
      return true;
    }

    const std::string resp = build_content_response(ascii, false);
    if (resp.size() >= response_size) return false;
    std::strcpy(response_buffer, resp.c_str());
    return true;
  }

  if (tool_name == "execute_command") {
    dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call execute_command");
    json_value command_args = params["arguments"];

    std::string params_json = "{}";
    if (command_args.is_object()) {
      auto members = command_args.members();
      if (!members.empty()) {
        json_builder pb;
        pb.start_object();
        for (const auto& kv : members) {
          if (kv.second.is_string()) {
            pb.add(kv.first, kv.second.get_string());
          } else if (kv.second.is_bool()) {
            pb.add(kv.first, kv.second.get_bool());
          } else if (kv.second.is_number()) {
            pb.add(kv.first, kv.second.get_double());
          }
        }
        params_json = pb.finish();
      }
    }

    dmcp_command_t       cmd{};
    mcp_result_generic_t parse_result = dmcp_parse_command_json_ex(
        reinterpret_cast<dmcp_context_t*>(ctx), params_json.c_str(), &cmd);

    if (parse_result.code == MCP_RESULT_CODE_OK) {
      dmcp_push_command(reinterpret_cast<dmcp_context_t*>(ctx), &cmd);
      dmcp_log(ctx, MCP_LOG_INFO, "Command queued via execute_command (type=%d)", cmd.type);

      const std::string resp = build_content_response("Command queued", false);
      if (resp.size() >= response_size) return false;
      std::strcpy(response_buffer, resp.c_str());
      return true;
    } else {
      dmcp_log(ctx, MCP_LOG_WARN, "Invalid execute_command payload");
      const std::string resp = build_content_response("Invalid command", true);
      if (resp.size() >= response_size) return false;
      std::strcpy(response_buffer, resp.c_str());
      return true;
    }
  }

  return false;
}

}  // namespace dmcp
