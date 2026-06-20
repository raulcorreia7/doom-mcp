#pragma once

#include "mcp/core/export.h"
#include "mcp/core/status.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MCP_CORE_API_VERSION_MAJOR 1
#define MCP_CORE_API_VERSION_MINOR 0
#define MCP_CORE_API_VERSION_PATCH 0
#define MCP_CORE_API_VERSION                                                     \
  ((MCP_CORE_API_VERSION_MAJOR * 10000u) + (MCP_CORE_API_VERSION_MINOR * 100u) + \
   MCP_CORE_API_VERSION_PATCH)

/**
 * Runtime public API description for compatibility checks.
 *
 * Consumers must initialize struct_size to sizeof(mcp_core_api_info_t) when
 * passing API-owned structures in extension points.
 */
typedef struct {
  uint32_t struct_size;
  uint32_t api_version;
  uint32_t status_size;
  uint32_t reserved;
} mcp_core_api_info_t;

/**
 * Fill out API metadata for the linked MCP core foundation library.
 */
MCP_CORE_API void mcp_core_api_get(mcp_core_api_info_t* out_info);

#ifdef __cplusplus
}
#endif
