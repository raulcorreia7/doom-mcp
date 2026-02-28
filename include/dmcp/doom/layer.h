#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "dmcp/doom/export.h"
#include "mcp/generic/protocol.h"
#include "mcp/generic/server.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct dmcp_layer_s   dmcp_layer_t;
typedef struct dmcp_context_s dmcp_context_t;

// ============================================================================
// Layer VTable - Implemented by Each Layer
// ============================================================================

typedef struct dmcp_layer_vtable_s dmcp_layer_vtable_t;

struct dmcp_layer_vtable_s {
  const char* (*name)(void);
  const char* (*description)(void);
  size_t (*tool_count)(void);
  const char* const* (*tools)(void);
  bool (*register_methods)(dmcp_layer_t* layer, mcp_server_t* server, void* user_data);
  bool (*register_routes)(dmcp_layer_t* layer, mcp_server_t* server, void* user_data);
  void (*tick)(dmcp_layer_t* layer, dmcp_context_t* ctx);
  void (*destroy)(dmcp_layer_t* layer);
};

// ============================================================================
// Layer Handle
// ============================================================================

struct dmcp_layer_s {
  const dmcp_layer_vtable_t* vtable;
  void*                      state;
};

// ============================================================================
// Layer Static Info
// ============================================================================

typedef struct {
  const char* name;
  const char* description;
  size_t      tool_count;
} dmcp_layer_info_t;

// ============================================================================
// Layer Registry (Opaque Handle)
// ============================================================================

typedef struct dmcp_layer_registry_s dmcp_layer_registry_t;

// ============================================================================
// Built-in Layer Names
// ============================================================================

#define DMCP_LAYER_ORCHESTRATOR "orchestrator"
#define DMCP_LAYER_INPUT "input"

// ============================================================================
// Registry C API
// ============================================================================

DMCP_API dmcp_layer_registry_t* dmcp_layer_registry_create(void);
DMCP_API void                   dmcp_layer_registry_destroy(dmcp_layer_registry_t* registry);
DMCP_API bool   dmcp_layer_registry_register(dmcp_layer_registry_t* registry, dmcp_layer_t* layer);
DMCP_API bool   dmcp_layer_registry_enable(dmcp_layer_registry_t* registry, const char* name);
DMCP_API bool   dmcp_layer_registry_disable(dmcp_layer_registry_t* registry, const char* name);
DMCP_API bool   dmcp_layer_registry_is_enabled(const dmcp_layer_registry_t* registry,
                                               const char*                  name);
DMCP_API size_t dmcp_layer_registry_count(const dmcp_layer_registry_t* registry);
DMCP_API void   dmcp_layer_registry_tick_all(dmcp_layer_registry_t* registry, dmcp_context_t* ctx);

#ifdef __cplusplus
}
#endif
