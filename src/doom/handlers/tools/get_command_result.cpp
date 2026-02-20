#include "tools.hpp"

#include <vector>

namespace dmcp {

namespace {

bool parse_sequences_request(const json_value& request, std::vector<uint64_t>* out_sequences,
                             std::string* out_error) {
  if (!out_sequences || !out_error) {
    return false;
  }

  out_sequences->clear();

  json_value sequences_val = request["sequences"];
  if (sequences_val.is_array()) {
    if (sequences_val.size() == 0) {
      *out_error = "sequences must be a non-empty array";
      return false;
    }

    for (size_t i = 0; i < sequences_val.size(); ++i) {
      uint64_t sequence = 0;
      if (!parse_sequence_field(sequences_val[i], &sequence)) {
        *out_error = "sequences must contain positive integers";
        return false;
      }
      out_sequences->push_back(sequence);
    }
    return true;
  }

  uint64_t sequence = 0;
  if (!parse_sequence_field(request["sequence"], &sequence)) {
    *out_error = "sequence must be a positive integer";
    return false;
  }

  out_sequences->push_back(sequence);
  return true;
}

json_builder build_command_result_object(const dmcp_command_result_t& result) {
  json_builder payload;
  payload.start_object();
  payload.add("sequence", static_cast<int64_t>(result.sequence));
  payload.add("command_type", static_cast<int64_t>(result.command_type));

  if (!result.completed) {
    payload.add("status", "pending");
  } else if (result.success) {
    payload.add("status", "success");
  } else {
    payload.add("status", "failed");
  }

  payload.add("completed", result.completed);
  payload.add("success", result.success);
  if (result.entity_id >= 0) {
    payload.add("entity_id", static_cast<int64_t>(result.entity_id));
  }
  if (result.message[0] != '\0') {
    payload.add("message", result.message);
  }

  return payload;
}

}  // namespace

bool handle_tool_get_command_result(context* ctx, const json_value& params, char* response_buffer,
                                    size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_command_result");

  json_value request = extract_tool_arguments(params);

  std::vector<uint64_t> sequences;
  std::string           parse_error;
  if (!parse_sequences_request(request, &sequences, &parse_error)) {
    const std::string resp = build_content_response(parse_error, true);
    return write_json_response(resp, response_buffer, response_size);
  }

  if (sequences.size() == 1) {
    dmcp_command_result_t result = {};
    mcp_result_generic_t  get_result =
        dmcp_command_result_get(reinterpret_cast<dmcp_context_t*>(ctx), sequences[0], &result);
    if (get_result.code != MCP_RESULT_CODE_OK) {
      const std::string resp = build_content_response("Command result not found", true);
      return write_json_response(resp, response_buffer, response_size);
    }

    const std::string result_json = build_command_result_json(result);
    const std::string resp        = build_content_response(result_json, false);
    return write_json_response(resp, response_buffer, response_size);
  }

  json_builder payload;
  payload.start_object();
  payload.add("requested", static_cast<int64_t>(sequences.size()));

  json_builder results;
  results.start_array();

  json_builder not_found;
  not_found.start_array();

  for (uint64_t sequence : sequences) {
    dmcp_command_result_t result = {};
    mcp_result_generic_t  get_result =
        dmcp_command_result_get(reinterpret_cast<dmcp_context_t*>(ctx), sequence, &result);
    if (get_result.code == MCP_RESULT_CODE_OK) {
      results.push(build_command_result_object(result));
    } else {
      not_found.push(static_cast<int64_t>(sequence));
    }
  }

  payload.add("results", std::move(results));
  payload.add("not_found", std::move(not_found));

  const std::string resp = build_content_response(payload.finish(), false);
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
  sequence_prop.add("description", "Single command sequence id returned by execute_command");
  props.add("sequence", std::move(sequence_prop));

  json_builder sequences_prop;
  sequences_prop.start_object();
  sequences_prop.add("type", "array");
  sequences_prop.add("description", "Multiple command sequence ids for batch status lookup");
  json_builder item_schema;
  item_schema.start_object();
  item_schema.add("type", "integer");
  sequences_prop.add("items", std::move(item_schema));
  props.add("sequences", std::move(sequences_prop));

  schema.add("properties", std::move(props));
  return schema;
}

}  // namespace dmcp
