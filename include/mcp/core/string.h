#pragma once

#include "mcp/core/export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compare two null-terminated strings case-insensitively for ASCII identifiers.
 *
 * This intentionally does not use locale-sensitive rules. It is suitable for
 * protocol tokens, command names, environment values, and engine console text.
 * Public content IDs should still use exact matching when a canonical API
 * spelling is required.
 *
 * NULL is ordered before non-NULL. Two NULL pointers compare equal.
 */
MCP_CORE_API int mcp_strcmp_ci(const char* lhs, const char* rhs);

#ifdef __cplusplus
}
#endif
