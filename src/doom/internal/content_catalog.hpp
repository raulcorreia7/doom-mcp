#pragma once

#include <string_view>
#include <vector>

#include "dmcp/doom/types.h"

namespace dmcp {

enum class content_scope {
  all,
  enemies,
  entities,
  items,
  weapons,
  ammo,
  keys,
  maps,
  giveable,
};

struct game_mode_resolution {
  dmcp_gamemode_t mode   = DMCP_GAMEMODE_UNKNOWN;
  const char*     source = "unknown";
};

struct content_catalog {
  int                  catalog_version = 1;
  game_mode_resolution game_mode{};
  content_scope        scope = content_scope::all;

  std::vector<std::string_view> enemies;
  std::vector<std::string_view> entities;
  std::vector<std::string_view> items;
  std::vector<std::string_view> weapons;
  std::vector<std::string_view> ammo;
  std::vector<std::string_view> keys;
  std::vector<std::string_view> maps;
  std::vector<std::string_view> giveable;
};

const char* content_scope_key(content_scope scope);
const char* game_mode_to_string(dmcp_gamemode_t mode);

game_mode_resolution resolve_game_mode(const dmcp_snapshot_t& snapshot,
                                       std::string_view       requested_mode);

content_catalog build_content_catalog(const dmcp_snapshot_t& snapshot, game_mode_resolution mode,
                                      content_scope scope);

bool map_is_available_for_snapshot(const dmcp_snapshot_t& snapshot, std::string_view map_name,
                                   dmcp_gamemode_t mode);

}  // namespace dmcp
