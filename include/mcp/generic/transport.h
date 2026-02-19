#pragma once

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Transport Abstraction
//
// The transport layer handles the low-level HTTP/WebSocket communication.
// This allows swapping between different implementations (uWebSockets, libuv,
// etc.)
// ============================================================================

/**
 * @brief Opaque handle to transport instance
 *
 * Transport implementations use opaque handles to maintain ABI stability.
 * The internal structure is implementation-specific.
 */
typedef struct mcp_transport_s mcp_transport_t;

// Transport callbacks
/**
 * @brief Transport callback functions
 *
 * Callbacks that the transport layer invokes to handle HTTP requests
 * and SSE client events. Implement these to integrate with the MCP server.
 *
 * @note All callbacks are called from transport worker threads
 * @note Implementations must be thread-safe
 */
typedef struct {
  size_t struct_size;  ///< Must be sizeof(mcp_transport_callbacks_t)

  /**
   * @brief Called when a new HTTP request arrives
   *
   * Handle incoming HTTP requests (GET, POST, etc.) and return response.
   * Return true if request was handled, false if not (causes 404).
   *
   * @param user_data User data pointer
   * @param method HTTP method (GET, POST, etc.)
   * @param path Request path (/mcp, /health, etc.)
   * @param body Request body (NULL if none or GET request)
   * @param response_buffer Output buffer for response
   * @param response_size Size of response buffer
   * @param http_status OUT: HTTP status code to return
   * @return true if handled, false otherwise
   */
  bool (*on_http_request)(void* user_data, const char* method, const char* path, const char* body,
                          char* response_buffer, size_t response_size, int* http_status);

  /**
   * @brief Called when a new SSE client connects
   *
   * Invoked when a client connects to the SSE stream endpoint (/mcp).
   * Returns an opaque client handle for this connection.
   *
   * @param user_data User data pointer
   * @param client_handle OUT: Opaque client handle (implementation-specific)
   * @return Client handle (implementation-specific pointer)
   */
  void* (*on_sse_connect)(void* user_data, void** client_handle);

  /**
   * @brief Called when SSE client disconnects
   *
   * Invoked when an SSE client disconnects or connection is lost.
   * Client handle will be invalid after this callback.
   *
   * @param user_data User data pointer
   * @param client_handle Client handle from on_sse_connect()
   */
  void (*on_sse_disconnect)(void* user_data, void* client_handle);

  /**
   * @brief Called to send data to a specific SSE client
   *
   * Send data to individual SSE client. Typically used for per-client
   * responses or targeted messages.
   *
   * @param user_data User data pointer
   * @param client_handle Client handle from on_sse_connect()
   * @param data Data to send
   * @param len Length of data in bytes
   * @return true if sent successfully, false on failure
   */
  bool (*on_sse_send)(void* user_data, void* client_handle, const char* data, size_t len);

} mcp_transport_callbacks_t;

// Transport interface (vtable)
/**
 * @brief Transport interface (vtable)
 *
 * Defines the interface that all transport implementations must provide.
 * This allows swapping between different HTTP/SSE implementations
 * (uWebSockets, libuv, ASIO, etc.) without changing server code.
 *
 * @note This is a function pointer table (vtable pattern)
 * @note All implementations must provide all functions
 */
typedef struct {
  uint32_t version;  ///< Interface version (currently 1)

  /**
   * @brief Transport name
   *
   * Human-readable name for logging and debugging (e.g., "sse-uws").
   */
  const char* name;

  /**
   * @brief Create transport instance
   *
   * Create and initialize a new transport instance with specified
   * configuration.
   *
   * @param port TCP port to listen on
   * @param callbacks Callback functions for HTTP/SSE events
   * @param user_data User data passed to callbacks
   * @return Transport handle on success, NULL on failure
   */
  mcp_transport_t* (*create)(uint16_t port, const mcp_transport_callbacks_t* callbacks,
                             void* user_data);

  /**
   * @brief Destroy transport instance
   *
   * Stop transport and free all resources. After this call, the
   * transport handle becomes invalid.
   *
   * @param transport Transport handle
   */
  void (*destroy)(mcp_transport_t* transport);

  // Lifecycle
  /**
   * @brief Start transport
   *
   * Begin accepting connections on the configured port.
   *
   * @param transport Transport handle
   * @return true if started successfully, false on failure
   */
  bool (*start)(mcp_transport_t* transport);

  /**
   * @brief Stop transport
   *
   * Stop accepting new connections and gracefully shutdown.
   * Existing connections are closed.
   *
   * @param transport Transport handle
   */
  void (*stop)(mcp_transport_t* transport);

  /**
   * @brief Check if transport is running
   *
   * Returns true if transport is actively accepting connections.
   *
   * @param transport Transport handle
   * @return true if running, false otherwise
   */
  bool (*is_running)(const mcp_transport_t* transport);

  /**
   * @brief Broadcast to all connected SSE clients
   *
   * Send data to all currently connected SSE clients.
   * Used to push real-time updates to all connected agents.
   *
   * @param transport Transport handle
   * @param data Data to broadcast
   * @param len Length of data in bytes
   */
  void (*broadcast)(mcp_transport_t* transport, const char* data, size_t len);

  /**
   * @brief Get number of connected clients
   *
   * Return count of currently connected SSE clients.
   *
   * @param transport Transport handle
   * @return Number of connected clients
   */
  size_t (*get_client_count)(const mcp_transport_t* transport);

} mcp_transport_interface_t;

// SSE transport over HTTP (uWebSockets implementation)
/**
 * @brief SSE transport over HTTP (uWebSockets implementation)
 *
 * Default transport implementation using uWebSockets library for
 * HTTP and Server-Sent Events (SSE). This is the built-in
 * transport used by default server configuration.
 *
 * @note Supports HTTP/1.1 and SSE protocol
 * @note Non-blocking, event-driven architecture
 */
extern const mcp_transport_interface_t mcp_sse_transport;

#ifdef __cplusplus
}
#endif
