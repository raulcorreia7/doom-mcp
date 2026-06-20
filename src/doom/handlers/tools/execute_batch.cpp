#include "tools.hpp"
#include "dmcp/doom/commands.h"
#include "dmcp/doom/protocol.h"

#include <string_view>
#include <vector>

namespace dmcp {

namespace {

bool parse_batch_call(const json_value& call_obj, const command_tool_definition** out_tool,
                      json_value* out_arguments, std::string* out_error) {
  if (!out_tool || !out_arguments || !out_error) {
    return false;
  }

  *out_tool      = nullptr;
  *out_arguments = {};
  *out_error     = "Invalid batch call";

  if (!call_obj.is_object()) {
    *out_error = "Batch call must be an object";
    return false;
  }

  json_value name_val = call_obj["name"];
  if (!name_val.is_string()) {
    *out_error = "Batch call requires string name";
    return false;
  }

  const std::string_view tool_name{name_val.get_string()};
  if (tool_name == tools::change_level) {
    *out_error =
        "change_level is not supported in execute_batch; run it separately and wait for "
        "get_command_result before sending follow-up commands";
    return false;
  }

  const command_tool_definition* tool = find_command_tool(tool_name);
  if (!tool) {
    *out_error = "Batch call name must be a command tool";
    return false;
  }

  json_value arguments = call_obj["arguments"];
  if (!arguments.is_object()) {
    *out_error = "Batch call requires object arguments";
    return false;
  }

  *out_tool      = tool;
  *out_arguments = arguments;
  return true;
}

}  // namespace

bool handle_tool_execute_batch(context* ctx, const json_value& params, char* response_buffer,
                               size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call execute_batch");

  json_value args = extract_tool_arguments(params);

  json_value calls_arr = args["calls"];
  if (!calls_arr.is_array()) {
    const std::string resp = build_content_response("'calls' must be an array", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  std::vector<uint64_t> sequences;

  json_builder accepted;
  accepted.start_array();

  json_builder rejected;
  rejected.start_array();

  for (size_t original_idx = 0; original_idx < calls_arr.size(); ++original_idx) {
    const size_t exec_idx = original_idx;
    json_value   call_obj = calls_arr[original_idx];

    const command_tool_definition* tool = nullptr;
    json_value                     arguments;
    std::string                    call_error;
    if (!parse_batch_call(call_obj, &tool, &arguments, &call_error)) {
      json_builder item;
      item.start_object();
      item.add("index", static_cast<int64_t>(original_idx));
      item.add("execution_index", static_cast<int64_t>(exec_idx));
      item.add("error", call_error);
      rejected.push(std::move(item));
      continue;
    }

    const std::string cmd_json = std::string("{\"type\":\"") + tool->command_type +
                                 "\",\"params\":" + arguments.dump() + "}";
    dmcp_command_t cmd{};
    std::string    error_message;
    if (queue_command_from_json(ctx, cmd_json, &cmd, &error_message)) {
      sequences.push_back(cmd.sequence);
      json_builder item;
      item.start_object();
      item.add("index", static_cast<int64_t>(original_idx));
      item.add("execution_index", static_cast<int64_t>(exec_idx));
      item.add("sequence", static_cast<int64_t>(cmd.sequence));
      item.add("tool_name", tool->tool_name);
      item.add("command_type", static_cast<int64_t>(cmd.type));
      accepted.push(std::move(item));
    } else {
      json_builder item;
      item.start_object();
      item.add("index", static_cast<int64_t>(original_idx));
      item.add("execution_index", static_cast<int64_t>(exec_idx));
      item.add("error", error_message.empty() ? "Invalid command" : error_message);
      rejected.push(std::move(item));
    }
  }

  json_builder result;
  result.start_object();
  result.add("requested", static_cast<int64_t>(calls_arr.size()));
  result.add("queued", static_cast<int64_t>(sequences.size()));
  result.add("rejected", static_cast<int64_t>(calls_arr.size() - sequences.size()));
  result.add("ordering", "as_provided");

  json_builder execution_order;
  execution_order.start_array();
  for (size_t idx = 0; idx < calls_arr.size(); ++idx) {
    execution_order.push(static_cast<int64_t>(idx));
  }
  result.add("execution_order", std::move(execution_order));

  json_builder seq_arr;
  seq_arr.start_array();
  for (uint64_t seq : sequences) {
    seq_arr.push(static_cast<int64_t>(seq));
  }
  result.add("sequences", std::move(seq_arr));
  result.add("accepted_commands", std::move(accepted));
  result.add("rejected_commands", std::move(rejected));

  if (sequences.empty()) {
    result.add("status", "error");
    result.add("message", "No valid commands in batch");
    const std::string resp = build_content_response(result.finish(), true);
    return write_json_response(resp, response_buffer, response_size);
  }

  result.add("status", "queued");

  const std::string payload = result.finish();
  const std::string resp    = build_content_response(payload, false);
  return write_json_response(resp, response_buffer, response_size);
}

json_builder build_execute_batch_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();

  json_builder calls_prop;
  calls_prop.start_object();
  calls_prop.add("type", "array");
  calls_prop.add("description",
                 "Array of command tool calls. Each item uses {name, arguments}; change_level is "
                 "not allowed in batch.");

  json_builder items_schema;
  items_schema.start_object();
  items_schema.add("type", "object");
  json_builder item_props;
  item_props.start_object();

  json_builder name_prop;
  name_prop.start_object();
  name_prop.add("type", "string");
  name_prop.add("description", "Command tool name: spawn_entity, give_item, pause_game, etc.");
  item_props.add("name", std::move(name_prop));

  json_builder arguments_prop;
  arguments_prop.start_object();
  arguments_prop.add("type", "object");
  arguments_prop.add("description", "Arguments for the named command tool");
  item_props.add("arguments", std::move(arguments_prop));

  items_schema.add("properties", std::move(item_props));
  json_builder item_required;
  item_required.start_array();
  item_required.push("name");
  item_required.push("arguments");
  items_schema.add("required", std::move(item_required));
  calls_prop.add("items", std::move(items_schema));

  props.add("calls", std::move(calls_prop));

  schema.add("properties", std::move(props));

  json_builder required;
  required.start_array();
  required.push("calls");
  schema.add("required", std::move(required));

  return schema;
}

}  // namespace dmcp
