#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Generic Result Type
// ============================================================================

// Generic result struct with error code and message
// All result types should include this struct and use macros for convenience
typedef struct {
  int32_t     code;     // Error code (0 = success, negative = error)
  const char* message;  // Static error message string (thread-local if needed)
} mcp_result_generic_t;

// Helper macro to create a result with code and message
#define MCP_RESULT_MAKE(code, msg) ((mcp_result_generic_t){(code), (msg)})

// Helper macro to create success result
#define MCP_RESULT_OK(message) \
  MCP_RESULT_MAKE(0, (message) ? (message) : "Success")

// Helper macro to create error result
#define MCP_RESULT_ERROR(code, message) \
  MCP_RESULT_MAKE((code), (message) ? (message) : "Unknown error")

#ifdef __cplusplus
}
#endif
