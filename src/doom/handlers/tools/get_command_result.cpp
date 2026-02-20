#include "tools.hpp"

namespace dmcp {

bool handle_tool_get_command_result(context* ctx, const json_value& params, char* response_buffer,
                                    size_t response_size) {
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

}  // namespace dmcp
