#pragma once

#include "mcp/generic/export.h"
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Opaque Server Handle
// ============================================================================

/**
 * @brief Opaque handle to MCP server instance
 *
 * This handle is returned by mcp_server_create() and used by all server
 * operations. The internal structure is opaque to maintain ABI stability.
 */
typedef struct mcp_server_s mcp_server_t;

/**
 * @brief HTTP route handler callback type
 *
 * Called for registered HTTP routes. Handlers can inspect request metadata,
 * parse the optional body, and write a response payload and status.
 *
 * @param user_data User data pointer passed during registration
 * @param method HTTP method (for example: "GET", "POST")
 * @param path Request path (for example: "/game/state")
 * @param body Request body as JSON/text when present (NULL for bodyless requests)
 * @param response_buffer Output buffer for response body
 * @param response_size Size of response buffer in bytes
 * @param http_status OUT: HTTP status code to return
 * @return true if the route was handled, false to fallback to default handling
 */
typedef bool (*mcp_route_handler_t)(void* user_data, const char* method, const char* path,
                                    const char* body, char* response_buffer, size_t response_size,
                                    int* http_status);

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * @brief Create a new MCP server
 *
 * Creates and initializes a new MCP server with the specified configuration.
 * The server starts listening on the configured port and begins accepting
 * connections.
 *
 * @param config Server configuration (port, logging, rate limiting)
 * @return Server handle on success, NULL on failure
 *
 * @note The caller owns the returned handle and must destroy it with
 *       mcp_server_destroy() when done.
 *
 * @warning If server fails to start (e.g., port already in use), NULL is
 *          returned and no resources are leaked.
 *
 * Example:
 * @code
 * mcp_server_config_t config = mcp_default_config();
 * config.port = 6060;
 * mcp_server_t* server = mcp_server_create(&config);
 * if (!server) {
 *     fprintf(stderr, "Failed to create server\n");
 *     return -1;
 * }
 * // ... use server ...
 * mcp_server_destroy(server);
 * @endcode
 */
MCP_API mcp_server_t* mcp_server_create(const mcp_server_config_t* config);

/**
 * @brief Destroy server and free all resources
 *
 * Stops the server, closes all client connections, and frees all associated
 * resources. After this call, the server handle becomes invalid.
 *
 * @param server Server handle (may be NULL, safe to call)
 *
 * @note Thread-safe - can be called from any thread
 * @note After this call, do not use the server handle
 *
 * Example:
 * @code
 * mcp_server_t* server = mcp_server_create(&config);
 * // ... use server ...
 * mcp_server_destroy(server);  // Clean up
 * server = NULL;  // Avoid dangling pointer
 * @endcode
 */
MCP_API void mcp_server_destroy(mcp_server_t* server);

/**
 * @brief Check if server is running
 *
 * Returns true if the server is currently accepting connections and processing
 * requests. Returns false if the server has been stopped or is in the
 * process of shutting down.
 *
 * @param server Server handle
 * @return true if server is running, false otherwise
 *
 * @note Returns false if server handle is NULL
 *
 * Example:
 * @code
 * if (mcp_server_is_running(server)) {
 *     printf("Server is active\n");
 * } else {
 *     printf("Server is not running\n");
 * }
 * @endcode
 */
MCP_API bool mcp_server_is_running(const mcp_server_t* server);

// ============================================================================
// Route Registration
// ============================================================================

/**
 * @brief Register an HTTP route handler
 *
 * Registers a handler for a concrete HTTP method/path pair.
 *
 * @param server Server handle
 * @param method HTTP method (for example: "GET", "POST")
 * @param path Route path (for example: "/game/state")
 * @param handler Route handler callback
 * @param user_data User data passed to handler
 * @return Result struct with code and message
 */
MCP_API mcp_result_t mcp_server_route_register(mcp_server_t* server, const char* method,
                                               const char* path, mcp_route_handler_t handler,
                                               void* user_data);

/**
 * @brief Unregister an HTTP route handler
 *
 * Removes a previously registered route handler for a method/path pair.
 *
 * @param server Server handle
 * @param method HTTP method
 * @param path Route path
 */
MCP_API void mcp_server_route_unregister(mcp_server_t* server, const char* method,
                                         const char* path);

// ============================================================================
// Method Registration
// ============================================================================

/**
 * @brief Register a handler for a JSON-RPC method
 *
 * Registers a handler function that will be called when clients invoke the
 * specified JSON-RPC method. Multiple handlers can be registered for different
 * method names.
 *
 * Common method names:
 * - "initialize" - MCP protocol initialization
 * - "tools/list" - List available tools
 * - "tools/call" - Execute a tool
 * - "resources/list" - List available resources
 *
 * @param server Server handle
 * @param method Method name (e.g., "tools/call")
 * @param handler Handler function to call when method is invoked
 * @param user_data User data passed to handler (may be NULL)
 * @return Result struct with code and message
 *
 * @note Thread-safe - can be called from any thread
 * @note If a method with the same name is already registered, it is
 *       replaced with the new handler.
 *
 * Breaking Changes:
 * - Function renamed from mcp_server_register_method to
 * mcp_server_method_register (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * mcp_server_register_method(server, "tools/call", handler, data);
 *
 * // After
 * mcp_server_method_register(server, "tools/call", handler, data);
 * @endcode
 *
 * Example:
 * @code
 * bool OnToolCall(void* user, const char* method, const char* params,
 *                char* response, size_t response_size) {
 *     snprintf(response, response_size, "{\"result\":\"ok\"}");
 *     return true;
 * }
 *
 * mcp_result_t result = mcp_server_method_register(server, "tools/call",
 *                                                OnToolCall, NULL);
 * if (result.code != MCP_RESULT_CODE_OK) {
 *     fprintf(stderr, "Registration failed: %s\n", result.message);
 * }
 * @endcode
 */
MCP_API mcp_result_t mcp_server_method_register(mcp_server_t* server, const char* method,
                                                mcp_method_handler_t handler, void* user_data);

/**
 * @brief Unregister a method handler
 *
 * Removes a previously registered method handler. Clients will no longer be
 * able to invoke this method.
 *
 * @param server Server handle
 * @param method Method name to unregister
 *
 * @note Thread-safe - can be called from any thread
 * @note Does nothing if method is not registered
 *
 * Example:
 * @code
 * mcp_server_method_register(server, "temporary_tool", handler, NULL);
 * // ... use it ...
 * mcp_server_method_unregister(server, "temporary_tool");
 * @endcode
 */
MCP_API void mcp_server_method_unregister(mcp_server_t* server, const char* method);

/**
 * @brief Register multiple methods atomically with a single mutex lock
 *
 * Registers multiple method handlers in a single atomic operation. This is
 * more efficient than calling mcp_server_method_register() multiple times
 * because it only acquires the mutex once.
 *
 * @param server Server handle
 * @param methods Array of method registrations
 * @param count Number of methods to register
 * @return Result struct with code and message
 *
 * @note Thread-safe - acquires mutex once for entire batch
 * @note If any registration fails, entire batch fails (atomic semantics)
 * @note Allows NULL methods array when count is 0
 *
 * Example:
 * @code
 * mcp_method_registration_t methods[] = {
 *     {"tools/list", OnListTools, NULL},
 *     {"tools/call", OnCallTool, NULL},
 *     {"resources/list", OnListResources, NULL}
 * };
 *
 * mcp_result_t result = mcp_server_methods_register(server, methods, 3);
 * if (result.code != MCP_RESULT_CODE_OK) {
 *     fprintf(stderr, "Batch registration failed: %s\n", result.message);
 * }
 * @endcode
 */
typedef struct {
  const char*          method;     ///< Method name (e.g., "tools/call")
  mcp_method_handler_t handler;    ///< Handler function
  void*                user_data;  ///< User data passed to handler
} mcp_method_registration_t;

MCP_API mcp_result_t mcp_server_methods_register(mcp_server_t*                    server,
                                                 const mcp_method_registration_t* methods,
                                                 size_t                           count);

// ============================================================================
// Event Broadcasting (SSE)
// ============================================================================

/**
 * @brief Broadcast an event to all connected SSE clients
 *
 * Sends an event message to all currently connected Server-Sent Events (SSE)
 * clients. Events are typically used to push real-time state updates like
 * game state changes or notifications.
 *
 * Common event types:
 * - "state" - Game state snapshot
 * - "screenshot" - Screenshot available notification
 * - "notification" - General notifications
 *
 * @param server Server handle
 * @param event_type Event type string
 * @param json_payload JSON data as null-terminated string
 *
 * @note Thread-safe - can be called from any thread
 * @note If no clients are connected, the event is silently dropped
 * @note The json_payload must be valid JSON (not validated by this function)
 *
 * Breaking Changes:
 * - Function renamed from mcp_server_broadcast to mcp_server_event_broadcast
 * (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * mcp_server_broadcast(server, "state", "{\"hp\":100}");
 *
 * // After
 * mcp_server_event_broadcast(server, "state", "{\"hp\":100}");
 * @endcode
 *
 * Example:
 * @code
 * // In game loop, broadcast state to all clients
 * char json[256];
 * snprintf(json, sizeof(json), "{\"hp\":%d,\"ammo\":%d}",
 *          player->health, player->ammo);
 * mcp_server_event_broadcast(server, "state", json);
 * @endcode
 */
MCP_API void mcp_server_event_broadcast(mcp_server_t* server, const char* event_type,
                                        const char* json_payload);

/**
 * @brief Get the number of connected clients
 *
 * Returns the current count of connected SSE (Server-Sent Events) clients.
 * This is useful for monitoring server activity or adjusting update rates.
 *
 * @param server Server handle
 * @return Number of connected clients (0 if server is NULL or stopped)
 *
 * @note Thread-safe - can be called from any thread
 * @note Returns actual count, not a boolean like the old API
 *
 * Breaking Changes:
 * - Function renamed from mcp_server_has_clients to mcp_server_clients_count
 * (v0.5.0)
 * - Return type changed from bool to uint64_t for more information
 *
 * Migration:
 * @code
 * // Before (boolean)
 * if (mcp_server_has_clients(server)) { ... }
 *
 * // After (count)
 * uint64_t count = mcp_server_clients_count(server);
 * if (count > 0) { ... }
 * @endcode
 *
 * Example:
 * @code
 * uint64_t clients = mcp_server_clients_count(server);
 * if (clients == 0) {
 *     // No clients, can reduce update frequency
 *     update_interval_ms = 1000;
 * } else {
 *     update_interval_ms = 100;
 * }
 * @endcode
 */
MCP_API uint64_t mcp_server_clients_count(const mcp_server_t* server);

// ============================================================================
// Statistics
// ============================================================================

/**
 * @brief Get server statistics
 *
 * Retrieves cumulative statistics about server operation, including request
 * counts, client connections, and data transfer metrics. Useful for
 * monitoring and debugging.
 *
 * @param server Server handle
 * @param stats Output structure to receive statistics
 *
 * @note Thread-safe - can be called from any thread
 * @note Stats are cumulative since server creation
 *
 * Breaking Changes:
 * - Function renamed from mcp_server_get_stats to mcp_server_stats_get (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * mcp_server_stats_t stats;
 * mcp_server_get_stats(server, &stats);
 *
 * // After (same pattern, just renamed)
 * mcp_server_stats_t stats;
 * mcp_server_stats_get(server, &stats);
 * @endcode
 *
 * Example:
 * @code
 * mcp_server_stats_t stats;
 * mcp_server_stats_get(server, &stats);
 * printf("Clients: %llu\n", stats.connected_clients);
 * printf("Requests: %llu\n", stats.requests_handled);
 * printf("Failed: %llu\n", stats.requests_failed);
 * @endcode
 */
MCP_API void mcp_server_stats_get(const mcp_server_t* server, mcp_server_stats_t* stats);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Helper to create a JSON-RPC 2.0 success response
 *
 * Formats a JSON-RPC 2.0 success response with the specified ID and
 * result data. This is a utility function for method handlers that need to
 * format responses.
 *
 * @param buffer Output buffer for response string
 * @param buffer_size Size of output buffer
 * @param id Request ID string (from incoming request)
 * @param result_json JSON data for "result" field
 * @return Number of bytes written (excluding null terminator), or 0 on error
 *
 * @note The response is null-terminated
 * @note Returns 0 if buffer is too small or inputs are invalid
 *
 * Example:
 * @code
 * char response[1024];
 * size_t written = mcp_format_success_response(response, sizeof(response),
 *                                            "123", "{\"value\":42}");
 * // response = {"jsonrpc":"2.0","id":"123","result":{"value":42}}
 * @endcode
 */
MCP_API size_t mcp_format_success_response(char* buffer, size_t buffer_size, const char* id,
                                           const char* result_json);

/**
 * @brief Helper to create a JSON-RPC 2.0 error response
 *
 * Formats a JSON-RPC 2.0 error response with the specified ID, error
 * code, and message. This is a utility function for method handlers that
 * need to return errors.
 *
 * @param buffer Output buffer for response string
 * @param buffer_size Size of output buffer
 * @param id Request ID string (from incoming request)
 * @param error_code JSON-RPC error code (typically -32600 to -32703)
 * @param error_message Human-readable error description
 * @return Number of bytes written (excluding null terminator), or 0 on error
 *
 * @note The response is null-terminated
 * @note Returns 0 if buffer is too small or inputs are invalid
 *
 * Example:
 * @code
 * char response[1024];
 * size_t written = mcp_format_error_response(response, sizeof(response),
 *                                          "123", -32601, "Method not found");
 * // response =
 * {"jsonrpc":"2.0","id":"123","error":{"code":-32601,"message":"Method not
 * found"}}
 * @endcode
 */
MCP_API size_t mcp_format_error_response(char* buffer, size_t buffer_size, const char* id,
                                         int error_code, const char* error_message);

#ifdef __cplusplus
}
#endif
