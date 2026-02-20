#include "tools.hpp"

namespace dmcp {

namespace {

// Helper to build a single parameter entry
void add_param(json_builder* params, const char* key, const char* value) {
  params->add(key, value);
}

void add_param(json_builder* params, const char* key, int64_t value) { params->add(key, value); }

void add_param(json_builder* params, const char* key, bool value) { params->add(key, value); }

// Build a command example entry with description
json_builder build_entry(const char* description) {
  json_builder entry;
  entry.start_object();
  entry.add("description", description);
  return entry;
}

// Build example object with type and params
json_builder build_example(const char* type) {
  json_builder example;
  example.start_object();
  example.add("type", type);
  return example;
}

// Add params to example and attach to entry
void add_example_to_entry(json_builder* entry, json_builder* example, json_builder* params) {
  example->add("params", *params);
  entry->add("example", *example);
}

// Command with single string param
void add_string_command(json_builder* result, const char* key, const char* type,
                        const char* description, const char* param_key, const char* param_value) {
  json_builder entry   = build_entry(description);
  json_builder example = build_example(type);
  json_builder params;
  params.start_object();
  add_param(&params, param_key, param_value);
  add_example_to_entry(&entry, &example, &params);
  result->add(key, entry);
}

// Command with single int64 param
void add_int_command(json_builder* result, const char* key, const char* type,
                     const char* description, const char* param_key, int64_t param_value) {
  json_builder entry   = build_entry(description);
  json_builder example = build_example(type);
  json_builder params;
  params.start_object();
  add_param(&params, param_key, param_value);
  add_example_to_entry(&entry, &example, &params);
  result->add(key, entry);
}

// Command with single bool param
void add_bool_command(json_builder* result, const char* key, const char* type,
                      const char* description, const char* param_key, bool param_value) {
  json_builder entry   = build_entry(description);
  json_builder example = build_example(type);
  json_builder params;
  params.start_object();
  add_param(&params, param_key, param_value);
  add_example_to_entry(&entry, &example, &params);
  result->add(key, entry);
}

// give_item with item_class and amount
void add_give_item_example(json_builder* result, const char* key, const char* item_class,
                           int64_t amount, const char* description) {
  json_builder entry   = build_entry(description);
  json_builder example = build_example("give_item");
  json_builder params;
  params.start_object();
  add_param(&params, "item_class", item_class);
  add_param(&params, "amount", amount);
  add_example_to_entry(&entry, &example, &params);
  result->add(key, entry);
}

// spawn_entity specific helper
void add_spawn_entity(json_builder* result) {
  json_builder entry   = build_entry("Spawn an enemy at specific coordinates");
  json_builder example = build_example("spawn_entity");
  json_builder params;
  params.start_object();
  add_param(&params, "entity_class", "DoomImp");
  add_param(&params, "x", static_cast<int64_t>(1000));
  add_param(&params, "y", static_cast<int64_t>(-500));
  add_param(&params, "angle", static_cast<int64_t>(90));
  add_example_to_entry(&entry, &example, &params);
  result->add("spawn_entity", entry);
}

void add_spawn_item(json_builder* result) {
  json_builder entry   = build_entry("Spawn a pickup item at specific coordinates");
  json_builder example = build_example("spawn_entity");
  json_builder params;
  params.start_object();
  add_param(&params, "entity_class", "Medikit");
  add_param(&params, "x", static_cast<int64_t>(1000));
  add_param(&params, "y", static_cast<int64_t>(-500));
  add_param(&params, "angle", static_cast<int64_t>(0));
  add_example_to_entry(&entry, &example, &params);
  result->add("spawn_item", entry);
}

// change_level specific helper
void add_change_level(json_builder* result) {
  json_builder entry   = build_entry("Change to a different map/level");
  json_builder example = build_example("change_level");
  json_builder params;
  params.start_object();
  add_param(&params, "map_name", "E1M2");
  add_param(&params, "skill_level", static_cast<int64_t>(3));
  add_param(&params, "reset_inventory", false);
  add_example_to_entry(&entry, &example, &params);
  result->add("change_level", entry);
}

// teleport_player / set_player_position helper
void add_position_command(json_builder* result, const char* key, const char* type,
                          const char* description, int64_t angle) {
  json_builder entry   = build_entry(description);
  json_builder example = build_example(type);
  json_builder params;
  params.start_object();
  add_param(&params, "x", static_cast<int64_t>(1000));
  add_param(&params, "y", static_cast<int64_t>(-500));
  add_param(&params, "angle", angle);
  add_example_to_entry(&entry, &example, &params);
  result->add(key, entry);
}

// damage_entity specific helper
void add_damage_entity(json_builder* result) {
  json_builder entry   = build_entry("Apply damage to a specific enemy (use tid from game state)");
  json_builder example = build_example("damage_entity");
  json_builder params;
  params.start_object();
  add_param(&params, "target_tid", static_cast<int64_t>(42));
  add_param(&params, "damage", static_cast<int64_t>(50));
  add_param(&params, "damage_type", "custom");
  add_example_to_entry(&entry, &example, &params);
  result->add("damage_entity", entry);
}

// kill_entity specific helper
void add_kill_entity(json_builder* result) {
  json_builder entry   = build_entry("Instantly kill a specific enemy (use tid from game state)");
  json_builder example = build_example("kill_entity");
  json_builder params;
  params.start_object();
  add_param(&params, "target_tid", static_cast<int64_t>(42));
  add_example_to_entry(&entry, &example, &params);
  result->add("kill_entity", entry);
}

// batch execution helper
void add_batch_execution(json_builder* result) {
  // Build individual commands
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

  // Build the example
  json_builder entry = build_entry(
      "Execute multiple mutating commands in one request (do not mix with read batches)");
  json_builder example = build_example("execute_batch");
  json_builder params;
  params.start_object();
  params.add("commands", commands);
  add_example_to_entry(&entry, &example, &params);
  result->add("execute_batch", entry);
}

void add_get_state_batch(json_builder* result) {
  json_builder r1;
  r1.start_object();
  r1.add("section", "player");

  json_builder r2;
  r2.start_object();
  r2.add("section", "enemies");
  r2.add("status", "alive");
  r2.add("limit", static_cast<int64_t>(8));

  json_builder requests;
  requests.start_array();
  requests.push(r1);
  requests.push(r2);

  json_builder entry   = build_entry("Read multiple state sections in one read-only batch");
  json_builder example = build_example("get_state_batch");
  json_builder params;
  params.start_object();
  params.add("requests", requests);
  add_example_to_entry(&entry, &example, &params);
  result->add("get_state_batch", entry);
}

}  // namespace

bool handle_tool_get_command_examples(context* /*ctx*/, char* response_buffer,
                                      size_t response_size) {
  json_builder result;
  result.start_object();

  // Entity commands
  add_spawn_entity(&result);
  add_spawn_item(&result);
  add_damage_entity(&result);
  add_kill_entity(&result);

  // Level commands
  add_change_level(&result);

  // give_item examples
  add_give_item_example(&result, "give_item_weapon", "Shotgun", 1, "Give a weapon");
  add_give_item_example(&result, "give_item_ammo_bullets", "Clip", 50,
                        "Give bullets (pistol/chaingun ammo)");
  add_give_item_example(&result, "give_item_ammo_shells", "Shells", 20, "Give shotgun shells");
  add_give_item_example(&result, "give_item_ammo_rockets", "RocketAmmo", 10, "Give rockets");
  add_give_item_example(&result, "give_item_ammo_cells", "Cell", 20,
                        "Give plasma cells (BFG/plasma rifle ammo)");
  add_give_item_example(&result, "give_item_health", "Medikit", 1, "Give health item");
  add_give_item_example(&result, "give_item_armor", "BlueArmor", 1, "Give armor");
  add_give_item_example(&result, "give_item_powerup", "Invulnerability", 1, "Give powerup");

  // Player commands
  add_int_command(&result, "set_player_health", "set_player_health",
                  "Set player health to specific value", "health", 100);
  add_position_command(&result, "teleport_player", "teleport_player",
                       "Teleport player to coordinates", 180);
  add_position_command(&result, "set_player_position", "set_player_position",
                       "Set player position (preserves momentum)", 0);

  // System commands
  add_string_command(&result, "execute_console", "execute_console",
                     "Execute raw console command (engine-specific)", "command", "god");
  add_bool_command(&result, "pause_game", "pause_game", "Pause or unpause game simulation",
                   "paused", true);

  // Batch execution
  add_batch_execution(&result);
  add_get_state_batch(&result);

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
