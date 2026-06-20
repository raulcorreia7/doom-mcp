#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Copy a byte range when the destination capacity is known.
 *
 * Returns true when src_size bytes were copied. Returns false for NULL buffers
 * or when src_size exceeds dest_size. A zero-byte copy succeeds without touching
 * either pointer.
 */
static inline bool mcp_memcpy_safe(void* dest, size_t dest_size, const void* src, size_t src_size) {
  if (src_size == 0) {
    return true;
  }
  if (!dest || !src || src_size > dest_size) {
    return false;
  }

  memcpy(dest, src, src_size);
  return true;
}

#ifdef __cplusplus
}
#endif
