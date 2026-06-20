#include "tools.hpp"
#include "dmcp/doom/protocol.h"
#include "doom/internal/permissions.hpp"

namespace dmcp {

namespace {

constexpr std::string_view k_game_tool_group  = "game";
constexpr std::string_view k_input_tool_group = "input";

bool write_tool_group_disabled_error(std::string_view tool_name, std::string_view group_name,
                                     char* response_buffer, size_t response_size) {
  std::string message = "Tool '";
  message += tool_name;
  message += "' is unavailable because tool group '";
  message += group_name;
  message += "' is disabled";

  return write_json_response(build_content_response(message, true), response_buffer, response_size);
}

bool is_tool_allowed_by_permissions(const context* ctx, std::string_view tool_name) {
  if (!ctx) {
    return false;
  }

  if (tool_requires_console_permission(tool_name) &&
      !ctx->config.permissions.allow_console_commands) {
    return false;
  }

  if (tool_requires_cheat_permission(tool_name) && !ctx->config.permissions.allow_cheats) {
    return false;
  }

  return true;
}

bool write_permission_disabled_error(std::string_view tool_name, char* response_buffer,
                                     size_t response_size) {
  std::string message = "Tool '";
  message += tool_name;
  message += "' is disabled by DMCP permissions";
  return write_json_response(build_content_response(message, true), response_buffer, response_size);
}

}  // namespace

bool handle_tools_list(void* user_data, const char* /*method*/, const char* /*request_json*/,
                       char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/list requested");

  const bool game_tools_enabled  = ctx && ctx->config.tools.game;
  const bool input_tools_enabled = ctx && ctx->config.tools.input;

  json_builder tools;
  tools.start_array();

  if (game_tools_enabled) {
    add_command_tool(&tools, DMCP_TOOL_GET_PLAYER, "Get current player state only",
                     build_get_player_schema(), infer_tool_annotation_hints(DMCP_TOOL_GET_PLAYER));

    add_command_tool(&tools, DMCP_TOOL_GET_ENEMIES, "Get enemy list with pagination",
                     build_get_enemies_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_ENEMIES));

    add_command_tool(&tools, DMCP_TOOL_GET_ENTITIES,
                     "Get all world entities with kind/status filters", build_get_entities_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_ENTITIES));

    add_command_tool(&tools, DMCP_TOOL_GET_ITEMS, "Get world items with kind filters",
                     build_get_items_schema(), infer_tool_annotation_hints(DMCP_TOOL_GET_ITEMS));

    add_command_tool(&tools, DMCP_TOOL_GET_MAP, "Get current map/level state only",
                     build_get_map_schema(), infer_tool_annotation_hints(DMCP_TOOL_GET_MAP));

    add_command_tool(&tools, DMCP_TOOL_GET_INVENTORY, "Get inventory list with pagination",
                     build_get_inventory_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_INVENTORY));

    add_command_tool(&tools, DMCP_TOOL_GET_GAME_INFO, "Get game mode/version metadata",
                     build_get_game_info_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_GAME_INFO));

    add_command_tool(&tools, DMCP_TOOL_GET_STATE, "Get selected game state section for agents",
                     build_get_state_schema(), infer_tool_annotation_hints(DMCP_TOOL_GET_STATE));

    add_command_tool(&tools, DMCP_TOOL_GET_STATE_BATCH,
                     "Get multiple state sections in one read-only batch request",
                     build_get_state_batch_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_STATE_BATCH));

    if (ctx->screenshot.enabled.load()) {
      json_builder screenshot_schema;
      add_empty_object_schema(&screenshot_schema);
      add_command_tool(
          &tools, DMCP_TOOL_GET_SCREENSHOT, "Capture a screenshot of the current game state",
          std::move(screenshot_schema), infer_tool_annotation_hints(DMCP_TOOL_GET_SCREENSHOT));
    }

    add_command_tool(&tools, DMCP_TOOL_GET_COMMAND_RESULT,
                     "Get asynchronous execution status for a queued command sequence",
                     build_get_command_result_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_COMMAND_RESULT));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_CONTENT,
                     "Get available enemies, spawnable entities, giveable content, and maps",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_CONTENT));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_ENEMIES,
                     "Get available enemy classes for the active game mode",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_ENEMIES));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_ENTITIES,
                     "Get available spawn_entity entity_class values",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_ENTITIES));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_ITEMS,
                     "Get available item pickup classes for the active game mode",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_ITEMS));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_WEAPONS,
                     "Get available weapon classes for the active game mode",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_WEAPONS));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_AMMO,
                     "Get available ammo item classes for the active game mode",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_AMMO));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_KEYS,
                     "Get available key item classes for the active game mode",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_KEYS));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_MAPS,
                     "Get available map names for change_level",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_MAPS));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_GIVEABLE,
                     "Get available give_item item_class values",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_GIVEABLE));

    add_command_tool(
        &tools, DMCP_TOOL_EXECUTE_BATCH, "Execute multiple mutating commands in a single request",
        build_execute_batch_schema(), infer_tool_annotation_hints(DMCP_TOOL_EXECUTE_BATCH));

    add_command_tool(&tools, DMCP_TOOL_GET_COMMAND_EXAMPLES,
                     "Get working examples of all commands including ammo, weapons, items",
                     build_get_command_examples_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_COMMAND_EXAMPLES));

    const command_tool_definition* command_tools = get_command_tools_array();
    size_t                         count         = get_command_tools_count();
    for (size_t i = 0; i < count; ++i) {
      if (!is_tool_allowed_by_permissions(ctx, command_tools[i].tool_name)) {
        continue;
      }

      json_builder schema;
      add_command_tool_schema(command_tools[i].tool_name, &schema);
      add_command_tool(&tools, command_tools[i].tool_name, command_tools[i].description,
                       std::move(schema), infer_tool_annotation_hints(command_tools[i].tool_name));
    }
  }

  if (input_tools_enabled) {
    add_command_tool(&tools, DMCP_TOOL_PLAYER_INPUT,
                     "Send a single player input (movement, turn, attack, use) - one per tick",
                     build_input_schema(), infer_tool_annotation_hints(DMCP_TOOL_PLAYER_INPUT));
  }

  json_builder result;
  result.start_object();
  result.add("tools", std::move(tools));

  const std::string resp = result.finish();
  return write_json_response(resp, response_buffer, response_size);
}

bool handle_tools_call(void* user_data, const char* /*method*/, const char* request_json,
                       char* response_buffer, size_t response_size) {
  auto*      ctx                 = static_cast<context*>(user_data);
  const bool game_tools_enabled  = ctx && ctx->config.tools.game;
  const bool input_tools_enabled = ctx && ctx->config.tools.input;

  json_document doc;
  if (!doc.parse(request_json)) return false;

  json_value params   = doc.root();
  json_value name_val = params["name"];
  if (!name_val.is_string()) {
    const std::string resp = build_content_response("Missing or invalid 'name' field", true);
    return write_json_response(resp, response_buffer, response_size);
  }
  const std::string_view tool_name{name_val.get_string()};
  if (tool_name.empty()) {
    const std::string resp = build_content_response("Tool name cannot be empty", true);
    return write_json_response(resp, response_buffer, response_size);
  }

  if (tool_name == tools::get_screenshot) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_screenshot(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::get_player) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_player(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::get_enemies) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_enemies(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_entities) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_entities(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_items) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_items(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_map) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_map(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::get_inventory) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_inventory(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_game_info) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_game_info(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::get_state) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_state(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_state_batch) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_state_batch(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_command_result) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_command_result(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_content) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_available_content(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_enemies) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_available_enemies(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_entities) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_available_entities(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_items) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_available_items(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_weapons) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_available_weapons(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_ammo) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_available_ammo(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_keys) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_available_keys(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_maps) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_available_maps(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_giveable) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_available_giveable(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::execute_batch) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_execute_batch(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_command_examples) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_get_command_examples(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::player_input) {
    if (!input_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_input_tool_group, response_buffer,
                                             response_size);
    }
    return handle_tool_input(ctx, params, response_buffer, response_size);
  }

  if (const command_tool_definition* command_tool = find_command_tool(tool_name)) {
    if (!game_tools_enabled) {
      return write_tool_group_disabled_error(tool_name, k_game_tool_group, response_buffer,
                                             response_size);
    }
    if (!is_tool_allowed_by_permissions(ctx, tool_name)) {
      return write_permission_disabled_error(tool_name, response_buffer, response_size);
    }
    return handle_tool_command(ctx, command_tool, params, response_buffer, response_size);
  }

  return false;
}

}  // namespace dmcp
