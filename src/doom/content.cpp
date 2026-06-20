#include "dmcp/doom/content.h"

#include <cstdio>
#include <cstring>

namespace {

bool str_equals(const char* lhs, const char* rhs) {
  if (!lhs || !rhs) {
    return lhs == rhs;
  }
  return std::strcmp(lhs, rhs) == 0;
}

bool matches_any(const char* name, const char* const* list, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (str_equals(name, list[i])) {
      return true;
    }
  }
  return false;
}

using known_check_fn      = bool (*)(const char*);
using restricted_check_fn = bool (*)(const char*, dmcp_gamemode_t);

bool evaluate_with_policy(const char* name, dmcp_gamemode_t mode, known_check_fn is_known,
                          restricted_check_fn is_restricted) {
  if (!name || name[0] == '\0') {
    return false;
  }

  const bool known      = is_known ? is_known(name) : true;
  const bool restricted = is_restricted ? is_restricted(name, mode) : false;
  return known && !restricted;
}

const char* gamemode_name(dmcp_gamemode_t mode) {
  switch (mode) {
    case DMCP_GAMEMODE_SHAREWARE:
      return "shareware";
    case DMCP_GAMEMODE_REGISTERED:
      return "registered";
    case DMCP_GAMEMODE_COMMERCIAL:
      return "commercial";
    case DMCP_GAMEMODE_RETAIL:
      return "retail";
    default:
      return "unknown";
  }
}

int format_content_unavailable_message(const char* content_type, const char* content_name,
                                       dmcp_gamemode_t mode, char* buffer, size_t buffer_size) {
  if (!buffer || buffer_size == 0) {
    return -1;
  }

  const int written = std::snprintf(buffer, buffer_size, "%s '%s' not available in %s mode",
                                    content_type ? content_type : "Content",
                                    content_name ? content_name : "unknown", gamemode_name(mode));

  if (written < 0 || static_cast<size_t>(written) >= buffer_size) {
    buffer[0] = '\0';
    return -1;
  }

  return written;
}

}  // namespace

extern "C" {

// ============================================================================
// All Available Content
// ============================================================================

const char* const dmcp_all_weapons[] = {"Fist",           "Chainsaw",     "Pistol",
                                        "Shotgun",        "SuperShotgun", "Chaingun",
                                        "RocketLauncher", "PlasmaRifle",  "BFG9000"};

const size_t dmcp_all_weapons_count = sizeof(dmcp_all_weapons) / sizeof(dmcp_all_weapons[0]);

const char* const dmcp_all_items[] = {
    // Health
    "HealthBonus", "Stimpack", "Medikit", "SoulSphere", "MegaSphere",
    // Armor
    "ArmorBonus", "GreenArmor", "BlueArmor",
    // Ammo
    "Clip", "BoxOfBullets", "Shells", "BoxOfShells", "RocketAmmo", "BoxOfRockets", "Cell",
    "CellPack",
    // Keys
    "BlueKeycard", "YellowKeycard", "RedKeycard", "BlueSkullKey", "YellowSkullKey", "RedSkullKey",
    // Powerups
    "Backpack", "Invulnerability", "Berserk", "Invisibility", "RadiationSuit", "ComputerMap",
    "LightAmp", "MegaArmor"};

const size_t dmcp_all_items_count = sizeof(dmcp_all_items) / sizeof(dmcp_all_items[0]);

const char* const dmcp_all_enemies[] = {
    "Zombieman", "ShotgunGuy", "ChaingunGuy", "DoomImp",          "Demon",       "Spectre",
    "LostSoul",  "Cacodemon",  "BaronOfHell", "HellKnight",       "Arachnotron", "PainElemental",
    "Revenant",  "Mancubus",   "Archvile",    "SpiderMastermind", "Cyberdemon",  "BossBrain"};

const size_t dmcp_all_enemies_count = sizeof(dmcp_all_enemies) / sizeof(dmcp_all_enemies[0]);

const char* const dmcp_all_maps_doom1[] = {
    "E1M1", "E1M2", "E1M3", "E1M4", "E1M5", "E1M6", "E1M7", "E1M8", "E1M9", "E2M1", "E2M2", "E2M3",
    "E2M4", "E2M5", "E2M6", "E2M7", "E2M8", "E2M9", "E3M1", "E3M2", "E3M3", "E3M4", "E3M5", "E3M6",
    "E3M7", "E3M8", "E3M9", "E4M1", "E4M2", "E4M3", "E4M4", "E4M5", "E4M6", "E4M7", "E4M8", "E4M9"};

const size_t dmcp_all_maps_doom1_count =
    sizeof(dmcp_all_maps_doom1) / sizeof(dmcp_all_maps_doom1[0]);

const char* const dmcp_all_maps_doom2[] = {
    "MAP01", "MAP02", "MAP03", "MAP04", "MAP05", "MAP06", "MAP07", "MAP08",
    "MAP09", "MAP10", "MAP11", "MAP12", "MAP13", "MAP14", "MAP15", "MAP16",
    "MAP17", "MAP18", "MAP19", "MAP20", "MAP21", "MAP22", "MAP23", "MAP24",
    "MAP25", "MAP26", "MAP27", "MAP28", "MAP29", "MAP30", "MAP31", "MAP32"};

const size_t dmcp_all_maps_doom2_count =
    sizeof(dmcp_all_maps_doom2) / sizeof(dmcp_all_maps_doom2[0]);

// ============================================================================
// Shareware Restrictions
// ============================================================================

const char* const dmcp_shareware_restricted_weapons[] = {"PlasmaRifle", "BFG9000", "SuperShotgun"};

const size_t dmcp_shareware_restricted_weapons_count =
    sizeof(dmcp_shareware_restricted_weapons) / sizeof(dmcp_shareware_restricted_weapons[0]);

static const char* const dmcp_doom2_only_weapons[] = {"SuperShotgun"};

static const size_t dmcp_doom2_only_weapons_count =
    sizeof(dmcp_doom2_only_weapons) / sizeof(dmcp_doom2_only_weapons[0]);

// Enemies not present in Episode 1 (Knee-Deep in the Dead) / shareware
const char* const dmcp_shareware_restricted_enemies[] = {
    // Doom 2 / non-shareware monsters
    "ChaingunGuy", "HellKnight", "Arachnotron", "PainElemental", "Revenant", "Mancubus", "Archvile",
    "BossBrain",
    // Doom 1 Episode 2+ monsters
    "SpiderMastermind", "Cyberdemon", "Cacodemon", "LostSoul"};

const size_t dmcp_shareware_restricted_enemies_count =
    sizeof(dmcp_shareware_restricted_enemies) / sizeof(dmcp_shareware_restricted_enemies[0]);

static const char* const dmcp_doom2_only_enemies[] = {"ChaingunGuy",   "HellKnight", "Arachnotron",
                                                      "PainElemental", "Revenant",   "Mancubus",
                                                      "Archvile",      "BossBrain"};

static const size_t dmcp_doom2_only_enemies_count =
    sizeof(dmcp_doom2_only_enemies) / sizeof(dmcp_doom2_only_enemies[0]);

const char* const dmcp_shareware_restricted_maps[] = {
    "E2M1", "E2M2", "E2M3", "E2M4", "E2M5", "E2M6", "E2M7", "E2M8", "E2M9",
    "E3M1", "E3M2", "E3M3", "E3M4", "E3M5", "E3M6", "E3M7", "E3M8", "E3M9",
    "E4M1", "E4M2", "E4M3", "E4M4", "E4M5", "E4M6", "E4M7", "E4M8", "E4M9"};

const size_t dmcp_shareware_restricted_maps_count =
    sizeof(dmcp_shareware_restricted_maps) / sizeof(dmcp_shareware_restricted_maps[0]);

static const char* const dmcp_registered_restricted_maps[] = {
    "E4M1", "E4M2", "E4M3", "E4M4", "E4M5", "E4M6", "E4M7", "E4M8", "E4M9"};

static const size_t dmcp_registered_restricted_maps_count =
    sizeof(dmcp_registered_restricted_maps) / sizeof(dmcp_registered_restricted_maps[0]);

const char* const dmcp_shareware_restricted_items[] = {"Cell", "CellPack", "MegaSphere"};

const size_t dmcp_shareware_restricted_items_count =
    sizeof(dmcp_shareware_restricted_items) / sizeof(dmcp_shareware_restricted_items[0]);

static const char* const dmcp_commercial_only_items[] = {"MegaSphere"};

static const size_t dmcp_commercial_only_items_count =
    sizeof(dmcp_commercial_only_items) / sizeof(dmcp_commercial_only_items[0]);

static bool is_known_weapon_name(const char* name) {
  return matches_any(name, dmcp_all_weapons, dmcp_all_weapons_count);
}

static bool is_known_enemy_name(const char* name) {
  return matches_any(name, dmcp_all_enemies, dmcp_all_enemies_count);
}

static bool is_known_item_name(const char* name) {
  return matches_any(name, dmcp_all_items, dmcp_all_items_count) ||
         matches_any(name, dmcp_all_weapons, dmcp_all_weapons_count);
}

static bool is_known_map_name(const char* name) {
  return matches_any(name, dmcp_all_maps_doom1, dmcp_all_maps_doom1_count) ||
         matches_any(name, dmcp_all_maps_doom2, dmcp_all_maps_doom2_count);
}

static bool is_weapon_restricted_by_mode(const char* name, dmcp_gamemode_t mode) {
  if (mode == DMCP_GAMEMODE_SHAREWARE) {
    return matches_any(name, dmcp_shareware_restricted_weapons,
                       dmcp_shareware_restricted_weapons_count);
  }

  if (mode == DMCP_GAMEMODE_REGISTERED || mode == DMCP_GAMEMODE_RETAIL) {
    return matches_any(name, dmcp_doom2_only_weapons, dmcp_doom2_only_weapons_count);
  }

  return false;
}

static bool is_enemy_restricted_by_mode(const char* name, dmcp_gamemode_t mode) {
  if (mode == DMCP_GAMEMODE_SHAREWARE) {
    return matches_any(name, dmcp_shareware_restricted_enemies,
                       dmcp_shareware_restricted_enemies_count);
  }

  if (mode == DMCP_GAMEMODE_REGISTERED || mode == DMCP_GAMEMODE_RETAIL) {
    return matches_any(name, dmcp_doom2_only_enemies, dmcp_doom2_only_enemies_count);
  }

  return false;
}

static bool is_map_restricted_by_mode(const char* name, dmcp_gamemode_t mode) {
  if (mode == DMCP_GAMEMODE_COMMERCIAL &&
      matches_any(name, dmcp_all_maps_doom1, dmcp_all_maps_doom1_count)) {
    return true;
  }

  if (mode != DMCP_GAMEMODE_COMMERCIAL &&
      matches_any(name, dmcp_all_maps_doom2, dmcp_all_maps_doom2_count)) {
    return true;
  }

  if (mode == DMCP_GAMEMODE_SHAREWARE) {
    return matches_any(name, dmcp_shareware_restricted_maps, dmcp_shareware_restricted_maps_count);
  }

  if (mode == DMCP_GAMEMODE_REGISTERED) {
    return matches_any(name, dmcp_registered_restricted_maps,
                       dmcp_registered_restricted_maps_count);
  }

  return false;
}

static bool is_item_restricted_by_mode(const char* name, dmcp_gamemode_t mode) {
  if (is_weapon_restricted_by_mode(name, mode)) {
    return true;
  }

  if (mode == DMCP_GAMEMODE_SHAREWARE &&
      matches_any(name, dmcp_shareware_restricted_items, dmcp_shareware_restricted_items_count)) {
    return true;
  }

  if (mode != DMCP_GAMEMODE_COMMERCIAL &&
      matches_any(name, dmcp_commercial_only_items, dmcp_commercial_only_items_count)) {
    return true;
  }

  return false;
}

// ============================================================================
// Availability Checks
// ============================================================================

bool dmcp_is_weapon_available(const char* weapon_name, dmcp_gamemode_t mode) {
  return evaluate_with_policy(weapon_name, mode, is_known_weapon_name,
                              is_weapon_restricted_by_mode);
}

bool dmcp_is_enemy_spawnable(const char* enemy_type, dmcp_gamemode_t mode) {
  return evaluate_with_policy(enemy_type, mode, is_known_enemy_name, is_enemy_restricted_by_mode);
}

bool dmcp_is_map_available(const char* map_name, dmcp_gamemode_t mode) {
  return evaluate_with_policy(map_name, mode, is_known_map_name, is_map_restricted_by_mode);
}

bool dmcp_is_item_available(const char* item_name, dmcp_gamemode_t mode) {
  return evaluate_with_policy(item_name, mode, is_known_item_name, is_item_restricted_by_mode);
}

int dmcp_content_unavailable_message_copy(const char* content_type, const char* content_name,
                                          dmcp_gamemode_t mode, char* buffer, size_t buffer_size) {
  return format_content_unavailable_message(content_type, content_name, mode, buffer, buffer_size);
}

const char* dmcp_content_unavailable_message(const char* content_type, const char* content_name,
                                             dmcp_gamemode_t mode) {
  thread_local char buffer[256];
  (void)format_content_unavailable_message(content_type, content_name, mode, buffer,
                                           sizeof(buffer));
  return buffer;
}

}  // extern "C"
