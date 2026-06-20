#include <cstdint>
#include <string>

#include "doom/internal/json_types.hpp"
#include "doom/internal/json_serializers.hpp"
#include "internal/serialization.hpp"

namespace dmcp {

std::string snapshot_to_json(const dmcp_snapshot_t& snapshot) {
  json_builder root;
  root.start_object();
  root.add("player", json_serializers::player(snapshot.player));
  root.add("level", json_serializers::level(snapshot.level));
  root.add("game", json_serializers::game(snapshot.game));

  json_builder enemies;
  enemies.start_array();
  for (std::uint32_t i = 0; i < snapshot.enemy_count; ++i) {
    enemies.push(json_serializers::enemy(snapshot.enemies[i]));
  }
  root.add("enemies", enemies);
  root.add("enemy_count", static_cast<int64_t>(snapshot.enemy_count));

  json_builder entities;
  entities.start_array();
  for (std::uint32_t i = 0; i < snapshot.entity_count; ++i) {
    entities.push(json_serializers::entity(snapshot.entities[i]));
  }
  root.add("entities", entities);
  root.add("entity_count", static_cast<int64_t>(snapshot.entity_count));

  json_builder inventory;
  inventory.start_array();
  for (std::uint32_t i = 0; i < snapshot.inventory_count; ++i) {
    inventory.push(json_serializers::item(snapshot.inventory[i]));
  }
  root.add("inventory", inventory);
  root.add("inventory_count", static_cast<int64_t>(snapshot.inventory_count));

  return root.finish();
}

}  // namespace dmcp
