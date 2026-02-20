#include <cstring>
#include <string>
#include <string_view>

#include "dmcp/doom/api.h"
#include "doom/handlers/tools/tools.hpp"
#include "doom/internal/context.hpp"
#include "doom/internal/serialization.hpp"
#include "mcp/generic/constants.h"
#include "mcp/json/json.hpp"

namespace dmcp {

bool handle_route_game_state(void* user_data, const char* method, const char* path,
                             const char* body, char* response_buffer, size_t response_size,
                             int* http_status) {
  (void)path;
  (void)body;

  auto* ctx = static_cast<context*>(user_data);
  if (!ctx || !method || std::strcmp(method, "GET") != 0) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "route GET /game/state");
  dmcp_snapshot_t snapshot_copy;
  {
    std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
    snapshot_copy = ctx->last_snapshot;
  }

  const std::string payload = dmcp::snapshot_to_json(snapshot_copy);
  return write_route_response(payload.empty() ? "{}" : payload, 200, response_buffer, response_size,
                              http_status);
}

bool handle_route_game_screenshot(void* user_data, const char* method, const char* path,
                                  const char* body, char* response_buffer, size_t response_size,
                                  int* http_status) {
  (void)path;
  (void)body;

  auto* ctx = static_cast<context*>(user_data);
  if (!ctx || !method || std::strcmp(method, "GET") != 0) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "route GET /game/screenshot");

  if (!ctx->screenshot.enabled.load()) {
    const std::string payload = build_route_error("not_found", "Screenshot route disabled");
    return write_route_response(payload, 404, response_buffer, response_size, http_status);
  }

  if (ctx->screenshot.latest_pixels.empty()) {
    const std::string payload = build_route_error("not_found", "Screenshot not available");
    return write_route_response(payload, 404, response_buffer, response_size, http_status);
  }

  int written = dmcp_screenshot_to_json(reinterpret_cast<dmcp_context_t*>(ctx), response_buffer,
                                        response_size, 160);
  if (written < 0) {
    const std::string payload = build_route_error("internal_error", "Screenshot encoding failed");
    return write_route_response(payload, 500, response_buffer, response_size, http_status);
  }

  *http_status = 200;
  return true;
}

}  // namespace dmcp
