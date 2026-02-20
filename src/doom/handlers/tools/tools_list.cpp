#include "tools.hpp"
#include "mcp/generic/constants.h"

namespace dmcp {

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

  if (ctx->screenshot.enabled.load()) {
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

  add_command_tool(&tools, "get_available_content",
                   "Get available weapons, items, enemies, and maps for the game mode",
                   build_get_available_content_schema());

  add_command_tool(&tools, "execute_batch", "Execute multiple commands in a single request",
                   build_execute_batch_schema());

  add_command_tool(&tools, "get_command_examples",
                   "Get working examples of all commands including ammo, weapons, items",
                   build_get_command_examples_schema());

  const command_tool_definition* command_tools = get_command_tools_array();
  size_t                         count         = get_command_tools_count();
  for (size_t i = 0; i < count; ++i) {
    json_builder schema;
    add_command_tool_schema(command_tools[i].tool_name, &schema);
    add_command_tool(&tools, command_tools[i].tool_name, command_tools[i].description,
                     std::move(schema));
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

  json_value params   = doc.root();
  json_value name_val = params["name"];
  if (!name_val.is_string()) {
    const std::string resp = build_content_response("Missing or invalid 'name' field", true);
    return write_json_response(resp, response_buffer, response_size);
  }
  const std::string tool_name{name_val.get_string()};
  if (tool_name.empty()) {
    const std::string resp = build_content_response("Tool name cannot be empty", true);
    return write_json_response(resp, response_buffer, response_size);
  }

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

  if (tool_name == "get_available_content") {
    return handle_tool_get_available_content(ctx, params, response_buffer, response_size);
  }

  if (tool_name == "execute_batch") {
    return handle_tool_execute_batch(ctx, params, response_buffer, response_size);
  }

  if (tool_name == "get_command_examples") {
    return handle_tool_get_command_examples(ctx, response_buffer, response_size);
  }

  if (const command_tool_definition* command_tool = find_command_tool(tool_name)) {
    return handle_tool_command_alias(ctx, command_tool, params, response_buffer, response_size);
  }

  return false;
}

}  // namespace dmcp
