#include <cstring>
#include <string>

#include "internal/context.hpp"
#include "mcp/generic/constants.h"
#include "mcp/json/json.hpp"

namespace dmcp {

using json_builder  = ::mcp::json::Builder;
using json_document = ::mcp::json::Document;
using json_value    = ::mcp::json::Value;

static std::string build_content_response(const char* text,
                                          bool        is_error = false) {
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

bool handle_tools_list(void* user_data, const char* /*method*/,
                       const char* /*request_json*/, char* response_buffer,
                       size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);

  json_builder tools;
  tools.start_array();

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

bool handle_tools_call(void*       user_data, const char* /*method*/,
                       const char* request_json, char* response_buffer,
                       size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);

  json_document doc;
  if (!doc.parse(request_json)) return false;

  json_value        params = doc.root();
  const std::string tool_name{params["name"].get_string()};

  if (tool_name == "get_game_state") {
    const std::string resp = build_content_response(
	"Use SSE stream for real-time game state", false);
    if (resp.size() >= response_size) return false;
    std::strcpy(response_buffer, resp.c_str());
    return true;
  }

  if (tool_name == "get_screenshot") {
    if (!ctx->screenshot.enabled) return false;

    ctx->screenshot.pending_requests.fetch_add(1);

    const std::string resp =
	build_content_response("Screenshot capture queued", false);
    if (resp.size() >= response_size) return false;
    std::strcpy(response_buffer, resp.c_str());
    return true;
  }

  if (tool_name == "execute_command") {
    json_value command_params = params["params"];

    std::string params_json = "{}";
    if (command_params.is_object()) {
      auto members = command_params.members();
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

      const std::string resp = build_content_response("Command queued", false);
      if (resp.size() >= response_size) return false;
      std::strcpy(response_buffer, resp.c_str());
      return true;
    } else {
      const std::string resp = build_content_response("Invalid command", true);
      if (resp.size() >= response_size) return false;
      std::strcpy(response_buffer, resp.c_str());
      return true;
    }
  }

  return false;
}

}  // namespace dmcp
