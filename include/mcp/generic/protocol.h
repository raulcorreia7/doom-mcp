#pragma once

#include "mcp/generic/export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mcp/generic/constants.h"
#include "mcp/generic/result.h"

// ============================================================================
// MCP Protocol Version
// ============================================================================

/**
 * @brief MCP protocol version
 *
 * This string identifies the MCP protocol version this implementation
 * follows. Used in protocol handshake and for compatibility checking.
 */
#define MCP_PROTOCOL_VERSION "2025-03-26"

/**
 * @brief Server identification name
 *
 * Used in protocol metadata to identify this server implementation.
 */
#define MCP_SERVER_NAME "doom-mcp"

/**
 * @brief Server version string
 *
 * Version follows semantic versioning (MAJOR.MINOR.PATCH).
 * Updated with each release.
 */
#define MCP_SERVER_VERSION "0.6.0"

// ============================================================================
// Result Codes
// ============================================================================

/**
 * @brief Generic MCP result type
 *
 * @note Breaking Change (v0.5.0): Changed from enum to struct with code/message
 * fields.
 * @see mcp_result_generic_t for full documentation
 */
typedef mcp_result_generic_t mcp_result_t;

/**
 * @brief Result code constants
 *
 * These constants are used with the .code field of result structs.
 * Compare result.code against these values to check operation success.
 *
 * Values:
 * - 0 = Success
 * - Negative = Error codes (defined below)
 * - Positive = Reserved for future use
 */
#define MCP_RESULT_CODE_OK 0
#define MCP_RESULT_CODE_INVALID_ARGS -1
#define MCP_RESULT_CODE_ENCODING_FAILED -2
#define MCP_RESULT_CODE_DISABLED -3
#define MCP_RESULT_CODE_QUEUE_FULL -4
#define MCP_RESULT_CODE_NOT_FOUND -5
#define MCP_RESULT_CODE_INTERNAL -6

/**
 * @brief Convenience macros for creating results
 *
 * These macros create pre-configured result structs for common cases.
 * Use them when returning errors or success.
 */

/** @brief Success result with "Success" message */
#define MCP_OK MCP_RESULT_OK("Success")

/** @brief Invalid arguments error */
#define MCP_ERROR_INVALID_ARGS MCP_RESULT_ERROR(MCP_RESULT_CODE_INVALID_ARGS, "Invalid arguments")

/** @brief JSON encoding/decoding failed */
#define MCP_ERROR_ENCODING_FAILED \
  MCP_RESULT_ERROR(MCP_RESULT_CODE_ENCODING_FAILED, "Encoding failed")

/** @brief Operation is disabled (e.g., screenshots) */
#define MCP_ERROR_DISABLED MCP_RESULT_ERROR(MCP_RESULT_CODE_DISABLED, "Operation disabled")

/** @brief Queue is full, cannot add more items */
#define MCP_ERROR_QUEUE_FULL MCP_RESULT_ERROR(MCP_RESULT_CODE_QUEUE_FULL, "Queue full")

/** @brief Requested resource or method not found */
#define MCP_ERROR_NOT_FOUND MCP_RESULT_ERROR(MCP_RESULT_CODE_NOT_FOUND, "Not found")

/** @brief Internal error occurred */
#define MCP_ERROR_INTERNAL MCP_RESULT_ERROR(MCP_RESULT_CODE_INTERNAL, "Internal error")

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
 * mcp_server_config_t cfg = mcp_default_config();
 * cfg.on_log = OnLog;
 * @endcode
 */
typedef void (*mcp_log_callback_t)(void* user_data, int level, const char* message);

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
 * and logging. Use mcp_default_config() to get sensible defaults.
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

} mcp_server_config_t;

/**
 * @brief Create default server configuration
 *
 * Returns a configuration structure with sensible defaults:
 * - Port: 6060
 * - Max requests/sec: 100
 * - Max payload: 1MB
 * - Logging: disabled
 *
 * @return Configuration struct with defaults set
 *
 * Example:
 * @code
 * mcp_server_config_t config = mcp_default_config();
 * config.port = 8080;  // Override default port
 * mcp_server_t* server = mcp_server_create(&config);
 * @endcode
 */
static inline mcp_server_config_t mcp_default_config(void) {
  mcp_server_config_t cfg     = {};
  cfg.struct_size             = sizeof(mcp_server_config_t);
  cfg.port                    = MCP_DEFAULT_PORT;
  cfg.max_requests_per_second = 100;
  cfg.max_payload_size        = MCP_MAX_PAYLOAD_SIZE;
  cfg.on_log                  = NULL;
  cfg.log_user_data           = NULL;
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
