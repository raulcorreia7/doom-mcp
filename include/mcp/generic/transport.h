#pragma once

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Transport Abstraction
// 
// The transport layer handles the low-level HTTP/WebSocket communication.
// This allows swapping between different implementations (uWebSockets, libuv, etc.)
// ============================================================================

typedef struct mcp_transport_s mcp_transport_t;

// Transport callbacks
typedef struct {
  // Called when a new HTTP request arrives
  // Should return true if request was handled
  bool (*on_http_request)(void* user_data,
                          const char* method,      // GET, POST, etc.
                          const char* path,        // /mcp, /sse, etc.
                          const char* body,        // Request body (NULL if none)
                          char* response_buffer,   // Response buffer
                          size_t response_size,
                          int* http_status);       // HTTP status code to return

  // Called when a new SSE client connects
  // Returns a client handle (opaque pointer)
  void* (*on_sse_connect)(void* user_data,
                          void** client_handle);   // OUT: client handle

  // Called when SSE client disconnects
  void (*on_sse_disconnect)(void* user_data, void* client_handle);

  // Called to send data to a specific SSE client
  bool (*on_sse_send)(void* user_data, 
                      void* client_handle,
                      const char* data,
                      size_t len);

} mcp_transport_callbacks_t;

// Transport interface (vtable)
typedef struct {
  const char* name;  // Transport name (e.g., "sse-uws")

  // Create/destroy transport instance
  mcp_transport_t* (*create)(uint16_t port, 
                             const mcp_transport_callbacks_t* callbacks,
                             void* user_data);
  void (*destroy)(mcp_transport_t* transport);

  // Lifecycle
  bool (*start)(mcp_transport_t* transport);
  void (*stop)(mcp_transport_t* transport);
  bool (*is_running)(const mcp_transport_t* transport);

  // Broadcast to all connected SSE clients
  void (*broadcast)(mcp_transport_t* transport, const char* data, size_t len);

  // Get number of connected clients
  size_t (*get_client_count)(const mcp_transport_t* transport);

} mcp_transport_interface_t;

// SSE transport over HTTP (uWebSockets implementation)
extern const mcp_transport_interface_t mcp_sse_transport;

#ifdef __cplusplus
}
#endif
