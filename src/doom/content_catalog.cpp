#include "doom/internal/content_catalog.hpp"

#include <cstddef>
#include <string>

#include "dmcp/doom/content.h"
#include "dmcp/doom/protocol.h"
#include "doom/internal/content_categories.hpp"

namespace dmcp {

namespace {

struct content_list {
  const char* const* values;
  size_t             count;
  bool (*filter)(const char*, dmcp_gamemode_t);
};

void append_filtered(std::vector<std::string_view>* out, const char* const* items, size_t count,
                     dmcp_gamemode_t mode, bool (*filter)(const char*, dmcp_gamemode_t)) {
  if (!out) {
    return;
  }

  out->reserve(out->size() + count);
  for (size_t i = 0; i < count; ++i) {
    if (!filter || filter(items[i], mode)) {
      out->push_back(items[i]);
    }
  }
}

void append_merged(std::vector<std::string_view>* out, const content_list* lists, size_t list_count,
                   dmcp_gamemode_t mode) {
  if (!out) {
    return;
  }

  for (size_t list_idx = 0; list_idx < list_count; ++list_idx) {
    const content_list& list = lists[list_idx];
    append_filtered(out, list.values, list.count, mode, list.filter);
  }
}

void append_base_maps(std::vector<std::string_view>* out, dmcp_gamemode_t mode) {
  if (mode == DMCP_GAMEMODE_COMMERCIAL) {
    append_filtered(out, dmcp_all_maps_doom2, dmcp_all_maps_doom2_count, mode, nullptr);
    return;
  }

  append_filtered(out, dmcp_all_maps_doom1, dmcp_all_maps_doom1_count, mode, dmcp_is_map_available);
}

void append_snapshot_maps(std::vector<std::string_view>* out, const dmcp_snapshot_t& snapshot) {
  if (!out) {
    return;
  }

  out->reserve(out->size() + snapshot.map_count);
  for (uint32_t i = 0; i < snapshot.map_count && i < DMCP_MAX_MAPS; ++i) {
    const char* name = snapshot.maps[i].name;
    if (name[0] != '\0') {
      out->push_back(name);
    }
  }
}

bool snapshot_has_map_catalog(const dmcp_snapshot_t& snapshot) {
  for (uint32_t i = 0; i < snapshot.map_count && i < DMCP_MAX_MAPS; ++i) {
    if (snapshot.maps[i].name[0] != '\0') {
      return true;
    }
  }
  return false;
}

dmcp_gamemode_t parse_mode_name(std::string_view mode_name) {
  if (mode_name == DMCP_MODE_SHAREWARE) return DMCP_GAMEMODE_SHAREWARE;
  if (mode_name == DMCP_MODE_REGISTERED) return DMCP_GAMEMODE_REGISTERED;
  if (mode_name == DMCP_MODE_COMMERCIAL || mode_name == DMCP_MODE_DOOM2) {
    return DMCP_GAMEMODE_COMMERCIAL;
  }
  if (mode_name == DMCP_MODE_RETAIL || mode_name == DMCP_MODE_ULTIMATE) {
    return DMCP_GAMEMODE_RETAIL;
  }
  return DMCP_GAMEMODE_UNKNOWN;
}

}  // namespace

const char* content_scope_key(content_scope scope) {
  switch (scope) {
    case content_scope::enemies:
      return "enemies";
    case content_scope::entities:
      return "entities";
    case content_scope::items:
      return "items";
    case content_scope::weapons:
      return "weapons";
    case content_scope::ammo:
      return "ammo";
    case content_scope::keys:
      return "keys";
    case content_scope::maps:
      return "maps";
    case content_scope::giveable:
      return "giveable";
    case content_scope::all:
    default:
      return "all";
  }
}

const char* game_mode_to_string(dmcp_gamemode_t mode) {
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

game_mode_resolution resolve_game_mode(const dmcp_snapshot_t& snapshot,
                                       std::string_view       requested_mode) {
  if (snapshot.game.version[0] != '\0') {
    const dmcp_gamemode_t mode = parse_mode_name(snapshot.game.version);
    if (mode != DMCP_GAMEMODE_UNKNOWN) {
      return {mode, "snapshot"};
    }
  }

  if (!requested_mode.empty()) {
    const dmcp_gamemode_t mode = parse_mode_name(requested_mode);
    if (mode != DMCP_GAMEMODE_UNKNOWN) {
      return {mode, "parameter"};
    }
  }

  return {DMCP_GAMEMODE_RETAIL, "fallback"};
}

content_catalog build_content_catalog(const dmcp_snapshot_t& snapshot, game_mode_resolution mode,
                                      content_scope scope) {
  content_catalog catalog;
  catalog.game_mode = mode;
  catalog.scope     = scope;

  const bool include_all = scope == content_scope::all;

  if (include_all || scope == content_scope::enemies) {
    append_filtered(&catalog.enemies, dmcp_all_enemies, dmcp_all_enemies_count, mode.mode,
                    dmcp_is_enemy_spawnable);
  }

  if (include_all || scope == content_scope::weapons) {
    append_filtered(&catalog.weapons, dmcp_all_weapons, dmcp_all_weapons_count, mode.mode,
                    dmcp_is_weapon_available);
  }

  if (include_all || scope == content_scope::items) {
    append_filtered(&catalog.items, dmcp_all_items, dmcp_all_items_count, mode.mode,
                    dmcp_is_item_available);
  }

  if (include_all || scope == content_scope::ammo) {
    append_filtered(&catalog.ammo, content_categories::ammo_items.data(),
                    content_categories::ammo_items.size(), mode.mode, dmcp_is_item_available);
  }

  if (include_all || scope == content_scope::keys) {
    append_filtered(&catalog.keys, content_categories::key_items.data(),
                    content_categories::key_items.size(), mode.mode, dmcp_is_item_available);
  }

  if (include_all || scope == content_scope::maps) {
    if (snapshot_has_map_catalog(snapshot)) {
      append_snapshot_maps(&catalog.maps, snapshot);
    } else {
      append_base_maps(&catalog.maps, mode.mode);
    }
  }

  if (include_all || scope == content_scope::giveable) {
    const content_list giveable_lists[] = {
        {dmcp_all_weapons, dmcp_all_weapons_count, dmcp_is_weapon_available},
        {dmcp_all_items, dmcp_all_items_count, dmcp_is_item_available},
    };
    append_merged(&catalog.giveable, giveable_lists,
                  sizeof(giveable_lists) / sizeof(giveable_lists[0]), mode.mode);
  }

  if (include_all || scope == content_scope::entities) {
    const content_list entity_lists[] = {
        {dmcp_all_enemies, dmcp_all_enemies_count, dmcp_is_enemy_spawnable},
        {content_categories::spawnable_weapon_items.data(),
         content_categories::spawnable_weapon_items.size(), dmcp_is_weapon_available},
        {dmcp_all_items, dmcp_all_items_count, dmcp_is_item_available},
    };
    append_merged(&catalog.entities, entity_lists, sizeof(entity_lists) / sizeof(entity_lists[0]),
                  mode.mode);
  }

  return catalog;
}

bool map_is_available_for_snapshot(const dmcp_snapshot_t& snapshot, std::string_view map_name,
                                   dmcp_gamemode_t mode) {
  if (snapshot_has_map_catalog(snapshot)) {
    for (uint32_t i = 0; i < snapshot.map_count && i < DMCP_MAX_MAPS; ++i) {
      if (std::string_view(snapshot.maps[i].name) == map_name) {
        return true;
      }
    }
    return false;
  }

  const std::string map_name_text(map_name);
  return dmcp_is_map_available(map_name_text.c_str(), mode);
}

}  // namespace dmcp
