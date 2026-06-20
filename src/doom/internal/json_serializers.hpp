#pragma once

#include "dmcp/doom/types.h"
#include "doom/internal/json_types.hpp"

namespace dmcp::json_serializers {

json_builder vec3(const dmcp_vec3_t& position);
json_builder player(const dmcp_player_t& player);
json_builder level(const dmcp_level_t& level);
json_builder game(const dmcp_game_t& game);
json_builder enemy(const dmcp_enemy_t& enemy);
json_builder item(const dmcp_item_t& item);
json_builder entity(const dmcp_entity_t& entity);

}  // namespace dmcp::json_serializers
