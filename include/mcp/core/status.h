#pragma once

#include "mcp/core/export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * Shared status contract for public MCP and DMCP C APIs.
 *
 * The message pointer is owned by the library or caller that produced the
 * status. It is treated as a borrowed, null-terminated string.
 */
typedef struct {
  int32_t     code;
  const char* message;
} mcp_status_t;

#define MCP_STATUS_CODE_OK 0
#define MCP_STATUS_CODE_INVALID_ARGS -1
#define MCP_STATUS_CODE_ENCODING_FAILED -2
#define MCP_STATUS_CODE_DISABLED -3
#define MCP_STATUS_CODE_QUEUE_FULL -4
#define MCP_STATUS_CODE_NOT_FOUND -5
#define MCP_STATUS_CODE_INTERNAL -6

static inline mcp_status_t mcp_status_make(int32_t code, const char* message) {
  mcp_status_t status;
  status.code    = code;
  status.message = message;
  return status;
}

#define MCP_STATUS_MAKE(code, msg) mcp_status_make((code), (msg))

#define MCP_STATUS_OK(message) MCP_STATUS_MAKE(0, (message) ? (message) : "Success")
#define MCP_STATUS_ERROR(code, message) \
  MCP_STATUS_MAKE((code), (message) ? (message) : "Unknown error")

/**
 * Return a canonical success status.
 */
MCP_CORE_API mcp_status_t mcp_status_ok(void);

/**
 * Return a status with the provided code and message.
 */
MCP_CORE_API mcp_status_t mcp_status_error(int32_t code, const char* message);

/**
 * Return true when the status code represents success.
 */
static inline bool mcp_status_is_ok(mcp_status_t status) {
  return status.code == MCP_STATUS_CODE_OK;
}

#ifdef __cplusplus
}
#endif
