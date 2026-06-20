#include "doom/internal/mcp_registration.hpp"

#include "dmcp/doom/protocol.h"
#include "doom/internal/mcp_handlers.hpp"
#include "doom/handlers/tools/tools.hpp"
#include "mcp/game/registration.h"

namespace dmcp {

namespace {

constexpr const char* k_unknown_error = "unknown error";

}  // namespace

// Doom MCP is the only layer that maps Doom tool names onto generic MCP
// registration. Engines and adapters submit state/commands through the C API;
// they do not register MCP tools directly.
bool register_mcp_surface(context* ctx) {
  if (!ctx || !ctx->server) {
    return false;
  }

  const mcp_method_registration_t methods[] = {
      {"tools/list", handle_tools_list, ctx},
      {"tools/call", handle_tools_call, ctx},
  };

  const mcp_game_route_registration_t game_routes[] = {
      {"GET", "/game/state", dmcp::handle_route_game_state, ctx},
      {"GET", "/game/screenshot", dmcp::handle_route_game_screenshot, ctx},
  };

  mcp_game_registration_t registration = mcp_game_registration_default();
  registration.methods                 = methods;
  registration.method_count            = sizeof(methods) / sizeof(methods[0]);

  if (ctx->config.tools.game) {
    registration.routes      = game_routes;
    registration.route_count = sizeof(game_routes) / sizeof(game_routes[0]);
  } else {
    dmcp_log(ctx, MCP_LOG_INFO, "DMCP game tool group disabled by configuration");
  }

  const mcp_status_t status = mcp_game_register(ctx->server, &registration);
  if (!mcp_status_is_ok(status)) {
    dmcp_log(ctx, MCP_LOG_ERROR, "Failed to register Doom MCP surface: %s",
             status.message ? status.message : k_unknown_error);
    return false;
  }

  return true;
}

void broadcast_state_event(context* ctx, const std::string& json) {
  if (!ctx || !ctx->server || json.empty()) {
    return;
  }

  mcp_server_event_broadcast(ctx->server, "state", json.c_str());
}

}  // namespace dmcp
