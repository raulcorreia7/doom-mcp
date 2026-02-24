#include "tools.hpp"
#include "dmcp/doom/commands.h"
#include "doom/commands/types/command_parsers.hpp"

namespace dmcp {

bool handle_tool_input(context* ctx, const json_value& params, char* response_buffer,
                       size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call input");

  json_value args = extract_tool_arguments(params);
  if (!args.is_object()) {
    const std::string resp = build_content_response("Invalid arguments", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  dmcp_command_t cmd{};
  if (!parse_player_input_command(args, &cmd)) {
    const std::string resp = build_content_response("Invalid input action", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  mcp_result_generic_t push_result = dmcp_push_input(reinterpret_cast<dmcp_context_t*>(ctx), &cmd);
  if (push_result.code != MCP_RESULT_CODE_OK) {
    const std::string resp = build_content_response(
        push_result.message ? push_result.message : "Failed to queue input", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  dmcp_log(ctx, MCP_LOG_INFO, "Input queued (action=%d seq=%llu)", cmd.data.input.action,
           static_cast<unsigned long long>(cmd.sequence));

  json_builder result;
  result.start_object();
  result.add("status", "executed");
  result.add("sequence", static_cast<int64_t>(cmd.sequence));
  result.add("action", static_cast<int64_t>(cmd.data.input.action));

  const std::string payload = result.finish();
  const std::string resp    = build_content_response(payload, false);
  return write_json_response(resp, response_buffer, response_size);
}

json_builder build_input_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();

  json_builder action_prop;
  action_prop.start_object();
  action_prop.add("type", "string");
  action_prop.add("description",
                  "Input action: fwd, back, left, right, tleft, tright, aim, atk, use, wpn");
  props.add("a", std::move(action_prop));

  json_builder value_prop;
  value_prop.start_object();
  value_prop.add("type", "number");
  value_prop.add("description", "Value for aim (angle 0-360) or wpn (slot 1-7)");
  props.add("v", std::move(value_prop));

  schema.add("properties", std::move(props));

  json_builder required;
  required.start_array();
  required.push("a");
  schema.add("required", std::move(required));

  return schema;
}

}  // namespace dmcp
