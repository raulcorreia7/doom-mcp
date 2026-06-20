#include "mcp/core/core.h"

extern "C" {

const char* mcp_core_version_string(void) { return MCP_CORE_VERSION; }

uint32_t mcp_core_version_major(void) { return MCP_CORE_VERSION_MAJOR; }

uint32_t mcp_core_version_minor(void) { return MCP_CORE_VERSION_MINOR; }

uint32_t mcp_core_version_patch(void) { return MCP_CORE_VERSION_PATCH; }
}
