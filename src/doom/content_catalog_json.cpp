#include "doom/internal/content_catalog_json.hpp"

#include <cstdint>
#include <utility>

namespace dmcp::json_serializers {
namespace {

json_builder string_array(const std::vector<std::string_view>& values) {
  json_builder arr;
  arr.start_array();
  for (std::string_view value : values) {
    arr.push(value);
  }
  return arr;
}

void add_usage(json_builder* result) {
  json_builder usage;
  usage.start_object();
  usage.add("names", "exact");
  usage.add("spawn_entity", "entity_class from entities plus x,y,angle");
  usage.add("give_item", "item_class from giveable plus amount");
  usage.add("change_level", "map_name from maps");
  usage.add("set_player_position", "x,y,angle");

  json_builder weapon_slots;
  weapon_slots.start_object();
  weapon_slots.add("1", "Fist/Chainsaw");
  weapon_slots.add("2", "Pistol");
  weapon_slots.add("3", "Shotgun/SuperShotgun");
  weapon_slots.add("4", "Chaingun");
  weapon_slots.add("5", "RocketLauncher");
  weapon_slots.add("6", "PlasmaRifle");
  weapon_slots.add("7", "BFG9000");

  json_builder player_input;
  player_input.start_object();
  player_input.add("weapon_slots", std::move(weapon_slots));
  usage.add("player_input", std::move(player_input));

  result->add("usage", std::move(usage));
}

}  // namespace

json_builder content_catalog_payload(const dmcp::content_catalog& catalog) {
  json_builder result;
  result.start_object();
  result.add("catalog_version", static_cast<int64_t>(catalog.catalog_version));

  if (catalog.scope == content_scope::all || catalog.scope == content_scope::enemies) {
    result.add("enemies", string_array(catalog.enemies));
  }
  if (catalog.scope == content_scope::all || catalog.scope == content_scope::entities) {
    result.add("entities", string_array(catalog.entities));
  }
  if (catalog.scope == content_scope::all || catalog.scope == content_scope::items) {
    result.add("items", string_array(catalog.items));
  }
  if (catalog.scope == content_scope::all || catalog.scope == content_scope::weapons) {
    result.add("weapons", string_array(catalog.weapons));
  }
  if (catalog.scope == content_scope::all || catalog.scope == content_scope::ammo) {
    result.add("ammo", string_array(catalog.ammo));
  }
  if (catalog.scope == content_scope::all || catalog.scope == content_scope::keys) {
    result.add("keys", string_array(catalog.keys));
  }
  if (catalog.scope == content_scope::all || catalog.scope == content_scope::maps) {
    result.add("maps", string_array(catalog.maps));
  }
  if (catalog.scope == content_scope::all || catalog.scope == content_scope::giveable) {
    result.add("giveable", string_array(catalog.giveable));
  }

  result.add("game_mode", game_mode_to_string(catalog.game_mode.mode));
  result.add("game_mode_source", catalog.game_mode.source);
  result.add("catalog_scope", content_scope_key(catalog.scope));

  if (catalog.scope == content_scope::all) {
    add_usage(&result);
  }

  return result;
}

}  // namespace dmcp::json_serializers
