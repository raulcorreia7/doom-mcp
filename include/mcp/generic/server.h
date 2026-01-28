#pragma once

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Opaque Server Handle
// ============================================================================
typedef struct mcp_server_s mcp_server_t;

// ============================================================================
// Lifecycle
// ============================================================================

// Create a new MCP server
mcp_server_t* mcp_server_create(const mcp_server_config_t* config);

// Destroy server and free all resources
void mcp_server_destroy(mcp_server_t* server);

// Check if server is running
bool mcp_server_is_running(const mcp_server_t* server);

// ============================================================================
// Method Registration
// ============================================================================

// Register a handler for a JSON-RPC method
// Methods: "initialize", "tools/list", "tools/call", etc.
mcp_result_t mcp_server_method_register(mcp_server_t*        server,
                                        const char*          method,
                                        mcp_method_handler_t handler,
                                        void*                user_data);

// Unregister a method handler
void mcp_server_method_unregister(mcp_server_t* server, const char* method);

// Register multiple methods atomically with a single mutex lock
typedef struct {
  const char*          method;
  mcp_method_handler_t handler;
  void*                user_data;
} mcp_method_registration_t;

mcp_result_t mcp_server_methods_register(
    mcp_server_t* server, const mcp_method_registration_t* methods,
    size_t count);

// ============================================================================
// Event Broadcasting (SSE)
// ============================================================================

// Broadcast an event to all connected clients
// event_type: "state", "screenshot", "notification", etc.
// json_payload: JSON data as null-terminated string
void mcp_server_event_broadcast(mcp_server_t* server, const char* event_type,
                                const char* json_payload);

// Get the number of connected clients
uint64_t mcp_server_clients_count(const mcp_server_t* server);

// ============================================================================
// Statistics
// ============================================================================

void mcp_server_stats_get(const mcp_server_t* server,
                          mcp_server_stats_t* stats);

// ============================================================================
// Utility Functions
// ============================================================================

// Helper to create a JSON-RPC 2.0 success response
// Returns number of bytes written (excluding null terminator)
size_t mcp_format_success_response(char* buffer, size_t buffer_size,
                                   const char* id, const char* result_json);

// Helper to create a JSON-RPC 2.0 error response
// Returns number of bytes written (excluding null terminator)
size_t mcp_format_error_response(char* buffer, size_t buffer_size,
                                 const char* id, int error_code,
                                 const char* error_message);

#ifdef __cplusplus
}
#endif
