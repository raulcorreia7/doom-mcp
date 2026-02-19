// Type Mapping Utilities
// Centralizes conversions from Chocolate Doom types to DMCP output

#ifndef DMCP_MAPPINGS_H
#define DMCP_MAPPINGS_H

#include <stddef.h>
#include <string.h>

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"
#include "m_fixed.h"

#include "dmcp/doom/dmcp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DMCP_MAX_STRING 64

static inline float dmcp_fixed_to_float(fixed_t fixed_val) {
  return (float)fixed_val / (float)FRACUNIT;
}

static inline float dmcp_angle_to_radians(angle_t angle_val) {
  return (float)angle_val * (3.14159265358979323846f * 2.0f) / (float)UINT_MAX;
}

static inline const char* dmcp_skill_to_string(skill_t skill) {
  switch (skill) {
    case sk_baby:
      return "I'm Too Young To Die";
    case sk_easy:
      return "Hey, Not Too Rough";
    case sk_medium:
      return "Hurt Me Plenty";
    case sk_hard:
      return "Ultra-Violence";
    case sk_nightmare:
      return "Nightmare!";
    default:
      return "Unknown";
  }
}

static inline const char* dmcp_playerstate_to_string(playerstate_t state) {
  switch (state) {
    case PST_LIVE:
      return "alive";
    case PST_DEAD:
      return "dead";
    case PST_REBORN:
      return "reborn";
    default:
      return "unknown";
  }
}

static inline const char* dmcp_gamestate_to_string(gamestate_t state) {
  switch (state) {
    case GS_LEVEL:
      return "in_level";
    case GS_INTERMISSION:
      return "intermission";
    case GS_FINALE:
      return "finale";
    case GS_DEMOSCREEN:
      return "demo";
    default:
      return "unknown";
  }
}

static inline const char* dmcp_weapon_to_string(weapontype_t weapon) {
  switch (weapon) {
    case wp_fist:
      return "Fist";
    case wp_pistol:
      return "Pistol";
    case wp_shotgun:
      return "Shotgun";
    case wp_chaingun:
      return "Chaingun";
    case wp_missile:
      return "Rocket Launcher";
    case wp_plasma:
      return "Plasma Rifle";
    case wp_bfg:
      return "BFG9000";
    case wp_chainsaw:
      return "Chainsaw";
    case wp_supershotgun:
      return "Super Shotgun";
    default:
      return "Unknown";
  }
}

static inline const char* dmcp_armortype_to_string(int armortype) {
  switch (armortype) {
    case 0:
      return "None";
    case 1:
      return "Green Armor";
    case 2:
      return "Blue Armor";
    default:
      return "Unknown";
  }
}

static inline const char* dmcp_gamemode_to_string(boolean netgame, int deathmatch) {
  if (!netgame) {
    return "single_player";
  }
  switch (deathmatch) {
    case 0:
      return "cooperative";
    case 1:
      return "deathmatch";
    case 2:
      return "altdeath";
    default:
      return "unknown";
  }
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

#ifdef __cplusplus
}
#endif

#endif
