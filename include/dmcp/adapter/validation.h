#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline bool dmcp_validate_health(float health) { return health > 0.0f && health <= 200.0f; }

static inline bool dmcp_validate_health_with_max(float health, float max_health) {
  return health > 0.0f && health <= max_health;
}

static inline bool dmcp_validate_armor(float armor) { return armor >= 0.0f && armor <= 200.0f; }

static inline bool dmcp_validate_timescale(float scale) { return scale > 0.0f && scale <= 10.0f; }

static inline bool dmcp_validate_skill(int skill) { return skill >= 1 && skill <= 5; }

static inline bool dmcp_validate_position_2d(float x, float y, float min_x, float max_x,
                                             float min_y, float max_y) {
  return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
}

static inline bool dmcp_validate_angle(float angle) { return angle >= 0.0f && angle < 360.0f; }

static inline bool dmcp_validate_item_amount(int amount) { return amount >= 1 && amount <= 1000; }

static inline bool dmcp_validate_tid(int tid) { return tid >= 0 && tid <= 32767; }

static inline bool dmcp_validate_damage(float damage) {
  return damage > 0.0f && damage <= 10000.0f;
}

#ifdef __cplusplus
}
#endif
