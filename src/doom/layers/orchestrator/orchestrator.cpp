#include <cstring>

#include "dmcp/doom/api.h"
#include "dmcp/doom/protocol.h"
#include "doom/handlers/tools/tools.hpp"
#include "doom/internal.hpp"
#include "doom/internal/mcp_handlers.hpp"
#include "doom/layers/orchestrator/orchestrator.hpp"
#include "mcp/generic/server.h"

namespace dmcp {

namespace {

const char* ORCHESTRATOR_TOOLS[] = {
    DMCP_TOOL_SPAWN_ENTITY,      DMCP_TOOL_CHANGE_LEVEL,    DMCP_TOOL_GIVE_ITEM,
    DMCP_TOOL_SET_PLAYER_HEALTH, DMCP_TOOL_TELEPORT_PLAYER, DMCP_TOOL_EXECUTE_CONSOLE,
    DMCP_TOOL_PAUSE_GAME,        DMCP_TOOL_DAMAGE_ENTITY,   DMCP_TOOL_KILL_ENTITY,
    DMCP_TOOL_GET_STATE,         DMCP_TOOL_GET_SCREENSHOT,
};

}  // namespace

orchestrator_layer::orchestrator_layer() = default;

orchestrator_layer::~orchestrator_layer() = default;

const char* orchestrator_layer::layer_name() { return "orchestrator"; }

const char* orchestrator_layer::layer_description() {
  return "Game orchestration tools: spawn entities, change levels, give items, manage player state";
}

size_t orchestrator_layer::tool_count() {
  return sizeof(ORCHESTRATOR_TOOLS) / sizeof(ORCHESTRATOR_TOOLS[0]);
}

const char** orchestrator_layer::tools() { return ORCHESTRATOR_TOOLS; }

bool orchestrator_layer::register_methods(mcp_server_t* server, void* user_data) {
  if (!server || !user_data) {
    return false;
  }

  m_ctx = static_cast<context*>(user_data);

  auto register_method = [&](const char* method_name, mcp_method_handler_t handler) {
    mcp_result_t result = mcp_server_method_register(server, method_name, handler, m_ctx);
    if (result.code != MCP_RESULT_CODE_OK) {
      dmcp_log(m_ctx, MCP_LOG_ERROR, "Failed to register method %s: %s", method_name,
               result.message ? result.message : "unknown error");
      return false;
    }
    return true;
  };

  register_method(DMCP_TOOL_GET_STATE, dmcp::handle_method_get_state_section);
  register_method(DMCP_TOOL_GET_SCREENSHOT, dmcp::handle_method_get_screenshot);

  const char* command_tools[] = {
      DMCP_TOOL_SPAWN_ENTITY,      DMCP_TOOL_CHANGE_LEVEL,    DMCP_TOOL_GIVE_ITEM,
      DMCP_TOOL_SET_PLAYER_HEALTH, DMCP_TOOL_TELEPORT_PLAYER, DMCP_TOOL_EXECUTE_CONSOLE,
      DMCP_TOOL_PAUSE_GAME,        DMCP_TOOL_DAMAGE_ENTITY,   DMCP_TOOL_KILL_ENTITY,
  };

  for (const char* method_name : command_tools) {
    register_method(method_name, dmcp::handle_method_execute_command);
  }

  return true;
}

bool orchestrator_layer::register_routes(mcp_server_t* server, void* user_data) {
  (void)server;
  (void)user_data;
  return true;
}

void orchestrator_layer::tick(dmcp_context_t* ctx) { (void)ctx; }

dmcp_layer_t* create_orchestrator_layer() {
  auto* layer = new (std::nothrow) orchestrator_layer();
  if (!layer) {
    return nullptr;
  }

  dmcp_layer_t* handle = new (std::nothrow) dmcp_layer_t;
  if (!handle) {
    delete layer;
    return nullptr;
  }

  return handle;
}

}  // namespace dmcp
