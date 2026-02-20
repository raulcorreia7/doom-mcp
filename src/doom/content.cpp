#include "dmcp/adapter/content.h"
#include "dmcp/adapter/utils.h"
#include <cstdio>
#include <cstring>

namespace {

bool matches_any(const char* name, const char* const* list, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (dmcp_str_equals_ci(name, list[i])) {
      return true;
    }
  }
  return false;
}

struct name_alias {
  const char* alias;
  const char* canonical;
};

const char* normalize_alias(const char* name, const name_alias* aliases, size_t alias_count) {
  if (!name) {
    return name;
  }

  for (size_t i = 0; i < alias_count; ++i) {
    if (dmcp_str_equals_ci(name, aliases[i].alias)) {
      return aliases[i].canonical;
    }
  }

  return name;
}

enum class content_policy {
  whitelist,
  blacklist,
  hybrid,
};

constexpr content_policy k_default_policy = content_policy::hybrid;

using known_check_fn      = bool (*)(const char*);
using restricted_check_fn = bool (*)(const char*, dmcp_gamemode_t);

bool evaluate_with_policy(const char* name, dmcp_gamemode_t mode, known_check_fn is_known,
                          restricted_check_fn is_restricted, content_policy policy) {
  if (!name || name[0] == '\0') {
    return false;
  }

  const bool known      = is_known ? is_known(name) : true;
  const bool restricted = is_restricted ? is_restricted(name, mode) : false;

  switch (policy) {
    case content_policy::whitelist:
      return known && !restricted;
    case content_policy::blacklist:
      return !restricted;
    case content_policy::hybrid:
      // Canonical/base content is validated strictly; unknown names are allowed so
      // mods/custom WADs are not blocked by static SDK tables.
      return known ? !restricted : true;
  }

  return false;
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
    "Stimpack", "Medikit", "SoulSphere", "MegaSphere",
    // Armor
    "GreenArmor", "BlueArmor",
    // Ammo
    "Clip", "BoxOfBullets", "Shells", "BoxOfShells", "RocketAmmo", "BoxOfRockets", "Cell",
    "CellPack",
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

static const char* const dmcp_registered_restricted_items[] = {"MegaSphere"};

static const size_t dmcp_registered_restricted_items_count =
    sizeof(dmcp_registered_restricted_items) / sizeof(dmcp_registered_restricted_items[0]);

static const name_alias dmcp_weapon_aliases[] = {
    {"Plasma Rifle", "PlasmaRifle"}, {"Plasma", "PlasmaRifle"},         {"BFG", "BFG9000"},
    {"BFG 9000", "BFG9000"},         {"Super Shotgun", "SuperShotgun"}, {"SSG", "SuperShotgun"}};

static const size_t dmcp_weapon_aliases_count =
    sizeof(dmcp_weapon_aliases) / sizeof(dmcp_weapon_aliases[0]);

static const name_alias dmcp_item_aliases[] = {{"Bullets", "Clip"},
                                               {"Shell", "Shells"},
                                               {"Rocket", "RocketAmmo"},
                                               {"Rockets", "RocketAmmo"},
                                               {"Rocket Ammo", "RocketAmmo"},
                                               {"Box of Bullets", "BoxOfBullets"},
                                               {"Box of Shells", "BoxOfShells"},
                                               {"Box of Rockets", "BoxOfRockets"},
                                               {"Cells", "Cell"},
                                               {"Cell Pack", "CellPack"},
                                               {"Green Armor", "GreenArmor"},
                                               {"Blue Armor", "BlueArmor"},
                                               {"Soul Sphere", "SoulSphere"},
                                               {"Mega Sphere", "MegaSphere"}};

static const size_t dmcp_item_aliases_count =
    sizeof(dmcp_item_aliases) / sizeof(dmcp_item_aliases[0]);

static const name_alias dmcp_enemy_aliases[] = {{"Imp", "DoomImp"},
                                                {"Zombie", "Zombieman"},
                                                {"Shotgun Guy", "ShotgunGuy"},
                                                {"Chaingun Guy", "ChaingunGuy"},
                                                {"Pinky", "Demon"},
                                                {"Lost Soul", "LostSoul"},
                                                {"Baron of Hell", "BaronOfHell"},
                                                {"Hell Knight", "HellKnight"},
                                                {"Pain Elemental", "PainElemental"},
                                                {"Arch-vile", "Archvile"},
                                                {"Arch Vile", "Archvile"},
                                                {"Spider Mastermind", "SpiderMastermind"},
                                                {"Fatso", "Mancubus"},
                                                {"Commando", "ChaingunGuy"},
                                                {"Icon Of Sin", "BossBrain"},
                                                {"Cyber", "Cyberdemon"}};

static const size_t dmcp_enemy_aliases_count =
    sizeof(dmcp_enemy_aliases) / sizeof(dmcp_enemy_aliases[0]);

static const char* normalize_weapon_name(const char* name) {
  return normalize_alias(name, dmcp_weapon_aliases, dmcp_weapon_aliases_count);
}

static const char* normalize_item_name(const char* name) {
  const char* normalized = normalize_weapon_name(name);
  return normalize_alias(normalized, dmcp_item_aliases, dmcp_item_aliases_count);
}

static const char* normalize_enemy_name(const char* name) {
  return normalize_alias(name, dmcp_enemy_aliases, dmcp_enemy_aliases_count);
}

static bool is_known_weapon_name(const char* name) {
  const char* normalized = normalize_weapon_name(name);
  return matches_any(normalized, dmcp_all_weapons, dmcp_all_weapons_count);
}

static bool is_known_enemy_name(const char* name) {
  const char* normalized = normalize_enemy_name(name);
  return matches_any(normalized, dmcp_all_enemies, dmcp_all_enemies_count);
}

static bool is_known_item_name(const char* name) {
  const char* normalized = normalize_item_name(name);
  return matches_any(normalized, dmcp_all_items, dmcp_all_items_count) ||
         matches_any(normalized, dmcp_all_weapons, dmcp_all_weapons_count);
}

static bool is_known_map_name(const char* name) {
  return matches_any(name, dmcp_all_maps_doom1, dmcp_all_maps_doom1_count) ||
         matches_any(name, dmcp_all_maps_doom2, dmcp_all_maps_doom2_count);
}

static bool is_weapon_restricted_by_mode(const char* name, dmcp_gamemode_t mode) {
  const char* normalized = normalize_weapon_name(name);

  if (mode == DMCP_GAMEMODE_SHAREWARE) {
    return matches_any(normalized, dmcp_shareware_restricted_weapons,
                       dmcp_shareware_restricted_weapons_count);
  }

  if (mode == DMCP_GAMEMODE_REGISTERED || mode == DMCP_GAMEMODE_RETAIL) {
    return matches_any(normalized, dmcp_doom2_only_weapons, dmcp_doom2_only_weapons_count);
  }

  return false;
}

static bool is_enemy_restricted_by_mode(const char* name, dmcp_gamemode_t mode) {
  const char* normalized = normalize_enemy_name(name);

  if (mode == DMCP_GAMEMODE_SHAREWARE) {
    return matches_any(normalized, dmcp_shareware_restricted_enemies,
                       dmcp_shareware_restricted_enemies_count);
  }

  if (mode == DMCP_GAMEMODE_REGISTERED || mode == DMCP_GAMEMODE_RETAIL) {
    return matches_any(normalized, dmcp_doom2_only_enemies, dmcp_doom2_only_enemies_count);
  }

  return false;
}

static bool is_map_restricted_by_mode(const char* name, dmcp_gamemode_t mode) {
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
  const char* normalized = normalize_item_name(name);

  if (is_weapon_restricted_by_mode(normalized, mode)) {
    return true;
  }

  if (mode == DMCP_GAMEMODE_SHAREWARE && matches_any(normalized, dmcp_shareware_restricted_items,
                                                     dmcp_shareware_restricted_items_count)) {
    return true;
  }

  if (mode == DMCP_GAMEMODE_REGISTERED && matches_any(normalized, dmcp_registered_restricted_items,
                                                      dmcp_registered_restricted_items_count)) {
    return true;
  }

  return false;
}

// ============================================================================
// Availability Checks
// ============================================================================

bool dmcp_is_weapon_available(const char* weapon_name, dmcp_gamemode_t mode) {
  return evaluate_with_policy(weapon_name, mode, is_known_weapon_name, is_weapon_restricted_by_mode,
                              k_default_policy);
}

bool dmcp_is_enemy_spawnable(const char* enemy_type, dmcp_gamemode_t mode) {
  return evaluate_with_policy(enemy_type, mode, is_known_enemy_name, is_enemy_restricted_by_mode,
                              k_default_policy);
}

bool dmcp_is_map_available(const char* map_name, dmcp_gamemode_t mode) {
  return evaluate_with_policy(map_name, mode, is_known_map_name, is_map_restricted_by_mode,
                              k_default_policy);
}

bool dmcp_is_item_available(const char* item_name, dmcp_gamemode_t mode) {
  return evaluate_with_policy(item_name, mode, is_known_item_name, is_item_restricted_by_mode,
                              k_default_policy);
}

const char* dmcp_content_unavailable_message(const char* content_type, const char* content_name,
                                             dmcp_gamemode_t mode) {
  static char buffer[256];

  const char* mode_str = "unknown";
  switch (mode) {
    case DMCP_GAMEMODE_SHAREWARE:
      mode_str = "shareware";
      break;
    case DMCP_GAMEMODE_REGISTERED:
      mode_str = "registered";
      break;
    case DMCP_GAMEMODE_COMMERCIAL:
      mode_str = "commercial";
      break;
    case DMCP_GAMEMODE_RETAIL:
      mode_str = "retail";
      break;
    default:
      break;
  }

  std::snprintf(buffer, sizeof(buffer), "%s '%s' not available in %s mode",
                content_type ? content_type : "Content", content_name ? content_name : "unknown",
                mode_str);

  return buffer;
}

}  // extern "C"
