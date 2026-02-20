#include "tools.hpp"
#include "doom/internal/serialization.hpp"

namespace dmcp {

bool handle_tool_get_game_state(context* ctx, char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_state: entering");
  const std::string payload = get_game_state_json(ctx);
  if (payload.empty()) {
    dmcp_log(ctx, MCP_LOG_WARN, "tools/call get_game_state: payload empty");
  } else {
    dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_state: payload size=%zu", payload.size());
  }
  const std::string resp = build_content_response(payload, false);
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_state: wrapped size=%zu buffer=%zu",
           resp.size(), response_size);
  bool success = write_json_response(resp, response_buffer, response_size);
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_state: write_json_response=%d", success);
  return success;
}

json_builder build_get_game_state_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();
  schema.add("properties", std::move(props));
  return schema;
}

}  // namespace dmcp
