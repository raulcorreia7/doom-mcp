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

const char* const dmcp_shareware_restricted_weapons[] = {
    "PlasmaRifle", "Plasma Rifle", "Plasma",        "BFG9000", "BFG",
    "BFG 9000",    "SuperShotgun", "Super Shotgun", "SSG"};

const size_t dmcp_shareware_restricted_weapons_count =
    sizeof(dmcp_shareware_restricted_weapons) / sizeof(dmcp_shareware_restricted_weapons[0]);

const char* const dmcp_shareware_restricted_enemies[] = {"Arachnotron",       "PainElemental",
                                                         "Pain Elemental",    "Revenant",
                                                         "Mancubus",          "Fatso",
                                                         "Archvile",          "Arch-vile",
                                                         "Arch Vile",         "SpiderMastermind",
                                                         "Spider Mastermind", "SpiderBoss",
                                                         "Cyberdemon",        "Cyber"};

const size_t dmcp_shareware_restricted_enemies_count =
    sizeof(dmcp_shareware_restricted_enemies) / sizeof(dmcp_shareware_restricted_enemies[0]);

const char* const dmcp_shareware_restricted_maps[] = {
    "E2M1", "E2M2", "E2M3", "E2M4", "E2M5", "E2M6", "E2M7", "E2M8", "E2M9",
    "E3M1", "E3M2", "E3M3", "E3M4", "E3M5", "E3M6", "E3M7", "E3M8", "E3M9",
    "E4M1", "E4M2", "E4M3", "E4M4", "E4M5", "E4M6", "E4M7", "E4M8", "E4M9"};

const size_t dmcp_shareware_restricted_maps_count =
    sizeof(dmcp_shareware_restricted_maps) / sizeof(dmcp_shareware_restricted_maps[0]);

const char* const dmcp_shareware_restricted_items[] = {"Cell",        "Cells",   "PlasmaBolt",
                                                       "Plasma Bolt", "BFGBall", "BFG Ball"};

const size_t dmcp_shareware_restricted_items_count =
    sizeof(dmcp_shareware_restricted_items) / sizeof(dmcp_shareware_restricted_items[0]);

// ============================================================================
// Availability Checks
// ============================================================================

bool dmcp_is_weapon_available(const char* weapon_name, dmcp_gamemode_t mode) {
  if (!weapon_name) return false;

  if (mode == DMCP_GAMEMODE_SHAREWARE) {
    return !matches_any(weapon_name, dmcp_shareware_restricted_weapons,
                        dmcp_shareware_restricted_weapons_count);
  }
  return true;
}

bool dmcp_is_enemy_spawnable(const char* enemy_type, dmcp_gamemode_t mode) {
  if (!enemy_type) return false;

  if (mode == DMCP_GAMEMODE_SHAREWARE) {
    return !matches_any(enemy_type, dmcp_shareware_restricted_enemies,
                        dmcp_shareware_restricted_enemies_count);
  }
  return true;
}

bool dmcp_is_map_available(const char* map_name, dmcp_gamemode_t mode) {
  if (!map_name) return false;

  if (mode == DMCP_GAMEMODE_SHAREWARE) {
    return !matches_any(map_name, dmcp_shareware_restricted_maps,
                        dmcp_shareware_restricted_maps_count);
  }
  return true;
}

bool dmcp_is_item_available(const char* item_name, dmcp_gamemode_t mode) {
  if (!item_name) return false;

  if (mode == DMCP_GAMEMODE_SHAREWARE) {
    if (matches_any(item_name, dmcp_shareware_restricted_weapons,
                    dmcp_shareware_restricted_weapons_count)) {
      return false;
    }
    if (matches_any(item_name, dmcp_shareware_restricted_items,
                    dmcp_shareware_restricted_items_count)) {
      return false;
    }
  }
  return true;
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
