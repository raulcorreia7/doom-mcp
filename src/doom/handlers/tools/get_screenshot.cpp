#include "tools.hpp"

namespace dmcp {

bool handle_tool_get_screenshot(context* ctx, char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_screenshot");
  if (!ctx->screenshot.enabled.load()) {
    const std::string resp = build_content_response("Screenshot feature disabled", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  dmcp_screenshot_request(reinterpret_cast<dmcp_context_t*>(ctx));

  if (ctx->screenshot.latest_pixels.empty()) {
    const std::string resp = build_content_response("No screenshot available", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  const char* ascii = dmcp_screenshot_get_ascii(reinterpret_cast<dmcp_context_t*>(ctx), 160);
  if (!ascii) {
    const std::string resp = build_content_response("Screenshot conversion failed", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  const std::string resp = build_content_response(ascii, false);
  return write_json_response(resp, response_buffer, response_size);
}

json_builder build_get_screenshot_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();
  schema.add("properties", std::move(props));
  return schema;
}

}  // namespace dmcp
