#include <cstring>
#include <string>
#include <string_view>

#include "dmcp/doom/protocol.h"
#include "doom/internal/json_types.hpp"
#include "doom/handlers/tools/tools.hpp"

namespace dmcp {

tool_annotation_hints infer_tool_annotation_hints(std::string_view tool_name) {
  tool_annotation_hints hints{};

  if (tool_name == DMCP_TOOL_GET_PLAYER || tool_name == DMCP_TOOL_GET_ENEMIES ||
      tool_name == DMCP_TOOL_GET_ENTITIES || tool_name == DMCP_TOOL_GET_MAP ||
      tool_name == DMCP_TOOL_GET_ITEMS || tool_name == DMCP_TOOL_GET_INVENTORY ||
      tool_name == DMCP_TOOL_GET_GAME_INFO || tool_name == DMCP_TOOL_GET_STATE ||
      tool_name == DMCP_TOOL_GET_STATE_BATCH || tool_name == DMCP_TOOL_GET_SCREENSHOT ||
      tool_name == DMCP_TOOL_GET_COMMAND_RESULT || tool_name == DMCP_TOOL_GET_AVAILABLE_CONTENT ||
      tool_name == DMCP_TOOL_GET_AVAILABLE_ENEMIES ||
      tool_name == DMCP_TOOL_GET_AVAILABLE_ENTITIES || tool_name == DMCP_TOOL_GET_AVAILABLE_ITEMS ||
      tool_name == DMCP_TOOL_GET_AVAILABLE_WEAPONS || tool_name == DMCP_TOOL_GET_AVAILABLE_AMMO ||
      tool_name == DMCP_TOOL_GET_AVAILABLE_KEYS || tool_name == DMCP_TOOL_GET_AVAILABLE_MAPS ||
      tool_name == DMCP_TOOL_GET_AVAILABLE_GIVEABLE ||
      tool_name == DMCP_TOOL_GET_COMMAND_EXAMPLES) {
    hints.read_only  = true;
    hints.idempotent = true;
    return hints;
  }

  if (tool_name == DMCP_TOOL_EXECUTE_BATCH || tool_name == DMCP_TOOL_SPAWN_ENTITY ||
      tool_name == DMCP_TOOL_CHANGE_LEVEL || tool_name == DMCP_TOOL_GIVE_ITEM ||
      tool_name == DMCP_TOOL_SET_PLAYER_HEALTH || tool_name == DMCP_TOOL_SET_PLAYER_POSITION ||
      tool_name == DMCP_TOOL_EXECUTE_CONSOLE || tool_name == DMCP_TOOL_PAUSE_GAME ||
      tool_name == DMCP_TOOL_DAMAGE_ENTITY || tool_name == DMCP_TOOL_KILL_ENTITY ||
      tool_name == DMCP_TOOL_PLAYER_INPUT) {
    hints.destructive = true;
  }

  if (tool_name == DMCP_TOOL_SET_PLAYER_HEALTH || tool_name == DMCP_TOOL_SET_PLAYER_POSITION ||
      tool_name == DMCP_TOOL_PAUSE_GAME) {
    hints.idempotent = true;
  }

  if (tool_name == DMCP_TOOL_EXECUTE_CONSOLE) {
    hints.open_world = true;
  }

  return hints;
}

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
    add_property(&props, "tid", "integer", "Optional Thing ID; 0 lets the engine assign one");
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
    add_property(&props, "health", "number", "Health value");
    required.push("health");
    has_required = true;
  } else if (std::strcmp(tool_name, DMCP_TOOL_SET_PLAYER_POSITION) == 0) {
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
    add_property(&props, "paused", "boolean", "Pause state");
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
                      json_builder schema, tool_annotation_hints hints) {
  if (!tools || !name || !description) {
    return;
  }

  json_builder tool;
  tool.start_object();
  tool.add("name", name);
  tool.add("description", description);
  tool.add("inputSchema", std::move(schema));
  if (hints.read_only || hints.destructive || hints.idempotent || hints.open_world) {
    json_builder annotations;
    annotations.start_object();
    if (hints.read_only) {
      annotations.add("readOnlyHint", true);
    }
    if (hints.destructive) {
      annotations.add("destructiveHint", true);
    }
    if (hints.idempotent) {
      annotations.add("idempotentHint", true);
    }
    if (hints.open_world) {
      annotations.add("openWorldHint", true);
    }
    tool.add("annotations", std::move(annotations));
  }
  tools->push(std::move(tool));
}

}  // namespace dmcp
