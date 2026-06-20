#include "mcp/core/status.h"

extern "C" {

mcp_status_t mcp_status_ok(void) { return MCP_STATUS_OK("Success"); }

mcp_status_t mcp_status_error(int32_t code, const char* message) {
  return MCP_STATUS_ERROR(code, message);
}
}
