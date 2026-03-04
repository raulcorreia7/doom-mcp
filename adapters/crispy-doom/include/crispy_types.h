// Type Mapping Utilities
// Centralizes conversions from Crispy Doom types to DMCP output

#ifndef DMCP_MAPPINGS_H
#define DMCP_MAPPINGS_H

#include <stddef.h>

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"
#include "m_fixed.h"

#include "dmcp/doom/dmcp.h"
#include "dmcp/adapter/utils.h"

#ifdef __cplusplus
extern "C" {
#endif

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

static inline dmcp_gamemode_t dmcp_to_gamemode(GameMode_t mode) {
  switch (mode) {
    case shareware:
      return DMCP_GAMEMODE_SHAREWARE;
    case registered:
      return DMCP_GAMEMODE_REGISTERED;
    case commercial:
      return DMCP_GAMEMODE_COMMERCIAL;
    case retail:
      return DMCP_GAMEMODE_RETAIL;
    default:
      return DMCP_GAMEMODE_UNKNOWN;
  }
}

#ifdef __cplusplus
}
#endif

#endif
