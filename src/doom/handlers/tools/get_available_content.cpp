#include "tools.hpp"
#include "dmcp/doom/content.h"

#include <string>
#include <vector>

namespace dmcp {

static json_builder build_content_array(const char* const* items, size_t count,
                                        dmcp_gamemode_t mode,
                                        bool (*filter)(const char*, dmcp_gamemode_t)) {
  json_builder arr;
  arr.start_array();

  for (size_t i = 0; i < count; ++i) {
    if (!filter || filter(items[i], mode)) {
      arr.push(items[i]);
    }
  }

  return arr;
}

namespace {

struct game_mode_resolution {
  dmcp_gamemode_t mode;
  const char*     source;
};

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

void push_unique_string(json_builder* arr, std::vector<std::string>* seen, const char* value) {
  if (!arr || !seen || !value || value[0] == '\0') {
    return;
  }

  for (const std::string& existing : *seen) {
    if (existing == value) {
      return;
    }
  }

  seen->emplace_back(value);
  arr->push(value);
}

// Parse game mode from string
static dmcp_gamemode_t parse_game_mode(std::string_view mode_str) {
  if (mode_str == "shareware") return DMCP_GAMEMODE_SHAREWARE;
  if (mode_str == "registered") return DMCP_GAMEMODE_REGISTERED;
  if (mode_str == "commercial" || mode_str == "doom2") return DMCP_GAMEMODE_COMMERCIAL;
  if (mode_str == "retail" || mode_str == "ultimate") return DMCP_GAMEMODE_RETAIL;
  return DMCP_GAMEMODE_UNKNOWN;
}

// Get game mode from snapshot, fallback to parameter, then retail
static game_mode_resolution determine_game_mode(context* ctx, const json_value& params) {
  // First priority: use actual game mode from snapshot
  dmcp_gamemode_t snapshot_mode = DMCP_GAMEMODE_UNKNOWN;
  {
    std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
    const dmcp_game_t&          game = ctx->last_snapshot.game;
    if (game.version[0] != '\0') {
      snapshot_mode = parse_game_mode(game.version);
      if (snapshot_mode != DMCP_GAMEMODE_UNKNOWN) {
        return {snapshot_mode, "snapshot"};
      }
    }
  }

  // Second priority: use parameter override
  json_value args = extract_tool_arguments(params);
  if (args.is_object()) {
    json_value mode_val = args["game_mode"];
    if (mode_val.is_string()) {
      dmcp_gamemode_t param_mode = parse_game_mode(mode_val.get_string());
      if (param_mode != DMCP_GAMEMODE_UNKNOWN) {
        return {param_mode, "parameter"};
      }
    }
  }

  // Final fallback: retail mode
  return {DMCP_GAMEMODE_RETAIL, "fallback"};
}

}  // namespace

bool handle_tool_get_available_content(context* ctx, const json_value& params,
                                       char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_available_content");

  const game_mode_resolution mode_info = determine_game_mode(ctx, params);
  const dmcp_gamemode_t      mode      = mode_info.mode;

  dmcp_snapshot_t snapshot{};
  {
    std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
    snapshot = ctx->last_snapshot;
  }

  json_builder result;
  result.start_object();

  json_builder weapons =
      build_content_array(dmcp_all_weapons, dmcp_all_weapons_count, mode, dmcp_is_weapon_available);
  result.add("weapons", std::move(weapons));

  json_builder items =
      build_content_array(dmcp_all_items, dmcp_all_items_count, mode, dmcp_is_item_available);
  result.add("items", std::move(items));

  json_builder enemies =
      build_content_array(dmcp_all_enemies, dmcp_all_enemies_count, mode, dmcp_is_enemy_spawnable);
  result.add("enemies", std::move(enemies));

  if (mode == DMCP_GAMEMODE_COMMERCIAL) {
    json_builder maps;
    maps.start_array();
    for (size_t i = 0; i < dmcp_all_maps_doom2_count; ++i) {
      maps.push(dmcp_all_maps_doom2[i]);
    }
    result.add("maps", std::move(maps));
  } else {
    json_builder maps = build_content_array(dmcp_all_maps_doom1, dmcp_all_maps_doom1_count, mode,
                                            dmcp_is_map_available);
    result.add("maps", std::move(maps));
  }

  result.add("game_mode", game_mode_to_string(mode));
  result.add("game_mode_source", mode_info.source);

  if (snapshot.level.level_id[0] != '\0') {
    result.add("current_level", snapshot.level.level_id);
  }

  json_builder observed_enemies;
  observed_enemies.start_array();
  std::vector<std::string> seen_enemies;
  for (uint32_t i = 0; i < snapshot.enemy_count; ++i) {
    push_unique_string(&observed_enemies, &seen_enemies, snapshot.enemies[i].type);
  }
  result.add("observed_enemies", std::move(observed_enemies));

  json_builder observed_items;
  observed_items.start_array();
  std::vector<std::string> seen_items;
  for (uint32_t i = 0; i < snapshot.inventory_count; ++i) {
    push_unique_string(&observed_items, &seen_items, snapshot.inventory[i].name);
  }
  result.add("observed_items", std::move(observed_items));
  result.add("catalog_scope", "base-content-catalog");
  result.add("custom_content_policy",
             "Unknown custom content may still be executable; validate via command results");

  const std::string payload = result.finish();
  const std::string resp    = build_content_response(payload, false);
  return write_json_response(resp, response_buffer, response_size);
}

json_builder build_get_available_content_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();

  json_builder mode_prop;
  mode_prop.start_object();
  mode_prop.add("type", "string");
  mode_prop.add("description",
                "Optional override for game mode (auto-detected from running game if not "
                "specified): shareware, registered, commercial, retail");
  json_builder mode_enum;
  mode_enum.start_array();
  mode_enum.push("shareware");
  mode_enum.push("registered");
  mode_enum.push("commercial");
  mode_enum.push("retail");
  mode_prop.add("enum", std::move(mode_enum));
  props.add("game_mode", std::move(mode_prop));

  schema.add("properties", std::move(props));
  return schema;
}

}  // namespace dmcp
