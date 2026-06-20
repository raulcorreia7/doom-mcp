#pragma once

#include <stdbool.h>

#include "dmcp/doom/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline bool dmcp_validate_health(int health) {
  return health > 0 && health <= DMCP_PLAYER_MAX_HEALTH;
}

static inline bool dmcp_validate_health_with_max(int health, int max_health) {
  return health > 0 && health <= max_health;
}

static inline bool dmcp_validate_armor(int armor) {
  return armor >= 0 && armor <= DMCP_PLAYER_MAX_ARMOR;
}

static inline bool dmcp_validate_skill(int skill) {
  return skill >= DMCP_SKILL_MIN && skill <= DMCP_SKILL_MAX;
}

static inline bool dmcp_validate_position_2d(float x, float y, float min_x, float max_x,
                                             float min_y, float max_y) {
  return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
}

static inline bool dmcp_validate_angle(float angle) { return angle >= 0.0f && angle < 360.0f; }

static inline bool dmcp_validate_item_amount(int amount) { return amount >= 1 && amount <= 1000; }

static inline bool dmcp_validate_tid(int tid) { return tid >= DMCP_TID_MIN && tid <= DMCP_TID_MAX; }

static inline bool dmcp_validate_damage(int damage) {
  return damage >= DMCP_DAMAGE_MIN && damage <= DMCP_DAMAGE_MAX;
}

static inline bool dmcp_validate_ammo(int ammo) {
  return ammo >= 0 && ammo <= DMCP_AMMO_MAX_DEFAULT;
}

#ifdef __cplusplus
}
#endif
