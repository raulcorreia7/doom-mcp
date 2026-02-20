#include "dmcp/adapter/content.h"
#include <cctype>
#include <cstdio>
#include <cstring>

namespace {

bool str_equals_ci(const char* a, const char* b) {
  if (!a || !b) return false;
  while (*a && *b) {
    if (std::tolower(static_cast<unsigned char>(*a)) !=
        std::tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool matches_any(const char* name, const char* const* list, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (str_equals_ci(name, list[i])) {
      return true;
    }
  }
  return false;
}

}  // namespace

extern "C" {

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
}
