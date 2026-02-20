#include "tools.hpp"

namespace dmcp {

bool handle_tool_get_command_examples(context* /*ctx*/, char* response_buffer,
                                      size_t response_size) {
  json_builder result;
  result.start_object();

  // spawn_entity
  {
    json_builder example;
    example.start_object();
    example.add("type", "spawn_entity");
    json_builder params;
    params.start_object();
    params.add("entity_class", "DoomImp");
    params.add("x", static_cast<int64_t>(1000));
    params.add("y", static_cast<int64_t>(-500));
    params.add("angle", static_cast<int64_t>(90));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Spawn an enemy at specific coordinates");
    entry.add("example", example);
    result.add("spawn_entity", entry);
  }

  // change_level
  {
    json_builder example;
    example.start_object();
    example.add("type", "change_level");
    json_builder params;
    params.start_object();
    params.add("map_name", "E1M2");
    params.add("skill_level", static_cast<int64_t>(3));
    params.add("reset_inventory", false);
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Change to a different map/level");
    entry.add("example", example);
    result.add("change_level", entry);
  }

  // give_item - weapon
  {
    json_builder example;
    example.start_object();
    example.add("type", "give_item");
    json_builder params;
    params.start_object();
    params.add("item_class", "Shotgun");
    params.add("amount", static_cast<int64_t>(1));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Give a weapon");
    entry.add("example", example);
    result.add("give_item_weapon", entry);
  }

  // give_item - ammo (Clip)
  {
    json_builder example;
    example.start_object();
    example.add("type", "give_item");
    json_builder params;
    params.start_object();
    params.add("item_class", "Clip");
    params.add("amount", static_cast<int64_t>(50));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Give bullets (pistol/chaingun ammo)");
    entry.add("example", example);
    result.add("give_item_ammo_bullets", entry);
  }

  // give_item - ammo (Shells)
  {
    json_builder example;
    example.start_object();
    example.add("type", "give_item");
    json_builder params;
    params.start_object();
    params.add("item_class", "Shells");
    params.add("amount", static_cast<int64_t>(20));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Give shotgun shells");
    entry.add("example", example);
    result.add("give_item_ammo_shells", entry);
  }

  // give_item - ammo (RocketAmmo)
  {
    json_builder example;
    example.start_object();
    example.add("type", "give_item");
    json_builder params;
    params.start_object();
    params.add("item_class", "RocketAmmo");
    params.add("amount", static_cast<int64_t>(10));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Give rockets");
    entry.add("example", example);
    result.add("give_item_ammo_rockets", entry);
  }

  // give_item - ammo (Cell)
  {
    json_builder example;
    example.start_object();
    example.add("type", "give_item");
    json_builder params;
    params.start_object();
    params.add("item_class", "Cell");
    params.add("amount", static_cast<int64_t>(20));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Give plasma cells (BFG/plasma rifle ammo)");
    entry.add("example", example);
    result.add("give_item_ammo_cells", entry);
  }

  // give_item - health
  {
    json_builder example;
    example.start_object();
    example.add("type", "give_item");
    json_builder params;
    params.start_object();
    params.add("item_class", "Medikit");
    params.add("amount", static_cast<int64_t>(1));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Give health item");
    entry.add("example", example);
    result.add("give_item_health", entry);
  }

  // give_item - armor
  {
    json_builder example;
    example.start_object();
    example.add("type", "give_item");
    json_builder params;
    params.start_object();
    params.add("item_class", "BlueArmor");
    params.add("amount", static_cast<int64_t>(1));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Give armor");
    entry.add("example", example);
    result.add("give_item_armor", entry);
  }

  // give_item - powerup
  {
    json_builder example;
    example.start_object();
    example.add("type", "give_item");
    json_builder params;
    params.start_object();
    params.add("item_class", "Invulnerability");
    params.add("amount", static_cast<int64_t>(1));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Give powerup");
    entry.add("example", example);
    result.add("give_item_powerup", entry);
  }

  // set_player_health
  {
    json_builder example;
    example.start_object();
    example.add("type", "set_player_health");
    json_builder params;
    params.start_object();
    params.add("health", static_cast<int64_t>(100));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Set player health to specific value");
    entry.add("example", example);
    result.add("set_player_health", entry);
  }

  // teleport_player
  {
    json_builder example;
    example.start_object();
    example.add("type", "teleport_player");
    json_builder params;
    params.start_object();
    params.add("x", static_cast<int64_t>(1000));
    params.add("y", static_cast<int64_t>(-500));
    params.add("angle", static_cast<int64_t>(180));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Teleport player to coordinates");
    entry.add("example", example);
    result.add("teleport_player", entry);
  }

  // set_player_position
  {
    json_builder example;
    example.start_object();
    example.add("type", "set_player_position");
    json_builder params;
    params.start_object();
    params.add("x", static_cast<int64_t>(1000));
    params.add("y", static_cast<int64_t>(-500));
    params.add("angle", static_cast<int64_t>(0));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Set player position (preserves momentum)");
    entry.add("example", example);
    result.add("set_player_position", entry);
  }

  // execute_console
  {
    json_builder example;
    example.start_object();
    example.add("type", "execute_console");
    json_builder params;
    params.start_object();
    params.add("command", "god");
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Execute raw console command (engine-specific)");
    entry.add("example", example);
    result.add("execute_console", entry);
  }

  // pause_game
  {
    json_builder example;
    example.start_object();
    example.add("type", "pause_game");
    json_builder params;
    params.start_object();
    params.add("paused", true);
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Pause or unpause game simulation");
    entry.add("example", example);
    result.add("pause_game", entry);
  }

  // set_timescale
  {
    json_builder example;
    example.start_object();
    example.add("type", "set_timescale");
    json_builder params;
    params.start_object();
    params.add("scale", 0.5);
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Adjust game simulation speed");
    entry.add("example", example);
    result.add("set_timescale", entry);
  }

  // damage_entity
  {
    json_builder example;
    example.start_object();
    example.add("type", "damage_entity");
    json_builder params;
    params.start_object();
    params.add("target_tid", static_cast<int64_t>(42));
    params.add("damage", static_cast<int64_t>(50));
    params.add("damage_type", "custom");
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Apply damage to a specific enemy (use tid from game state)");
    entry.add("example", example);
    result.add("damage_entity", entry);
  }

  // kill_entity
  {
    json_builder example;
    example.start_object();
    example.add("type", "kill_entity");
    json_builder params;
    params.start_object();
    params.add("target_tid", static_cast<int64_t>(42));
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Instantly kill a specific enemy (use tid from game state)");
    entry.add("example", example);
    result.add("kill_entity", entry);
  }

  // batch execution
  {
    json_builder c1;
    c1.start_object();
    c1.add("type", "give_item");
    json_builder p1;
    p1.start_object();
    p1.add("item_class", "Shotgun");
    p1.add("amount", static_cast<int64_t>(1));
    c1.add("params", p1);

    json_builder c2;
    c2.start_object();
    c2.add("type", "give_item");
    json_builder p2;
    p2.start_object();
    p2.add("item_class", "Shells");
    p2.add("amount", static_cast<int64_t>(20));
    c2.add("params", p2);

    json_builder commands;
    commands.start_array();
    commands.push(c1);
    commands.push(c2);

    json_builder example;
    example.start_object();
    example.add("type", "execute_batch");
    json_builder params;
    params.start_object();
    params.add("commands", commands);
    example.add("params", params);

    json_builder entry;
    entry.start_object();
    entry.add("description", "Execute multiple commands in one request");
    entry.add("example", example);
    result.add("execute_batch", entry);
  }

  const std::string payload = result.finish();
  const std::string resp    = build_content_response(payload, false);
  return write_json_response(resp, response_buffer, response_size);
}

json_builder build_get_command_examples_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();
  schema.add("properties", std::move(props));
  return schema;
}

}  // namespace dmcp
