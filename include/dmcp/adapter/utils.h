#pragma once

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DMCP_FRACUNIT 65536

static inline int dmcp_str_equals_ci(const char* a, const char* b) {
  if (!a || !b) return 0;
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
      return 0;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

static inline float dmcp_fixed_to_float(int32_t fixed_val) {
  return (float)fixed_val / (float)DMCP_FRACUNIT;
}

static inline int32_t dmcp_float_to_fixed(float val) {
  return (int32_t)(val * (float)DMCP_FRACUNIT);
}

static inline float dmcp_angle_to_degrees(uint32_t angle) {
  return (float)angle * 360.0f / 4294967296.0f;
}

static inline uint32_t dmcp_degrees_to_angle(float degrees) {
  return (uint32_t)(degrees * 4294967296.0f / 360.0f);
}

static inline float dmcp_angle_to_radians(uint32_t angle) {
  return (float)angle * 6.28318530717958647693f / 4294967296.0f;
}

static inline uint32_t dmcp_radians_to_angle(float radians) {
  return (uint32_t)(radians * 4294967296.0f / 6.28318530717958647693f);
}

#ifdef __cplusplus
}
#endif
