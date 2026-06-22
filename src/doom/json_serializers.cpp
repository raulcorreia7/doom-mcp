#include "doom/internal/json_serializers.hpp"

#include <cstdint>

#include "dmcp/doom/protocol.h"

namespace dmcp::json_serializers {
namespace {

constexpr const char* k_weapon_names[] = {"Fist",           "Pistol",       "Shotgun",
                                          "Chaingun",       "RocketLauncher", "PlasmaRifle",
                                          "BFG9000",        "Chainsaw",     "SuperShotgun"};

constexpr const char* k_ammo_names[] = {"Clip", "Shells", "Cell", "RocketAmmo"};

constexpr const char* k_key_names[] = {"BlueKeycard", "YellowKeycard", "RedKeycard",
                                       "BlueSkullKey", "YellowSkullKey", "RedSkullKey"};

constexpr const char* k_power_names[] = {"Invulnerability", "Berserk",      "Invisibility",
                                         "RadiationSuit",   "ComputerMap",  "LightAmp"};

json_builder owned_weapons(const int32_t* values) {
  json_builder arr;
  arr.start_array();
  for (std::size_t i = 0; i < DMCP_MAX_WEAPONS; ++i) {
    if (values[i] != 0) {
      arr.push(k_weapon_names[i]);
    }
  }
  return arr;
}

json_builder ammo_counts(const int32_t* ammo, const int32_t* maxammo) {
  json_builder obj;
  obj.start_object();
  for (std::size_t i = 0; i < DMCP_MAX_AMMO_TYPES; ++i) {
    json_builder entry;
    entry.start_object();
    entry.add("amount", static_cast<int64_t>(ammo[i]));
    entry.add("max", static_cast<int64_t>(maxammo[i]));
    obj.add(k_ammo_names[i], std::move(entry));
  }
  return obj;
}

json_builder owned_keys(const int32_t* values) {
  json_builder arr;
  arr.start_array();
  for (std::size_t i = 0; i < DMCP_MAX_KEYS; ++i) {
    if (values[i] != 0) {
      arr.push(k_key_names[i]);
    }
  }
  return arr;
}

json_builder power_timers(const int32_t* values) {
  json_builder obj;
  obj.start_object();
  for (std::size_t i = 0; i < DMCP_MAX_POWERUPS; ++i) {
    obj.add(k_power_names[i], static_cast<int64_t>(values[i]));
  }
  return obj;
}

}  // namespace

json_builder vec3(const dmcp_vec3_t& position) {
  json_builder obj;
  obj.start_object();
  obj.add("x", static_cast<double>(position.x));
  obj.add("y", static_cast<double>(position.y));
  obj.add("z", static_cast<double>(position.z));
  return obj;
}

json_builder player(const dmcp_player_t& player) {
  json_builder obj;
  obj.start_object();
  obj.add("hp", static_cast<int64_t>(player.hp));
  obj.add("armor", static_cast<int64_t>(player.armor));
  obj.add("armortype", player.armortype);
  obj.add("position", vec3(player.position));
  obj.add("angle", static_cast<double>(player.angle));
  obj.add("readyweapon", player.readyweapon);
  obj.add("pendingweapon", player.pendingweapon);
  obj.add("weapons", owned_weapons(player.weaponowned));
  obj.add("ammo", ammo_counts(player.ammo, player.maxammo));
  obj.add("backpack", player.backpack != 0);
  obj.add("powers", power_timers(player.powers));
  obj.add("keys", owned_keys(player.cards));
  obj.add("playerstate", player.playerstate);
  obj.add("cheats", static_cast<int64_t>(player.cheats));
  obj.add("damagecount", static_cast<int64_t>(player.damagecount));
  return obj;
}

json_builder level(const dmcp_level_t& level) {
  json_builder obj;
  obj.start_object();
  obj.add("tic", static_cast<int64_t>(level.tic));
  obj.add("leveltime", static_cast<int64_t>(level.leveltime));
  obj.add("level_id", level.level_id);
  obj.add("level_name", level.level_name);
  obj.add("kill_count", static_cast<int64_t>(level.kill_count));
  obj.add("item_count", static_cast<int64_t>(level.item_count));
  obj.add("secret_count", static_cast<int64_t>(level.secret_count));
  obj.add("totalkills", static_cast<int64_t>(level.totalkills));
  obj.add("totalitems", static_cast<int64_t>(level.totalitems));
  obj.add("totalsecrets", static_cast<int64_t>(level.totalsecrets));
  obj.add("skill", level.skill);
  obj.add("gamestate", level.gamestate);
  obj.add("paused", level.paused != 0);
  return obj;
}

json_builder game(const dmcp_game_t& game) {
  json_builder obj;
  obj.start_object();
  obj.add("mode", game.mode);
  obj.add("version", game.version);
  obj.add("gamestate", game.gamestate);
  obj.add("paused", game.paused != 0);
  obj.add("respawnmonsters", game.respawnmonsters != 0);
  obj.add("consoleplayer", static_cast<int64_t>(game.consoleplayer));
  return obj;
}

json_builder enemy(const dmcp_enemy_t& enemy) {
  json_builder obj;
  obj.start_object();
  obj.add("id", static_cast<int64_t>(enemy.id));
  obj.add("hp", static_cast<int64_t>(enemy.hp));
  obj.add("max_hp", static_cast<int64_t>(enemy.max_hp));
  obj.add("state", enemy.hp > 0 ? DMCP_STATUS_ALIVE : DMCP_STATUS_DEAD);
  obj.add("position", vec3(enemy.position));
  obj.add("angle", static_cast<double>(enemy.angle));
  obj.add("target_id", static_cast<int64_t>(enemy.target_id));
  obj.add("type", enemy.type);
  return obj;
}

json_builder item(const dmcp_item_t& item) {
  json_builder obj;
  obj.start_object();
  obj.add("name", item.name);
  obj.add("amount", static_cast<int64_t>(item.amount));
  return obj;
}

json_builder entity(const dmcp_entity_t& entity) {
  json_builder obj;
  obj.start_object();
  obj.add("id", static_cast<int64_t>(entity.id));
  obj.add("hp", static_cast<int64_t>(entity.hp));
  obj.add("max_hp", static_cast<int64_t>(entity.max_hp));
  obj.add("position", vec3(entity.position));
  obj.add("angle", static_cast<double>(entity.angle));
  obj.add("type", entity.type);
  return obj;
}

}  // namespace dmcp::json_serializers
