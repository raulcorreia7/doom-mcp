#pragma once

#include "mcp/game/export.h"
#include "mcp/generic/server.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct {
  const char*         method;
  const char*         path;
  mcp_route_handler_t handler;
  void*               user_data;
} mcp_game_route_registration_t;

/*
 * Game integrations provide flat arrays of MCP methods/routes. The generic game
 * layer owns the repetitive server registration and rollback plumbing; it must
 * not know anything about Doom, engines, or adapter types.
 */
typedef struct {
  uint32_t struct_size;

  const mcp_method_registration_t*     methods;
  size_t                               method_count;
  const mcp_game_route_registration_t* routes;
  size_t                               route_count;
} mcp_game_registration_t;

static inline mcp_game_registration_t mcp_game_registration_default(void) {
  mcp_game_registration_t registration;
  registration.struct_size  = sizeof(mcp_game_registration_t);
  registration.methods      = NULL;
  registration.method_count = 0;
  registration.routes       = NULL;
  registration.route_count  = 0;
  return registration;
}

MCP_GAME_API mcp_status_t mcp_game_register(mcp_server_t*                  server,
                                            const mcp_game_registration_t* registration);
MCP_GAME_API void         mcp_game_unregister(mcp_server_t*                  server,
                                              const mcp_game_registration_t* registration);

#ifdef __cplusplus
}
#endif
