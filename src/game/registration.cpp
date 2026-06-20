#include "mcp/game/registration.h"

#include <cstring>
#include <vector>

#include "mcp/core/memory.h"

namespace {

mcp_game_registration_t copy_registration(const mcp_game_registration_t* registration) {
  mcp_game_registration_t local = mcp_game_registration_default();
  if (!registration) {
    return local;
  }

  size_t copy_size = registration->struct_size;
  if (copy_size == 0 || copy_size > sizeof(mcp_game_registration_t)) {
    copy_size = sizeof(mcp_game_registration_t);
  }

  mcp_memcpy_safe(&local, sizeof(local), registration, copy_size);
  return local;
}

bool validate_registration(const mcp_game_registration_t& registration) {
  if ((registration.method_count > 0 && !registration.methods) ||
      (registration.route_count > 0 && !registration.routes)) {
    return false;
  }

  for (size_t i = 0; i < registration.method_count; ++i) {
    if (!registration.methods[i].method || !registration.methods[i].handler) {
      return false;
    }
  }

  for (size_t i = 0; i < registration.route_count; ++i) {
    if (!registration.routes[i].method || !registration.routes[i].path ||
        !registration.routes[i].handler) {
      return false;
    }
  }

  return true;
}

}  // namespace

extern "C" {

mcp_status_t mcp_game_register(mcp_server_t* server, const mcp_game_registration_t* registration) {
  if (!server || !registration) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  const mcp_game_registration_t local = copy_registration(registration);
  if (!validate_registration(local)) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid registration");
  }

  mcp_status_t status = mcp_server_methods_register(server, local.methods, local.method_count);
  if (!mcp_status_is_ok(status)) {
    return status;
  }

  // Route registration can partially fail; unwind everything this helper added
  // so callers do not need game-specific cleanup paths.
  std::vector<size_t> registered_routes;
  registered_routes.reserve(local.route_count);

  for (size_t i = 0; i < local.route_count; ++i) {
    const mcp_game_route_registration_t& route = local.routes[i];
    status =
        mcp_server_route_register(server, route.method, route.path, route.handler, route.user_data);
    if (!mcp_status_is_ok(status)) {
      for (size_t registered_index : registered_routes) {
        const mcp_game_route_registration_t& registered = local.routes[registered_index];
        mcp_server_route_unregister(server, registered.method, registered.path);
      }
      for (size_t method_index = 0; method_index < local.method_count; ++method_index) {
        mcp_server_method_unregister(server, local.methods[method_index].method);
      }
      return status;
    }
    registered_routes.push_back(i);
  }

  return MCP_STATUS_OK("Success");
}

void mcp_game_unregister(mcp_server_t* server, const mcp_game_registration_t* registration) {
  if (!server || !registration) {
    return;
  }

  const mcp_game_registration_t local = copy_registration(registration);

  if (local.routes) {
    for (size_t i = 0; i < local.route_count; ++i) {
      if (local.routes[i].method && local.routes[i].path) {
        mcp_server_route_unregister(server, local.routes[i].method, local.routes[i].path);
      }
    }
  }

  if (local.methods) {
    for (size_t i = 0; i < local.method_count; ++i) {
      if (local.methods[i].method) {
        mcp_server_method_unregister(server, local.methods[i].method);
      }
    }
  }
}
}
