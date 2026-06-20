#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "dmcp/doom/protocol.h"
#include "dmcp/doom/types.h"
#include "doom/internal/content_categories.hpp"
#include "doom/internal/json_serializers.hpp"
#include "doom/internal/json_types.hpp"
#include "doom/handlers/tools/tools.hpp"

namespace dmcp {

namespace {

enum class enemy_status_filter {
  alive,
  dead,
  all,
};

enum class entity_kind_filter {
  all,
  enemy,
  item,
  weapon,
  ammo,
  key,
  health,
  armor,
  powerup,
  world,
};

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
                               std::string*        out_error,
                               enemy_status_filter default_filter = enemy_status_filter::alive) {
  if (!out_filter || !out_error) {
    return false;
  }

  *out_filter = default_filter;

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

std::string_view status_filter_name(enemy_status_filter filter) {
  switch (filter) {
    case enemy_status_filter::alive:
      return status::alive;
    case enemy_status_filter::dead:
      return status::dead;
    case enemy_status_filter::all:
      return status::all;
  }
  return status::all;
}

bool parse_entity_kind_filter(const json_value& args, entity_kind_filter* out_filter,
                              std::string*       out_error,
                              entity_kind_filter default_filter = entity_kind_filter::all) {
  if (!out_filter || !out_error) {
    return false;
  }

  *out_filter = default_filter;
  if (!args.is_object()) {
    return true;
  }

  json_value kind_val = args["kind"];
  if (!kind_val) {
    return true;
  }

  if (!kind_val.is_string()) {
    *out_error =
        "kind must be one of: all, enemy, item, weapon, ammo, key, health, armor, powerup, world";
    return false;
  }

  const std::string_view kind(kind_val.get_string());
  if (kind == DMCP_KIND_ALL) {
    *out_filter = entity_kind_filter::all;
  } else if (kind == DMCP_KIND_ENEMY) {
    *out_filter = entity_kind_filter::enemy;
  } else if (kind == DMCP_KIND_ITEM) {
    *out_filter = entity_kind_filter::item;
  } else if (kind == DMCP_KIND_WEAPON) {
    *out_filter = entity_kind_filter::weapon;
  } else if (kind == DMCP_KIND_AMMO) {
    *out_filter = entity_kind_filter::ammo;
  } else if (kind == DMCP_KIND_KEY) {
    *out_filter = entity_kind_filter::key;
  } else if (kind == DMCP_KIND_HEALTH) {
    *out_filter = entity_kind_filter::health;
  } else if (kind == DMCP_KIND_ARMOR) {
    *out_filter = entity_kind_filter::armor;
  } else if (kind == DMCP_KIND_POWERUP) {
    *out_filter = entity_kind_filter::powerup;
  } else if (kind == DMCP_KIND_WORLD) {
    *out_filter = entity_kind_filter::world;
  } else {
    *out_error =
        "kind must be one of: all, enemy, item, weapon, ammo, key, health, armor, powerup, world";
    return false;
  }

  return true;
}

std::string_view entity_kind_filter_name(entity_kind_filter filter) {
  switch (filter) {
    case entity_kind_filter::all:
      return DMCP_KIND_ALL;
    case entity_kind_filter::enemy:
      return DMCP_KIND_ENEMY;
    case entity_kind_filter::item:
      return DMCP_KIND_ITEM;
    case entity_kind_filter::weapon:
      return DMCP_KIND_WEAPON;
    case entity_kind_filter::ammo:
      return DMCP_KIND_AMMO;
    case entity_kind_filter::key:
      return DMCP_KIND_KEY;
    case entity_kind_filter::health:
      return DMCP_KIND_HEALTH;
    case entity_kind_filter::armor:
      return DMCP_KIND_ARMOR;
    case entity_kind_filter::powerup:
      return DMCP_KIND_POWERUP;
    case entity_kind_filter::world:
      return DMCP_KIND_WORLD;
  }
  return DMCP_KIND_ALL;
}

std::string_view entity_kind_name(const dmcp_entity_t& entity) {
  const std::string_view type(entity.type);
  if (content_categories::is_item(type)) {
    return content_categories::item_kind(type);
  }
  return DMCP_KIND_WORLD;
}

bool entity_matches_kind(const dmcp_entity_t& entity, entity_kind_filter filter) {
  const std::string_view kind = entity_kind_name(entity);
  switch (filter) {
    case entity_kind_filter::all:
      return true;
    case entity_kind_filter::item:
      return content_categories::is_item(entity.type);
    case entity_kind_filter::weapon:
      return kind == DMCP_KIND_WEAPON;
    case entity_kind_filter::ammo:
      return kind == DMCP_KIND_AMMO;
    case entity_kind_filter::key:
      return kind == DMCP_KIND_KEY;
    case entity_kind_filter::health:
      return kind == DMCP_KIND_HEALTH;
    case entity_kind_filter::armor:
      return kind == DMCP_KIND_ARMOR;
    case entity_kind_filter::powerup:
      return kind == DMCP_KIND_POWERUP;
    case entity_kind_filter::world:
      return kind == DMCP_KIND_WORLD;
    case entity_kind_filter::enemy:
      return false;
  }
  return false;
}

json_builder enemy_entity_json(const dmcp_enemy_t& enemy) {
  json_builder obj = json_serializers::enemy(enemy);
  obj.add("kind", DMCP_KIND_ENEMY);
  return obj;
}

json_builder world_entity_json(const dmcp_entity_t& entity) {
  json_builder obj = json_serializers::entity(entity);
  obj.add("kind", entity_kind_name(entity));
  return obj;
}

}  // namespace

std::string build_player_state_json(const dmcp_snapshot_t& snapshot) {
  json_builder payload;
  payload.start_object();
  payload.add("player", json_serializers::player(snapshot.player));
  return payload.finish();
}

std::string build_map_state_json(const dmcp_snapshot_t& snapshot) {
  json_builder payload;
  payload.start_object();
  payload.add("map", json_serializers::level(snapshot.level));
  return payload.finish();
}

std::string build_game_info_json(const dmcp_snapshot_t& snapshot) {
  json_builder payload;
  payload.start_object();
  payload.add("game", json_serializers::game(snapshot.game));
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
    enemies.push(json_serializers::enemy(snapshot.enemies[matched_indexes[i]]));
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
    items.push(json_serializers::item(snapshot.inventory[i]));
  }
  payload.add("inventory", std::move(items));
  return payload.finish();
}

std::string build_entities_state_json(const dmcp_snapshot_t& snapshot, size_t offset, size_t limit,
                                      std::string_view kind_filter,
                                      std::string_view status_filter) {
  entity_kind_filter kind = entity_kind_filter::all;
  if (kind_filter == DMCP_KIND_ENEMY) {
    kind = entity_kind_filter::enemy;
  } else if (kind_filter == DMCP_KIND_ITEM) {
    kind = entity_kind_filter::item;
  } else if (kind_filter == DMCP_KIND_WEAPON) {
    kind = entity_kind_filter::weapon;
  } else if (kind_filter == DMCP_KIND_AMMO) {
    kind = entity_kind_filter::ammo;
  } else if (kind_filter == DMCP_KIND_KEY) {
    kind = entity_kind_filter::key;
  } else if (kind_filter == DMCP_KIND_HEALTH) {
    kind = entity_kind_filter::health;
  } else if (kind_filter == DMCP_KIND_ARMOR) {
    kind = entity_kind_filter::armor;
  } else if (kind_filter == DMCP_KIND_POWERUP) {
    kind = entity_kind_filter::powerup;
  } else if (kind_filter == DMCP_KIND_WORLD) {
    kind = entity_kind_filter::world;
  }

  enemy_status_filter status = enemy_status_filter::all;
  if (status_filter == "alive") {
    status = enemy_status_filter::alive;
  } else if (status_filter == "dead") {
    status = enemy_status_filter::dead;
  }

  struct matched_entity {
    bool   enemy;
    size_t index;
  };

  std::vector<matched_entity> matched;
  matched.reserve(snapshot.enemy_count + snapshot.entity_count);

  if (kind == entity_kind_filter::all || kind == entity_kind_filter::enemy) {
    for (size_t i = 0; i < snapshot.enemy_count; ++i) {
      if (enemy_matches_filter(snapshot.enemies[i], status)) {
        matched.push_back({true, i});
      }
    }
  }

  if (kind != entity_kind_filter::enemy) {
    for (size_t i = 0; i < snapshot.entity_count; ++i) {
      if (entity_matches_kind(snapshot.entities[i], kind)) {
        matched.push_back({false, i});
      }
    }
  }

  const size_t total = matched.size();
  const size_t start = std::min(offset, total);
  const size_t end   = std::min(start + limit, total);

  json_builder payload;
  payload.start_object();
  payload.add("kind", std::string(kind_filter));
  payload.add("status", std::string(status_filter));
  payload.add("entity_count", static_cast<int64_t>(total));
  payload.add("offset", static_cast<int64_t>(start));
  payload.add("limit", static_cast<int64_t>(limit));
  payload.add("returned", static_cast<int64_t>(end - start));

  json_builder entities;
  entities.start_array();
  for (size_t i = start; i < end; ++i) {
    const matched_entity& match = matched[i];
    if (match.enemy) {
      entities.push(enemy_entity_json(snapshot.enemies[match.index]));
    } else {
      entities.push(world_entity_json(snapshot.entities[match.index]));
    }
  }
  payload.add("entities", std::move(entities));
  return payload.finish();
}

std::string build_items_state_json(const dmcp_snapshot_t& snapshot, size_t offset, size_t limit,
                                   std::string_view kind_filter) {
  entity_kind_filter kind = entity_kind_filter::item;
  if (kind_filter == DMCP_KIND_WEAPON) {
    kind = entity_kind_filter::weapon;
  } else if (kind_filter == DMCP_KIND_AMMO) {
    kind = entity_kind_filter::ammo;
  } else if (kind_filter == DMCP_KIND_KEY) {
    kind = entity_kind_filter::key;
  } else if (kind_filter == DMCP_KIND_HEALTH) {
    kind = entity_kind_filter::health;
  } else if (kind_filter == DMCP_KIND_ARMOR) {
    kind = entity_kind_filter::armor;
  } else if (kind_filter == DMCP_KIND_POWERUP) {
    kind = entity_kind_filter::powerup;
  } else if (kind_filter == DMCP_KIND_ALL) {
    kind = entity_kind_filter::item;
  }

  std::vector<size_t> matched_indexes;
  matched_indexes.reserve(snapshot.entity_count);
  for (size_t i = 0; i < snapshot.entity_count; ++i) {
    if (entity_matches_kind(snapshot.entities[i], kind)) {
      matched_indexes.push_back(i);
    }
  }

  const size_t total = matched_indexes.size();
  const size_t start = std::min(offset, total);
  const size_t end   = std::min(start + limit, total);

  json_builder payload;
  payload.start_object();
  const std::string_view response_kind =
      kind_filter == DMCP_KIND_ALL ? std::string_view(DMCP_KIND_ITEM) : kind_filter;
  payload.add("kind", std::string(response_kind));
  payload.add("item_count", static_cast<int64_t>(total));
  payload.add("offset", static_cast<int64_t>(start));
  payload.add("limit", static_cast<int64_t>(limit));
  payload.add("returned", static_cast<int64_t>(end - start));

  json_builder items;
  items.start_array();
  for (size_t i = start; i < end; ++i) {
    items.push(world_entity_json(snapshot.entities[matched_indexes[i]]));
  }
  payload.add("items", std::move(items));
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
    *out_error = "section is required (player, enemies, entities, items, map, inventory, game)";
    return false;
  }

  if (section == "player") {
    *out_payload = build_player_state_json(snapshot);
    return true;
  }

  if (section == "map") {
    *out_payload = build_map_state_json(snapshot);
    return true;
  }

  if (section == "game") {
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

    *out_payload = build_enemies_state_json(snapshot, offset, limit, status_filter_name(filter));
    return true;
  }

  if (section == "entities") {
    entity_kind_filter kind_filter = entity_kind_filter::all;
    if (!parse_entity_kind_filter(args, &kind_filter, out_error)) {
      return false;
    }

    enemy_status_filter status_filter = enemy_status_filter::all;
    if (!parse_enemy_status_filter(args, &status_filter, out_error, enemy_status_filter::all)) {
      return false;
    }

    size_t       offset      = 0;
    size_t       limit       = k_default_limit;
    const size_t total_count = snapshot.enemy_count + snapshot.entity_count;
    if (!parse_offset_limit_from_json(args, total_count, k_default_limit, &offset, &limit,
                                      out_error)) {
      return false;
    }

    *out_payload =
        build_entities_state_json(snapshot, offset, limit, entity_kind_filter_name(kind_filter),
                                  status_filter_name(status_filter));
    return true;
  }

  if (section == "items") {
    entity_kind_filter kind_filter = entity_kind_filter::item;
    if (!parse_entity_kind_filter(args, &kind_filter, out_error, entity_kind_filter::item)) {
      return false;
    }
    if (kind_filter == entity_kind_filter::all || kind_filter == entity_kind_filter::enemy ||
        kind_filter == entity_kind_filter::world) {
      *out_error = "items kind must be one of: item, weapon, ammo, key, health, armor, powerup";
      return false;
    }

    size_t offset = 0;
    size_t limit  = k_default_limit;
    if (!parse_offset_limit_from_json(args, snapshot.entity_count, k_default_limit, &offset, &limit,
                                      out_error)) {
      return false;
    }

    *out_payload =
        build_items_state_json(snapshot, offset, limit, entity_kind_filter_name(kind_filter));
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

  *out_error =
      "Unknown section. Use one of: player, enemies, entities, items, map, inventory, game";
  return false;
}

}  // namespace dmcp
