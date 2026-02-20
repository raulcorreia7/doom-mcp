#include "tools.hpp"
#include "dmcp/doom/commands.h"

#include <string_view>
#include <vector>

namespace dmcp {

namespace {

bool is_change_level_command(const json_value& cmd_obj) {
  if (!cmd_obj.is_object()) {
    return false;
  }

  json_value type_val = cmd_obj["type"];
  return type_val.is_string() && std::string_view(type_val.get_string()) == "change_level";
}

}  // namespace

bool handle_tool_execute_batch(context* ctx, const json_value& params, char* response_buffer,
                               size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call execute_batch");

  json_value args = extract_tool_arguments(params);

  json_value commands_arr = args["commands"];
  if (!commands_arr.is_array()) {
    const std::string resp = build_content_response("'commands' must be an array", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  std::vector<uint64_t> sequences;

  json_builder accepted;
  accepted.start_array();

  json_builder rejected;
  rejected.start_array();

  for (size_t original_idx = 0; original_idx < commands_arr.size(); ++original_idx) {
    const size_t exec_idx = original_idx;
    json_value   cmd_obj  = commands_arr[original_idx];

    if (!cmd_obj.is_object()) {
      json_builder item;
      item.start_object();
      item.add("index", static_cast<int64_t>(original_idx));
      item.add("execution_index", static_cast<int64_t>(exec_idx));
      item.add("error", "Command entry must be an object");
      rejected.push(std::move(item));
      continue;
    }

    if (is_change_level_command(cmd_obj)) {
      json_builder item;
      item.start_object();
      item.add("index", static_cast<int64_t>(original_idx));
      item.add("execution_index", static_cast<int64_t>(exec_idx));
      item.add("error",
               "change_level is not supported in execute_batch; run it separately and wait for "
               "get_command_result before sending follow-up commands");
      rejected.push(std::move(item));
      continue;
    }

    const std::string cmd_json = cmd_obj.dump();
    if (cmd_json.empty()) {
      json_builder item;
      item.start_object();
      item.add("index", static_cast<int64_t>(original_idx));
      item.add("execution_index", static_cast<int64_t>(exec_idx));
      item.add("error", "Command entry JSON is empty");
      rejected.push(std::move(item));
      continue;
    }

    dmcp_command_t cmd{};
    std::string    error_message;
    if (queue_command_from_json(ctx, cmd_json, &cmd, &error_message)) {
      sequences.push_back(cmd.sequence);
      json_builder item;
      item.start_object();
      item.add("index", static_cast<int64_t>(original_idx));
      item.add("execution_index", static_cast<int64_t>(exec_idx));
      item.add("sequence", static_cast<int64_t>(cmd.sequence));
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
  result.add("requested", static_cast<int64_t>(commands_arr.size()));
  result.add("queued", static_cast<int64_t>(sequences.size()));
  result.add("rejected", static_cast<int64_t>(commands_arr.size() - sequences.size()));
  result.add("ordering", "as_provided");

  json_builder execution_order;
  execution_order.start_array();
  for (size_t idx = 0; idx < commands_arr.size(); ++idx) {
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

  json_builder commands_prop;
  commands_prop.start_object();
  commands_prop.add("type", "array");
  commands_prop.add("description",
                    "Array of commands to execute (change_level is not allowed in batch)");

  json_builder items_schema;
  items_schema.start_object();
  items_schema.add("type", "object");
  json_builder item_props;
  item_props.start_object();

  json_builder type_prop;
  type_prop.start_object();
  type_prop.add("type", "string");
  type_prop.add("description", "Command type: spawn_entity, give_item, etc. (not change_level)");
  item_props.add("type", std::move(type_prop));

  json_builder params_prop;
  params_prop.start_object();
  params_prop.add("type", "object");
  params_prop.add("description", "Command-specific parameters");
  item_props.add("params", std::move(params_prop));

  items_schema.add("properties", std::move(item_props));
  commands_prop.add("items", std::move(items_schema));

  props.add("commands", std::move(commands_prop));

  schema.add("properties", std::move(props));

  json_builder required;
  required.start_array();
  required.push("commands");
  schema.add("required", std::move(required));

  return schema;
}

}  // namespace dmcp
