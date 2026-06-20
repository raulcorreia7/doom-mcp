#pragma once

#include "mcp/core/export.h"
#include "mcp/core/version_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Return the linked MCP core foundation semantic version string.
 */
MCP_CORE_API const char* mcp_core_version_string(void);

/**
 * Return the linked MCP core foundation semantic version components.
 */
MCP_CORE_API uint32_t mcp_core_version_major(void);
MCP_CORE_API uint32_t mcp_core_version_minor(void);
MCP_CORE_API uint32_t mcp_core_version_patch(void);

#ifdef __cplusplus
}
#endif
