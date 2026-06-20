#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "dmcp/doom/protocol.h"

namespace dmcp::content_categories {

inline constexpr std::array<const char*, 8> ammo_items = {
    "Clip",       "BoxOfBullets", "Shells", "BoxOfShells",
    "RocketAmmo", "BoxOfRockets", "Cell",   "CellPack"};

inline constexpr std::array<const char*, 6> key_items = {
    "BlueKeycard", "YellowKeycard", "RedKeycard", "BlueSkullKey", "YellowSkullKey", "RedSkullKey"};

inline constexpr std::array<const char*, 7> spawnable_weapon_items = {
    "Chainsaw", "Shotgun", "SuperShotgun", "Chaingun", "RocketLauncher", "PlasmaRifle", "BFG9000"};

inline constexpr std::array<const char*, 5> health_items = {"HealthBonus", "Stimpack", "Medikit",
                                                            "SoulSphere", "MegaSphere"};

inline constexpr std::array<const char*, 5> armor_items = {"ArmorBonus", "GreenArmor", "BlueArmor",
                                                           "MegaArmor", "Backpack"};

inline constexpr std::array<const char*, 6> powerup_items = {
    "Invulnerability", "Berserk", "Invisibility", "RadiationSuit", "ComputerMap", "LightAmp"};

template <size_t N>
inline bool contains(const std::array<const char*, N>& values, std::string_view value) {
  for (const char* candidate : values) {
    if (std::string_view(candidate) == value) {
      return true;
    }
  }
  return false;
}

inline bool is_ammo(std::string_view value) { return contains(ammo_items, value); }
inline bool is_key(std::string_view value) { return contains(key_items, value); }
inline bool is_weapon(std::string_view value) { return contains(spawnable_weapon_items, value); }
inline bool is_health(std::string_view value) { return contains(health_items, value); }
inline bool is_armor(std::string_view value) { return contains(armor_items, value); }
inline bool is_powerup(std::string_view value) { return contains(powerup_items, value); }

inline bool is_item(std::string_view value) {
  return is_ammo(value) || is_key(value) || is_weapon(value) || is_health(value) ||
         is_armor(value) || is_powerup(value);
}

inline std::string_view item_kind(std::string_view value) {
  if (is_weapon(value)) return DMCP_KIND_WEAPON;
  if (is_ammo(value)) return DMCP_KIND_AMMO;
  if (is_key(value)) return DMCP_KIND_KEY;
  if (is_health(value)) return DMCP_KIND_HEALTH;
  if (is_armor(value)) return DMCP_KIND_ARMOR;
  if (is_powerup(value)) return DMCP_KIND_POWERUP;
  return DMCP_KIND_ITEM;
}

}  // namespace dmcp::content_categories
