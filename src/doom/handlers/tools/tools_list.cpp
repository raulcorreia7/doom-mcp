#include "tools.hpp"
#include "dmcp/doom/layer.h"
#include "dmcp/doom/protocol.h"
#include "doom/layers/registry.hpp"
#include "mcp/generic/constants.h"

namespace dmcp {

namespace {

bool is_layer_enabled(const context* ctx, const char* layer_name) {
  if (!ctx || !ctx->layers || !layer_name) {
    return false;
  }

  return ctx->layers->is_enabled(layer_name);
}

bool write_layer_disabled_error(std::string_view tool_name, std::string_view layer_name,
                                char* response_buffer, size_t response_size) {
  std::string message = "Tool '";
  message += tool_name;
  message += "' is unavailable because layer '";
  message += layer_name;
  message += "' is disabled";

  return write_json_response(build_content_response(message, true), response_buffer, response_size);
}

}  // namespace

bool handle_tools_list(void* user_data, const char* /*method*/, const char* /*request_json*/,
                       char* response_buffer, size_t response_size) {
  auto* ctx = static_cast<context*>(user_data);
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/list requested");

  const bool orchestrator_enabled = is_layer_enabled(ctx, DMCP_LAYER_ORCHESTRATOR);
  const bool input_enabled        = is_layer_enabled(ctx, DMCP_LAYER_INPUT);

  json_builder tools;
  tools.start_array();

  if (orchestrator_enabled) {
    add_command_tool(&tools, DMCP_TOOL_GET_PLAYER, "Get current player state only",
                     build_get_player_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_PLAYER));

    add_command_tool(&tools, DMCP_TOOL_GET_ENEMIES, "Get enemy list with pagination",
                     build_get_enemies_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_ENEMIES));

    add_command_tool(&tools, DMCP_TOOL_GET_ENTITIES,
                     "Get interactive world entities (pickups/barrels), paginated",
                     build_get_entities_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_ENTITIES));

    add_command_tool(&tools, DMCP_TOOL_GET_MAP, "Get current map/level state only",
                     build_get_map_schema(), infer_tool_annotation_hints(DMCP_TOOL_GET_MAP));

    add_command_tool(&tools, DMCP_TOOL_GET_LEVEL, "Alias for get_map", build_get_map_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_LEVEL));

    add_command_tool(&tools, DMCP_TOOL_GET_INVENTORY, "Get inventory list with pagination",
                     build_get_inventory_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_INVENTORY));

    add_command_tool(&tools, DMCP_TOOL_GET_GAME_INFO, "Get game mode/version metadata",
                     build_get_game_info_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_GAME_INFO));

    add_command_tool(&tools, DMCP_TOOL_GET_GAME, "Alias for get_game_info",
                     build_get_game_info_schema(), infer_tool_annotation_hints(DMCP_TOOL_GET_GAME));

    add_command_tool(&tools, DMCP_TOOL_GET_STATE, "Get selected game state section for agents",
                     build_get_state_schema(), infer_tool_annotation_hints(DMCP_TOOL_GET_STATE));

    add_command_tool(&tools, DMCP_TOOL_GET_STATE_BATCH,
                     "Get multiple state sections in one read-only batch request",
                     build_get_state_batch_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_STATE_BATCH));

    if (ctx->screenshot.enabled.load()) {
      json_builder screenshot_schema;
      add_empty_object_schema(&screenshot_schema);
      add_command_tool(&tools, DMCP_TOOL_GET_SCREENSHOT,
                       "Capture a screenshot of the current game state",
                       std::move(screenshot_schema),
                       infer_tool_annotation_hints(DMCP_TOOL_GET_SCREENSHOT));
    }

    add_command_tool(&tools, DMCP_TOOL_EXECUTE_COMMAND,
                     "Execute a game command (spawn enemy, change level, etc.)",
                     build_execute_command_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_EXECUTE_COMMAND));

    add_command_tool(&tools, DMCP_TOOL_GET_COMMAND_RESULT,
                     "Get asynchronous execution status for a queued command sequence",
                     build_get_command_result_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_COMMAND_RESULT));

    add_command_tool(&tools, DMCP_TOOL_GET_AVAILABLE_CONTENT,
                     "Get available weapons, items, enemies, and maps for the game mode",
                     build_get_available_content_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_AVAILABLE_CONTENT));

    add_command_tool(&tools, DMCP_TOOL_EXECUTE_BATCH,
                     "Execute multiple mutating commands in a single request",
                     build_execute_batch_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_EXECUTE_BATCH));

    add_command_tool(&tools, DMCP_TOOL_GET_COMMAND_EXAMPLES,
                     "Get working examples of all commands including ammo, weapons, items",
                     build_get_command_examples_schema(),
                     infer_tool_annotation_hints(DMCP_TOOL_GET_COMMAND_EXAMPLES));

    const command_tool_definition* command_tools = get_command_tools_array();
    size_t                         count         = get_command_tools_count();
    for (size_t i = 0; i < count; ++i) {
      json_builder schema;
      add_command_tool_schema(command_tools[i].tool_name, &schema);
      add_command_tool(&tools, command_tools[i].tool_name, command_tools[i].description,
                       std::move(schema),
                       infer_tool_annotation_hints(command_tools[i].tool_name));
    }
  }

  if (input_enabled) {
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
  auto*      ctx                  = static_cast<context*>(user_data);
  const bool orchestrator_enabled = is_layer_enabled(ctx, DMCP_LAYER_ORCHESTRATOR);
  const bool input_enabled        = is_layer_enabled(ctx, DMCP_LAYER_INPUT);

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
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_screenshot(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::get_player) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_player(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::get_enemies) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_enemies(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_entities) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_entities(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_map || tool_name == tools::get_level) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_map(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::get_inventory) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_inventory(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_game_info || tool_name == tools::get_game) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_game_info(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::get_state) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_state(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_state_batch) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_state_batch(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::execute_command) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_execute_command(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_command_result) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_command_result(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_available_content) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_available_content(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::execute_batch) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_execute_batch(ctx, params, response_buffer, response_size);
  }

  if (tool_name == tools::get_command_examples) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_get_command_examples(ctx, response_buffer, response_size);
  }

  if (tool_name == tools::player_input) {
    if (!input_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_INPUT, response_buffer,
                                        response_size);
    }
    return handle_tool_input(ctx, params, response_buffer, response_size);
  }

  if (const command_tool_definition* command_tool = find_command_tool(tool_name)) {
    if (!orchestrator_enabled) {
      return write_layer_disabled_error(tool_name, DMCP_LAYER_ORCHESTRATOR, response_buffer,
                                        response_size);
    }
    return handle_tool_command_alias(ctx, command_tool, params, response_buffer, response_size);
  }

  return false;
}

}  // namespace dmcp
