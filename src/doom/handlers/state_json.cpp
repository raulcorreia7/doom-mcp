#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "dmcp/doom/protocol.h"
#include "dmcp/doom/types.h"
#include "doom/internal/json_types.hpp"
#include "doom/handlers/tools/tools.hpp"

namespace dmcp {

namespace {

enum class enemy_status_filter {
  alive,
  dead,
  all,
};

const char* enemy_state_name(const dmcp_enemy_t& enemy) {
  return enemy.hp > 0 ? DMCP_STATUS_ALIVE : DMCP_STATUS_DEAD;
}

bool enemy_matches_filter(const dmcp_enemy_t& enemy, enemy_status_filter filter) {
  const bool is_alive = enemy.hp > 0;

  switch (filter) {
    case enemy_status_filter::alive:
      return is_alive;
    case enemy_status_filter::dead:
      return !is_alive;
    case enemy_status_filter::all:
      return true;
  }

  return false;
}

bool parse_enemy_status_filter(const json_value& args, enemy_status_filter* out_filter,
                               std::string* out_error) {
  if (!out_filter || !out_error) {
    return false;
  }

  *out_filter = enemy_status_filter::alive;

  if (!args.is_object()) {
    return true;
  }

  json_value status_val = args["status"];
  if (!status_val) {
    return true;
  }

  if (!status_val.is_string()) {
    *out_error = "status must be one of: alive, dead, all";
    return false;
  }

  const std::string_view status(status_val.get_string());
  if (status == status::alive) {
    *out_filter = enemy_status_filter::alive;
    return true;
  }

  if (status == status::dead) {
    *out_filter = enemy_status_filter::dead;
    return true;
  }

  if (status == status::all) {
    *out_filter = enemy_status_filter::all;
    return true;
  }

  *out_error = "status must be one of: alive, dead, all";
  return false;
}

json_builder build_vec3_json(const dmcp_vec3_t& position) {
  json_builder obj;
  obj.start_object();
  obj.add("x", static_cast<double>(position.x));
  obj.add("y", static_cast<double>(position.y));
  obj.add("z", static_cast<double>(position.z));
  return obj;
}

json_builder build_player_json(const dmcp_player_t& player) {
  json_builder obj;
  obj.start_object();
  obj.add("hp", static_cast<int64_t>(player.hp));
  obj.add("armor", static_cast<int64_t>(player.armor));
  obj.add("armortype", player.armortype);
  obj.add("position", build_vec3_json(player.position));
  obj.add("angle", static_cast<double>(player.angle));
  obj.add("readyweapon", player.readyweapon);
  obj.add("pendingweapon", player.pendingweapon);

  json_builder weapon_owned;
  weapon_owned.start_array();
  for (size_t i = 0; i < DMCP_MAX_WEAPONS; ++i) {
    weapon_owned.push(static_cast<int64_t>(player.weaponowned[i]));
  }
  obj.add("weaponowned", std::move(weapon_owned));

  json_builder ammo;
  ammo.start_array();
  for (size_t i = 0; i < DMCP_MAX_AMMO_TYPES; ++i) {
    ammo.push(static_cast<int64_t>(player.ammo[i]));
  }
  obj.add("ammo", std::move(ammo));

  json_builder maxammo;
  maxammo.start_array();
  for (size_t i = 0; i < DMCP_MAX_AMMO_TYPES; ++i) {
    maxammo.push(static_cast<int64_t>(player.maxammo[i]));
  }
  obj.add("maxammo", std::move(maxammo));

  obj.add("backpack", player.backpack != 0);

  json_builder powers;
  powers.start_array();
  for (size_t i = 0; i < DMCP_MAX_POWERUPS; ++i) {
    powers.push(static_cast<int64_t>(player.powers[i]));
  }
  obj.add("powers", std::move(powers));

  json_builder cards;
  cards.start_array();
  for (size_t i = 0; i < DMCP_MAX_KEYS; ++i) {
    cards.push(static_cast<int64_t>(player.cards[i]));
  }
  obj.add("cards", std::move(cards));

  obj.add("playerstate", player.playerstate);
  obj.add("cheats", static_cast<int64_t>(player.cheats));
  obj.add("damagecount", static_cast<int64_t>(player.damagecount));
  return obj;
}

json_builder build_level_json(const dmcp_level_t& level) {
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

json_builder build_game_json(const dmcp_game_t& game) {
  json_builder obj;
  obj.start_object();
  obj.add("mode", game.mode);
  obj.add("version", game.version);
  obj.add("respawnmonsters", game.respawnmonsters != 0);
  obj.add("consoleplayer", static_cast<int64_t>(game.consoleplayer));
  return obj;
}

json_builder build_enemy_json(const dmcp_enemy_t& enemy) {
  json_builder obj;
  obj.start_object();
  obj.add("id", static_cast<int64_t>(enemy.id));
  obj.add("hp", static_cast<int64_t>(enemy.hp));
  obj.add("max_hp", static_cast<int64_t>(enemy.max_hp));
  obj.add("state", enemy_state_name(enemy));
  obj.add("position", build_vec3_json(enemy.position));
  obj.add("angle", static_cast<double>(enemy.angle));
  obj.add("target_id", static_cast<int64_t>(enemy.target_id));
  obj.add("type", enemy.type);
  return obj;
}

json_builder build_item_json(const dmcp_item_t& item) {
  json_builder obj;
  obj.start_object();
  obj.add("name", item.name);
  obj.add("amount", static_cast<int64_t>(item.amount));
  return obj;
}

json_builder build_entity_json(const dmcp_entity_t& entity) {
  json_builder obj;
  obj.start_object();
  obj.add("id", static_cast<int64_t>(entity.id));
  obj.add("hp", static_cast<int64_t>(entity.hp));
  obj.add("max_hp", static_cast<int64_t>(entity.max_hp));
  obj.add("position", build_vec3_json(entity.position));
  obj.add("angle", static_cast<double>(entity.angle));
  obj.add("type", entity.type);
  return obj;
}

}  // namespace

std::string build_player_state_json(const dmcp_snapshot_t& snapshot) {
  json_builder payload;
  payload.start_object();
  payload.add("player", build_player_json(snapshot.player));
  return payload.finish();
}

std::string build_map_state_json(const dmcp_snapshot_t& snapshot) {
  json_builder payload;
  payload.start_object();
  payload.add("map", build_level_json(snapshot.level));
  return payload.finish();
}

std::string build_game_info_json(const dmcp_snapshot_t& snapshot) {
  json_builder payload;
  payload.start_object();
  payload.add("game", build_game_json(snapshot.game));
  return payload.finish();
}

std::string build_enemies_state_json(const dmcp_snapshot_t& snapshot, size_t offset, size_t limit,
                                     std::string_view status_filter) {
  enemy_status_filter filter = enemy_status_filter::alive;
  if (status_filter == "dead") {
    filter = enemy_status_filter::dead;
  } else if (status_filter == "all") {
    filter = enemy_status_filter::all;
  }

  std::vector<size_t> matched_indexes;
  matched_indexes.reserve(snapshot.enemy_count);
  for (size_t i = 0; i < snapshot.enemy_count; ++i) {
    if (enemy_matches_filter(snapshot.enemies[i], filter)) {
      matched_indexes.push_back(i);
    }
  }

  const size_t total = matched_indexes.size();
  const size_t start = std::min(offset, total);
  const size_t end   = std::min(start + limit, total);

  json_builder payload;
  payload.start_object();
  payload.add("status", std::string(status_filter));
  payload.add("enemy_count", static_cast<int64_t>(total));
  payload.add("offset", static_cast<int64_t>(start));
  payload.add("limit", static_cast<int64_t>(limit));
  payload.add("returned", static_cast<int64_t>(end - start));

  json_builder enemies;
  enemies.start_array();
  for (size_t i = start; i < end; ++i) {
    enemies.push(build_enemy_json(snapshot.enemies[matched_indexes[i]]));
  }
  payload.add("enemies", std::move(enemies));
  return payload.finish();
}

std::string build_inventory_state_json(const dmcp_snapshot_t& snapshot, size_t offset,
                                       size_t limit) {
  const size_t total = snapshot.inventory_count;
  const size_t start = std::min(offset, total);
  const size_t end   = std::min(start + limit, total);

  json_builder payload;
  payload.start_object();
  payload.add("inventory_count", static_cast<int64_t>(total));
  payload.add("offset", static_cast<int64_t>(start));
  payload.add("limit", static_cast<int64_t>(limit));
  payload.add("returned", static_cast<int64_t>(end - start));

  json_builder items;
  items.start_array();
  for (size_t i = start; i < end; ++i) {
    items.push(build_item_json(snapshot.inventory[i]));
  }
  payload.add("inventory", std::move(items));
  return payload.finish();
}

std::string build_entities_state_json(const dmcp_snapshot_t& snapshot, size_t offset,
                                      size_t limit) {
  const size_t total = snapshot.entity_count;
  const size_t start = std::min(offset, total);
  const size_t end   = std::min(start + limit, total);

  json_builder payload;
  payload.start_object();
  payload.add("entity_count", static_cast<int64_t>(total));
  payload.add("offset", static_cast<int64_t>(start));
  payload.add("limit", static_cast<int64_t>(limit));
  payload.add("returned", static_cast<int64_t>(end - start));

  json_builder entities;
  entities.start_array();
  for (size_t i = start; i < end; ++i) {
    entities.push(build_entity_json(snapshot.entities[i]));
  }
  payload.add("entities", std::move(entities));
  return payload.finish();
}

bool parse_offset_limit_from_json(const json_value& args, size_t total_count, size_t default_limit,
                                  size_t* out_offset, size_t* out_limit, std::string* out_error) {
  if (!out_offset || !out_limit || !out_error) {
    return false;
  }

  *out_offset = 0;
  *out_limit  = default_limit;
  *out_error  = "Invalid pagination params";

  if (!args.is_object()) {
    return true;
  }

  json_value offset_val = args["offset"];
  if (offset_val) {
    if (!parse_size_from_number(offset_val, out_offset)) {
      *out_error = "offset must be a non-negative integer";
      return false;
    }
  }

  json_value limit_val = args["limit"];
  if (limit_val) {
    if (!parse_size_from_number(limit_val, out_limit) || *out_limit == 0) {
      *out_error = "limit must be a positive integer";
      return false;
    }
  }

  if (*out_offset > total_count) {
    *out_offset = total_count;
  }

  if (*out_limit > total_count && total_count > 0) {
    *out_limit = total_count;
  }

  if (total_count == 0) {
    *out_offset = 0;
  }

  return true;
}

bool build_state_section_payload(const dmcp_snapshot_t& snapshot, std::string_view section,
                                 const json_value& args, std::string* out_payload,
                                 std::string* out_error) {
  if (!out_payload || !out_error) {
    return false;
  }

  constexpr size_t k_default_limit = 64;

  if (section.empty()) {
    *out_error = "section is required (player, enemies, entities, map, inventory, game)";
    return false;
  }

  if (section == "player") {
    *out_payload = build_player_state_json(snapshot);
    return true;
  }

  if (section == "map" || section == "level") {
    *out_payload = build_map_state_json(snapshot);
    return true;
  }

  if (section == "game" || section == "game_info") {
    *out_payload = build_game_info_json(snapshot);
    return true;
  }

  if (section == "enemies") {
    enemy_status_filter filter = enemy_status_filter::alive;
    if (!parse_enemy_status_filter(args, &filter, out_error)) {
      return false;
    }

    size_t offset = 0;
    size_t limit  = k_default_limit;
    if (!parse_offset_limit_from_json(args, snapshot.enemy_count, k_default_limit, &offset, &limit,
                                      out_error)) {
      return false;
    }

    std::string_view status_filter = "alive";
    if (filter == enemy_status_filter::dead) {
      status_filter = "dead";
    } else if (filter == enemy_status_filter::all) {
      status_filter = "all";
    }

    *out_payload = build_enemies_state_json(snapshot, offset, limit, status_filter);
    return true;
  }

  if (section == "entities") {
    size_t offset = 0;
    size_t limit  = k_default_limit;
    if (!parse_offset_limit_from_json(args, snapshot.entity_count, k_default_limit, &offset, &limit,
                                      out_error)) {
      return false;
    }

    *out_payload = build_entities_state_json(snapshot, offset, limit);
    return true;
  }

  if (section == "inventory") {
    size_t offset = 0;
    size_t limit  = k_default_limit;
    if (!parse_offset_limit_from_json(args, snapshot.inventory_count, k_default_limit, &offset,
                                      &limit, out_error)) {
      return false;
    }

    *out_payload = build_inventory_state_json(snapshot, offset, limit);
    return true;
  }

  *out_error = "Unknown section. Use one of: player, enemies, entities, map, inventory, game";
  return false;
}

}  // namespace dmcp
