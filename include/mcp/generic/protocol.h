#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// MCP Protocol Version
// ============================================================================
#define MCP_PROTOCOL_VERSION "2025-06-18"
#define MCP_SERVER_NAME      "doom-mcp"
#define MCP_SERVER_VERSION   "0.3.0"

// ============================================================================
// Result Codes
// ============================================================================
typedef enum {
  MCP_OK                    = 0,
  MCP_ERROR_INVALID_ARGS    = -1,
  MCP_ERROR_ENCODING_FAILED = -2,
  MCP_ERROR_DISABLED        = -3,
  MCP_ERROR_QUEUE_FULL      = -4,
  MCP_ERROR_NOT_FOUND       = -5,
  MCP_ERROR_INTERNAL        = -6,
} mcp_result_t;

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
typedef bool (*mcp_method_handler_t)(void* user_data, 
                                     const char* method,
                                     const char* request_json,
                                     char* response_buffer,
                                     size_t response_size);

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
  mcp_server_config_t cfg = {};
  cfg.struct_size         = sizeof(mcp_server_config_t);
  cfg.port                = 6060;
  cfg.max_requests_per_second = 100;
  cfg.max_payload_size    = 1024 * 1024;  // 1MB
  cfg.on_log              = NULL;
  cfg.log_user_data       = NULL;
  return cfg;
}

#ifdef __cplusplus
}
#endif
