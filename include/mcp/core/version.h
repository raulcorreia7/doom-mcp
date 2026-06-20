#pragma once

#include "mcp/core/export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef MCP_CORE_VERSION_MAJOR
#define MCP_CORE_VERSION_MAJOR 0
#endif
#ifndef MCP_CORE_VERSION_MINOR
#define MCP_CORE_VERSION_MINOR 6
#endif
#ifndef MCP_CORE_VERSION_PATCH
#define MCP_CORE_VERSION_PATCH 0
#endif
#ifndef MCP_CORE_VERSION
#define MCP_CORE_VERSION "0.6.0"
#endif

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
