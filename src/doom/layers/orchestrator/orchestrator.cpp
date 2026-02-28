#include <array>
#include <new>

#include "dmcp/doom/layer.h"
#include "dmcp/doom/protocol.h"
#include "doom/internal/mcp_handlers.hpp"
#include "doom/layers/orchestrator/orchestrator.hpp"
#include "mcp/generic/server.h"

namespace dmcp {

namespace {

constexpr std::array<const char*, 26> k_orchestrator_tools = {
    DMCP_TOOL_GET_PLAYER,         DMCP_TOOL_GET_ENEMIES,
    DMCP_TOOL_GET_ENTITIES,       DMCP_TOOL_GET_MAP,
    DMCP_TOOL_GET_LEVEL,          DMCP_TOOL_GET_INVENTORY,
    DMCP_TOOL_GET_GAME_INFO,      DMCP_TOOL_GET_GAME,
    DMCP_TOOL_GET_STATE,          DMCP_TOOL_GET_STATE_BATCH,
    DMCP_TOOL_GET_SCREENSHOT,     DMCP_TOOL_EXECUTE_COMMAND,
    DMCP_TOOL_GET_COMMAND_RESULT, DMCP_TOOL_GET_AVAILABLE_CONTENT,
    DMCP_TOOL_EXECUTE_BATCH,      DMCP_TOOL_GET_COMMAND_EXAMPLES,
    DMCP_TOOL_SPAWN_ENTITY,       DMCP_TOOL_CHANGE_LEVEL,
    DMCP_TOOL_GIVE_ITEM,          DMCP_TOOL_SET_PLAYER_HEALTH,
    DMCP_TOOL_TELEPORT_PLAYER,    DMCP_TOOL_SET_PLAYER_POSITION,
    DMCP_TOOL_EXECUTE_CONSOLE,    DMCP_TOOL_PAUSE_GAME,
    DMCP_TOOL_DAMAGE_ENTITY,      DMCP_TOOL_KILL_ENTITY,
};

constexpr std::array<mcp_method_registration_t, 22> k_method_registrations = {{
    {DMCP_TOOL_GET_PLAYER, dmcp::handle_method_get_state_section, nullptr},
    {DMCP_TOOL_GET_ENEMIES, dmcp::handle_method_get_state_section, nullptr},
    {DMCP_TOOL_GET_ENTITIES, dmcp::handle_method_get_state_section, nullptr},
    {DMCP_TOOL_GET_MAP, dmcp::handle_method_get_state_section, nullptr},
    {DMCP_TOOL_GET_LEVEL, dmcp::handle_method_get_state_section, nullptr},
    {DMCP_TOOL_GET_INVENTORY, dmcp::handle_method_get_state_section, nullptr},
    {DMCP_TOOL_GET_GAME_INFO, dmcp::handle_method_get_state_section, nullptr},
    {DMCP_TOOL_GET_GAME, dmcp::handle_method_get_state_section, nullptr},
    {DMCP_TOOL_GET_STATE, dmcp::handle_method_get_state_section, nullptr},
    {DMCP_TOOL_GET_SCREENSHOT, dmcp::handle_method_get_screenshot, nullptr},
    {DMCP_TOOL_EXECUTE_COMMAND, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_GET_COMMAND_RESULT, dmcp::handle_method_get_command_result, nullptr},
    {DMCP_TOOL_SPAWN_ENTITY, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_CHANGE_LEVEL, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_GIVE_ITEM, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_SET_PLAYER_HEALTH, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_TELEPORT_PLAYER, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_SET_PLAYER_POSITION, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_EXECUTE_CONSOLE, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_PAUSE_GAME, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_DAMAGE_ENTITY, dmcp::handle_method_execute_command, nullptr},
    {DMCP_TOOL_KILL_ENTITY, dmcp::handle_method_execute_command, nullptr},
}};

const char* layer_name(void) { return DMCP_LAYER_ORCHESTRATOR; }

const char* layer_description(void) {
  return "Game orchestration tools for state reads and command execution";
}

size_t tool_count(void) { return k_orchestrator_tools.size(); }

const char* const* tools(void) { return k_orchestrator_tools.data(); }

bool register_methods(dmcp_layer_t* layer, mcp_server_t* server, void* user_data) {
  if (!layer || !server || !user_data) {
    return false;
  }

  std::array<mcp_method_registration_t, k_method_registrations.size()> registrations =
      k_method_registrations;
  for (auto& registration : registrations) {
    registration.user_data = user_data;
  }

  const mcp_result_t result =
      mcp_server_methods_register(server, registrations.data(), registrations.size());
  return result.code == MCP_RESULT_CODE_OK;
}

bool register_routes(dmcp_layer_t* layer, mcp_server_t* server, void* user_data) {
  if (!layer || !server || !user_data) {
    return false;
  }

  const mcp_result_t state_route = mcp_server_route_register(
      server, "GET", "/game/state", dmcp::handle_route_game_state, user_data);
  if (state_route.code != MCP_RESULT_CODE_OK) {
    return false;
  }

  const mcp_result_t screenshot_route = mcp_server_route_register(
      server, "GET", "/game/screenshot", dmcp::handle_route_game_screenshot, user_data);
  return screenshot_route.code == MCP_RESULT_CODE_OK;
}

void tick(dmcp_layer_t* layer, dmcp_context_t* ctx) {
  (void)layer;
  (void)ctx;
}

void destroy(dmcp_layer_t* layer) {
  if (!layer) {
    return;
  }

  delete layer;
}

static const dmcp_layer_vtable_t ORCHESTRATOR_LAYER_VTABLE = {
    layer_name,       layer_description, tool_count, tools,
    register_methods, register_routes,   tick,       destroy,
};

}  // namespace

dmcp_layer_t* create_orchestrator_layer() {
  dmcp_layer_t* layer = new (std::nothrow) dmcp_layer_t;
  if (!layer) {
    return nullptr;
  }

  layer->vtable = &ORCHESTRATOR_LAYER_VTABLE;
  layer->state  = nullptr;

  return layer;
}

}  // namespace dmcp

extern "C" {

dmcp_layer_t* dmcp_orchestrator_layer_create(void) { return dmcp::create_orchestrator_layer(); }

void dmcp_orchestrator_layer_destroy(dmcp_layer_t* layer) { dmcp::destroy(layer); }

}  // extern "C"
