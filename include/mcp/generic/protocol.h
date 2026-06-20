#pragma once

#include "mcp/generic/export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "mcp/core/string.h"
#include "mcp/core/status.h"
#include "mcp/generic/constants.h"

// ============================================================================
// MCP Protocol Version
// ============================================================================

/**
 * @brief MCP protocol version
 *
 * This string identifies the MCP protocol version this implementation
 * follows. Used in protocol handshake and for compatibility checking.
 */
#define MCP_PROTOCOL_VERSION "2025-11-25"

/**
 * @brief Server version string
 *
 * Version follows semantic versioning (MAJOR.MINOR.PATCH).
 * Updated with each release.
 */
#define MCP_SERVER_VERSION "0.6.0"

// ============================================================================
// Log Levels
// ============================================================================

/**
 * @brief Log level enumeration
 *
 * Defines severity levels for log messages. Higher numbers indicate
 * more severe messages.
 *
 * @note Used by mcp_log_callback_t to categorize log messages
 */
typedef enum {
  MCP_LOG_DEBUG = 0,  ///< Detailed debug information
  MCP_LOG_INFO  = 1,  ///< General informational messages
  MCP_LOG_WARN  = 2,  ///< Warning messages (recoverable issues)
  MCP_LOG_ERROR = 3   ///< Error messages (serious problems)
} mcp_log_level_t;

// ============================================================================
// Callbacks
// ============================================================================

/**
 * @brief Logging callback function type
 *
 * This callback is invoked by the SDK to log messages. Implement it to
 * integrate SDK logging with your application's logging system.
 *
 * @param user_data User data pointer passed during registration
 * @param level Log level (MCP_LOG_DEBUG, MCP_LOG_INFO, etc.)
 * @param message Log message string (null-terminated)
 *
 * @note The message string should not be freed by the callback
 * @note Callback may be called from any thread
 *
 * Example:
 * @code
 * void OnLog(void* user, int level, const char* message) {
 *     const char* level_str = "INFO";
 *     if (level == MCP_LOG_ERROR) level_str = "ERROR";
 *     printf("[%s] %s\n", level_str, message);
 * }
 *
 * mcp_server_config_t cfg = mcp_server_config_default();
 * cfg.on_log = OnLog;
 * @endcode
 */
typedef void (*mcp_log_callback_t)(void* user_data, int level, const char* message);
typedef struct mcp_transport_interface_s mcp_transport_interface_t;

/**
 * @brief JSON-RPC method handler function type
 *
 * This callback is invoked when a client calls a registered JSON-RPC
 * method. The handler should process the request and write a JSON
 * response.
 *
 * @param user_data User data pointer passed during registration
 * @param method Method name being invoked (e.g., "tools/call")
 * @param request_json Request parameters as JSON string (null-terminated)
 * @param response_buffer Output buffer for response JSON (null-terminated)
 * @param response_size Size of response buffer in bytes
 * @return true if request was handled, false if not handled
 *
 * @note Thread-safe - handler may be called concurrently from multiple threads
 * @note The response must be valid JSON-RPC 2.0 format
 * @note Return false to indicate method not implemented or failed
 *
 * Example:
 * @code
 * bool OnToolCall(void* user, const char* method, const char* params,
 *                char* response, size_t response_size) {
 *     // Parse params and execute tool
 *     snprintf(response, response_size, "{\"result\":\"executed\"}");
 *     return true;
 * }
 *
 * mcp_server_method_register(server, "tools/call", OnToolCall, NULL);
 * @endcode
 */
typedef bool (*mcp_method_handler_t)(void* user_data, const char* method, const char* request_json,
                                     char* response_buffer, size_t response_size);

// ============================================================================
// Server Configuration
// ============================================================================

/**
 * @brief Server configuration structure
 *
 * Configures MCP server behavior including network settings, rate limiting,
 * and logging. Use mcp_server_config_default() to get sensible defaults.
 *
 * @note Always set struct_size to sizeof(mcp_server_config_t) before use
 */
typedef struct {
  uint32_t struct_size;  ///< Must be set to sizeof(mcp_server_config_t)

  // Network
  uint16_t port;  ///< TCP port to listen on (default: 6060)

  // Rate limiting
  uint32_t max_requests_per_second;  ///< Max requests per client (default: 100)
  size_t   max_payload_size;         ///< Max request body size in bytes

  // Logging
  mcp_log_callback_t on_log;         ///< Optional log callback (NULL to disable)
  void*              log_user_data;  ///< User data passed to log callback

  // Server identification
  char server_name[64];  ///< Server name for protocol identification (default: "mcp-server")

  // Transport lifecycle
  bool start_transport;  ///< Start HTTP/SSE transport during create (default: true)

  // Optional transport implementation. NULL selects the built-in HTTP/SSE backend.
  const mcp_transport_interface_t* transport;

} mcp_server_config_t;

/**
 * @brief Create default server configuration
 *
 * Returns a configuration structure with sensible defaults:
 * - Port: 6060
 * - Max requests/sec: 100
 * - Max payload: 1MB
 * - Logging: disabled
 * - Transport: started
 *
 * @return Configuration struct with defaults set
 *
 * Example:
 * @code
 * mcp_server_config_t config = mcp_server_config_default();
 * config.port = 8080;  // Override default port
 * mcp_server_t* server = mcp_server_create(&config);
 * @endcode
 */
static inline mcp_server_config_t mcp_server_config_default(void) {
  mcp_server_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.struct_size             = sizeof(mcp_server_config_t);
  cfg.port                    = MCP_DEFAULT_PORT;
  cfg.max_requests_per_second = 100;
  cfg.max_payload_size        = MCP_MAX_PAYLOAD_SIZE;
  cfg.on_log                  = NULL;
  cfg.log_user_data           = NULL;
  mcp_strcpy_safe(cfg.server_name, sizeof(cfg.server_name), "mcp-server");
  cfg.start_transport = true;
  cfg.transport       = NULL;
  return cfg;
}

// ============================================================================
// Server Statistics
// ============================================================================

/**
 * @brief Server statistics structure
 *
 * Contains cumulative statistics about server operation. Useful for monitoring
 * performance, debugging, and understanding usage patterns.
 *
 * @note Stats are reset only when server is destroyed
 * @note All fields are unsigned (0 = no events)
 */
typedef struct {
  size_t   struct_size;        ///< Must be sizeof(mcp_server_stats_t)
  uint64_t connected_clients;  ///< Total client connections (lifetime)
  uint64_t requests_handled;   ///< Total successful requests processed
  uint64_t requests_failed;    ///< Total failed requests
  uint64_t events_broadcast;   ///< Total events sent to clients
  uint64_t bytes_sent;         ///< Total bytes transmitted
} mcp_server_stats_t;

#ifdef __cplusplus
}
#endif
