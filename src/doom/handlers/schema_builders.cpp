#include <cstring>
#include <string>

#include "dmcp/doom/protocol.h"
#include "doom/internal/json_types.hpp"
#include "doom/handlers/tools/tools.hpp"

namespace dmcp {

void add_empty_object_schema(json_builder* schema) {
  if (!schema) {
    return;
  }

  schema->start_object();
  schema->add("type", "object");

  json_builder props;
  props.start_object();
  schema->add("properties", std::move(props));
}

void add_property(json_builder* props, const char* key, const char* type, const char* description) {
  if (!props || !key || !type || !description) {
    return;
  }

  json_builder prop;
  prop.start_object();
  prop.add("type", type);
  prop.add("description", description);
  props->add(key, std::move(prop));
}

void add_command_tool_schema(const char* tool_name, json_builder* schema) {
  if (!tool_name || !schema) {
    return;
  }

  schema->start_object();
  schema->add("type", "object");

  json_builder props;
  props.start_object();

  json_builder required;
  required.start_array();
  bool has_required = false;

  if (std::strcmp(tool_name, DMCP_TOOL_SPAWN_ENTITY) == 0) {
    add_property(&props, "entity_class", "string",
                 "Entity class (enemy or pickup), eg DoomImp, Zombieman, Medikit");
    add_property(&props, "x", "number", "Spawn X coordinate");
    add_property(&props, "y", "number", "Spawn Y coordinate");
    add_property(&props, "angle", "number", "Facing angle in degrees");
    required.push("entity_class");
    has_required = true;
  } else if (std::strcmp(tool_name, DMCP_TOOL_CHANGE_LEVEL) == 0) {
    add_property(&props, "map_name", "string", "Map name, eg E1M2 or MAP01");
    add_property(&props, "skill_level", "integer", "Difficulty level 1-5");
    add_property(&props, "reset_inventory", "boolean", "Reset inventory on map load");
    required.push("map_name");
    has_required = true;
  } else if (std::strcmp(tool_name, DMCP_TOOL_GIVE_ITEM) == 0) {
    add_property(&props, "item_class", "string", "Item class, eg Shotgun or Medikit");
    add_property(&props, "amount", "integer", "Quantity to give");
    required.push("item_class");
    has_required = true;
  } else if (std::strcmp(tool_name, DMCP_TOOL_SET_PLAYER_HEALTH) == 0) {
    add_property(&props, "health", "number", "Health value (alias: value)");
    required.push("health");
    has_required = true;
  } else if (std::strcmp(tool_name, DMCP_TOOL_TELEPORT_PLAYER) == 0 ||
             std::strcmp(tool_name, DMCP_TOOL_SET_PLAYER_POSITION) == 0) {
    add_property(&props, "x", "number", "Destination X coordinate");
    add_property(&props, "y", "number", "Destination Y coordinate");
    add_property(&props, "angle", "number", "Facing angle in degrees");
    required.push("x");
    required.push("y");
    has_required = true;
  } else if (std::strcmp(tool_name, DMCP_TOOL_EXECUTE_CONSOLE) == 0) {
    add_property(&props, "command", "string", "Raw console command to execute");
    required.push("command");
    has_required = true;
  } else if (std::strcmp(tool_name, DMCP_TOOL_PAUSE_GAME) == 0) {
    add_property(&props, "paused", "boolean", "Pause state (alias: pause)");
    required.push("paused");
    has_required = true;
  } else if (std::strcmp(tool_name, DMCP_TOOL_DAMAGE_ENTITY) == 0) {
    add_property(&props, "target_tid", "integer", "Target enemy id from game state");
    add_property(&props, "damage", "number", "Damage amount");
    add_property(&props, "damage_type", "string", "Damage type label");
    required.push("target_tid");
    required.push("damage");
    has_required = true;
  } else if (std::strcmp(tool_name, DMCP_TOOL_KILL_ENTITY) == 0) {
    add_property(&props, "target_tid", "integer", "Target enemy id from game state");
    required.push("target_tid");
    has_required = true;
  }

  schema->add("properties", std::move(props));
  if (has_required) {
    schema->add("required", std::move(required));
  }
}

void add_command_tool(json_builder* tools, const char* name, const char* description,
                      json_builder schema) {
  if (!tools || !name || !description) {
    return;
  }

  json_builder tool;
  tool.start_object();
  tool.add("name", name);
  tool.add("description", description);
  tool.add("inputSchema", std::move(schema));
  tools->push(std::move(tool));
}

}  // namespace dmcp
