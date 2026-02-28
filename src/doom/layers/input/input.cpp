#include "input.hpp"

#include <cstddef>
#include <new>

#include "dmcp/doom/layer.h"
#include "dmcp/doom/protocol.h"
#include "mcp/generic/server.h"
#include "doom/handlers/tools/tools.hpp"
#include "doom/internal/context.hpp"
#include "doom/internal/mcp_handlers.hpp"

namespace {

constexpr const char* LAYER_NAME        = "input";
constexpr const char* LAYER_DESCRIPTION = "Player input control layer (player_input tool)";

constexpr size_t   TOOL_COUNT             = 1;
static const char* TOOL_NAMES[TOOL_COUNT] = {DMCP_TOOL_PLAYER_INPUT};

struct input_layer_state {
  bool initialized = false;
};

const char* layer_name(void) { return LAYER_NAME; }

const char* layer_description(void) { return LAYER_DESCRIPTION; }

size_t tool_count(void) { return TOOL_COUNT; }

const char** tools(void) { return TOOL_NAMES; }

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

static const dmcp_layer_vtable_t INPUT_LAYER_VTABLE = {
    .name             = layer_name,
    .description      = layer_description,
    .tool_count       = tool_count,
    .tools            = tools,
    .register_methods = register_methods,
    .register_routes  = register_routes,
    .tick             = tick,
};

}  // namespace

extern "C" {

dmcp_layer_t* dmcp_input_layer_create(void) {
  dmcp_layer_t* layer = new (std::nothrow) dmcp_layer_t;
  if (!layer) {
    return nullptr;
  }

  layer->vtable = &INPUT_LAYER_VTABLE;

  auto* state = new (std::nothrow) input_layer_state;
  if (!state) {
    delete layer;
    return nullptr;
  }

  state->initialized = true;
  layer->state       = state;

  return layer;
}

void dmcp_input_layer_destroy(dmcp_layer_t* layer) {
  if (!layer) {
    return;
  }

  if (layer->state) {
    auto* state = static_cast<input_layer_state*>(layer->state);
    delete state;
  }

  delete layer;
}

}  // extern "C"
