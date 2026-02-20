#include "tools.hpp"

namespace dmcp {

bool handle_tool_execute_command(context* ctx, const json_value& params, char* response_buffer,
                                 size_t response_size) {
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

}  // namespace dmcp
