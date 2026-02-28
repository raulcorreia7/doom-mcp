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
  const char** (*tools)(void);
  bool (*register_methods)(dmcp_layer_t* layer, mcp_server_t* server, void* user_data);
  bool (*register_routes)(dmcp_layer_t* layer, mcp_server_t* server, void* user_data);
  void (*tick)(dmcp_layer_t* layer, dmcp_context_t* ctx);
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

#ifdef __cplusplus
}
#endif
