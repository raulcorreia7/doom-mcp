#include "tools.hpp"
#include "dmcp/doom/commands.h"
#include <vector>

namespace dmcp {

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

  for (size_t i = 0; i < commands_arr.size(); ++i) {
    json_value cmd_obj = commands_arr[i];
    if (!cmd_obj.is_object()) {
      continue;
    }

    const std::string cmd_json = cmd_obj.dump();
    if (cmd_json.empty()) {
      continue;
    }

    dmcp_command_t cmd{};
    std::string    error_message;
    if (queue_command_from_json(ctx, cmd_json, &cmd, &error_message)) {
      sequences.push_back(cmd.sequence);
    }
  }

  if (sequences.empty()) {
    const std::string resp = build_content_response("No valid commands in batch", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  json_builder result;
  result.start_object();
  result.add("queued", static_cast<int64_t>(sequences.size()));

  json_builder seq_arr;
  seq_arr.start_array();
  for (uint64_t seq : sequences) {
    seq_arr.push(static_cast<int64_t>(seq));
  }
  result.add("sequences", std::move(seq_arr));

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
  commands_prop.add("description", "Array of commands to execute");

  json_builder items_schema;
  items_schema.start_object();
  items_schema.add("type", "object");
  json_builder item_props;
  item_props.start_object();

  json_builder type_prop;
  type_prop.start_object();
  type_prop.add("type", "string");
  type_prop.add("description", "Command type: spawn_entity, give_item, etc.");
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
