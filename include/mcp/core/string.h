#pragma once

#include "mcp/core/export.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Copy a null-terminated string into a fixed-size C buffer.
 *
 * This is the project-wide bounded string copy helper for public C APIs and
 * adapter code. It always null-terminates dest when dest is non-NULL and
 * dest_size is non-zero.
 *
 * Returns true when the full source string was copied. Returns false for NULL
 * inputs, zero-size destinations, or truncation.
 */
static inline bool mcp_strcpy_safe(char* dest, size_t dest_size, const char* src) {
  size_t i;

  if (!dest || dest_size == 0) {
    return false;
  }
  if (!src) {
    dest[0] = '\0';
    return false;
  }

  i = 0;
  while (i < dest_size - 1 && src[i] != '\0') {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';

  return src[i] == '\0';
}

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
