#pragma once

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
#define MCP_PROTOCOL_VERSION "2025-06-18"
#define MCP_SERVER_NAME      "doom-mcp"
#define MCP_SERVER_VERSION   "0.3.0"

// ============================================================================
// Result Codes
// ============================================================================

typedef mcp_result_generic_t mcp_result_t;

// Result code constants (for comparing .code field)
#define MCP_RESULT_CODE_OK              0
#define MCP_RESULT_CODE_INVALID_ARGS    -1
#define MCP_RESULT_CODE_ENCODING_FAILED -2
#define MCP_RESULT_CODE_DISABLED        -3
#define MCP_RESULT_CODE_QUEUE_FULL      -4
#define MCP_RESULT_CODE_NOT_FOUND       -5
#define MCP_RESULT_CODE_INTERNAL        -6

// Convenience macros for creating results
#define MCP_OK MCP_RESULT_OK("Success")
#define MCP_ERROR_INVALID_ARGS \
  MCP_RESULT_ERROR(MCP_RESULT_CODE_INVALID_ARGS, "Invalid arguments")
#define MCP_ERROR_ENCODING_FAILED \
  MCP_RESULT_ERROR(MCP_RESULT_CODE_ENCODING_FAILED, "Encoding failed")
#define MCP_ERROR_DISABLED \
  MCP_RESULT_ERROR(MCP_RESULT_CODE_DISABLED, "Operation disabled")
#define MCP_ERROR_QUEUE_FULL \
  MCP_RESULT_ERROR(MCP_RESULT_CODE_QUEUE_FULL, "Queue full")
#define MCP_ERROR_NOT_FOUND \
  MCP_RESULT_ERROR(MCP_RESULT_CODE_NOT_FOUND, "Not found")
#define MCP_ERROR_INTERNAL \
  MCP_RESULT_ERROR(MCP_RESULT_CODE_INTERNAL, "Internal error")

// ============================================================================
// Log Levels
// ============================================================================
typedef enum {
  MCP_LOG_DEBUG = 0,
  MCP_LOG_INFO  = 1,
  MCP_LOG_WARN  = 2,
  MCP_LOG_ERROR = 3
} mcp_log_level_t;

// ============================================================================
// Callbacks
// ============================================================================

// Logging callback
typedef void (*mcp_log_callback_t)(void* user_data, int level,
                                   const char* message);

// Handler for JSON-RPC method calls
// request_json: null-terminated JSON string (the 'params' field)
// response_buffer: buffer to write response JSON (null-terminated)
// response_size: size of response buffer
// Returns: true if handled, false if not handled
typedef bool (*mcp_method_handler_t)(void* user_data, const char* method,
                                     const char* request_json,
                                     char*       response_buffer,
                                     size_t      response_size);

// ============================================================================
// Server Configuration
// ============================================================================
typedef struct {
  uint32_t struct_size;  // Set to sizeof(mcp_server_config_t)

  // Network
  uint16_t port;

  // Rate limiting
  uint32_t max_requests_per_second;
  size_t   max_payload_size;

  // Logging
  mcp_log_callback_t on_log;
  void*              log_user_data;

} mcp_server_config_t;

// ============================================================================
// Server Statistics
// ============================================================================
typedef struct {
  uint64_t connected_clients;
  uint64_t requests_handled;
  uint64_t requests_failed;
  uint64_t events_broadcast;
  uint64_t bytes_sent;
} mcp_server_stats_t;

// ============================================================================
// Default Configuration
// ============================================================================
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

#ifdef __cplusplus
}
#endif
