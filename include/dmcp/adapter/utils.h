#pragma once

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DMCP_FRACUNIT 65536

static inline int dmcp_clamp_int(int value, int min_value, int max_value) {
  if (value < min_value) return min_value;
  if (value > max_value) return max_value;
  return value;
}

static inline void dmcp_strcpy_safe(char* dest, const char* src, size_t dest_size) {
  if (!dest || dest_size == 0) return;
  if (!src) {
    dest[0] = '\0';
    return;
  }
  strncpy(dest, src, dest_size - 1);
  dest[dest_size - 1] = '\0';
}

static inline bool dmcp_str_equals_ci(const char* a, const char* b) {
  if (!a || !b) return false;
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

static inline float dmcp_fixed_to_float_safe(int32_t fixed_val) {
  return (float)fixed_val / (float)DMCP_FRACUNIT;
}

static inline int32_t dmcp_float_to_fixed_safe(float val) {
  float result;
  if (!isfinite(val)) return 0;
  result = val * (float)DMCP_FRACUNIT;
  if (result > (float)INT_MAX) return (int32_t)INT_MAX;
  if (result < (float)INT_MIN) return (int32_t)INT_MIN;
  return (int32_t)result;
}

static inline float dmcp_angle_to_degrees_safe(uint32_t angle) {
  return (float)angle * 360.0f / 4294967296.0f;
}

static inline uint32_t dmcp_degrees_to_angle_safe(float degrees) {
  float result;
  if (!isfinite(degrees)) return 0;
  result = degrees * (4294967296.0f / 360.0f);
  if (result < 0.0f) return 0;
  if (result > (float)UINT_MAX) return UINT_MAX;
  return (uint32_t)result;
}

static inline float dmcp_angle_to_radians_safe(uint32_t angle) {
  return (float)angle * 6.28318530717958647693f / 4294967296.0f;
}

static inline uint32_t dmcp_radians_to_angle_safe(float radians) {
  float result;
  if (!isfinite(radians)) return 0;
  result = radians * (4294967296.0f / 6.28318530717958647693f);
  if (result < 0.0f) return 0;
  if (result > (float)UINT_MAX) return UINT_MAX;
  return (uint32_t)result;
}

#define dmcp_fixed_to_float dmcp_fixed_to_float_safe
#define dmcp_float_to_fixed dmcp_float_to_fixed_safe
#define dmcp_angle_to_degrees dmcp_angle_to_degrees_safe
#define dmcp_degrees_to_angle dmcp_degrees_to_angle_safe
#define dmcp_angle_to_radians dmcp_angle_to_radians_safe
#define dmcp_radians_to_angle dmcp_radians_to_angle_safe

#ifdef __cplusplus
}
#endif
