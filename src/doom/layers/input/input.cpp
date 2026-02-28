#include "input.hpp"

#include <cstddef>
#include <new>

#include "dmcp/doom/layer.h"
#include "dmcp/doom/protocol.h"
#include "doom/internal/mcp_handlers.hpp"
#include "mcp/generic/server.h"

namespace {

constexpr const char* LAYER_NAME        = DMCP_LAYER_INPUT;
constexpr const char* LAYER_DESCRIPTION = "Player input control layer (player_input tool)";

constexpr size_t      TOOL_COUNT             = 1;
constexpr const char* TOOL_NAMES[TOOL_COUNT] = {DMCP_TOOL_PLAYER_INPUT};

const char* layer_name(void) { return LAYER_NAME; }

const char* layer_description(void) { return LAYER_DESCRIPTION; }

size_t tool_count(void) { return TOOL_COUNT; }

const char* const* tools(void) { return TOOL_NAMES; }

bool register_methods(dmcp_layer_t* layer, mcp_server_t* server, void* user_data) {
  (void)layer;

  if (!server || !user_data) {
    return false;
  }

  mcp_result_t result = mcp_server_method_register(server, DMCP_TOOL_PLAYER_INPUT,
                                                   dmcp::handle_method_input, user_data);

  if (result.code != MCP_RESULT_CODE_OK) {
    return false;
  }

  return true;
}

bool register_routes(dmcp_layer_t* layer, mcp_server_t* server, void* user_data) {
  (void)layer;
  (void)server;
  (void)user_data;
  return true;
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

static const dmcp_layer_vtable_t INPUT_LAYER_VTABLE = {
    layer_name,       layer_description, tool_count, tools,
    register_methods, register_routes,   tick,       destroy,
};

}  // namespace

extern "C" {

dmcp_layer_t* dmcp_input_layer_create(void) {
  dmcp_layer_t* layer = new (std::nothrow) dmcp_layer_t;
  if (!layer) {
    return nullptr;
  }

  layer->vtable = &INPUT_LAYER_VTABLE;
  layer->state  = nullptr;

  return layer;
}

void dmcp_input_layer_destroy(dmcp_layer_t* layer) { destroy(layer); }

}  // extern "C"
