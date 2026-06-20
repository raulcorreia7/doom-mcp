#include "tools.hpp"

#include <string_view>

#include "dmcp/doom/protocol.h"
#include "doom/internal/content_catalog.hpp"
#include "doom/internal/content_catalog_json.hpp"

namespace dmcp {

namespace {

std::string_view requested_game_mode(const json_value& params) {
  const json_value args = extract_tool_arguments(params);
  if (!args.is_object()) {
    return {};
  }

  const json_value mode = args["game_mode"];
  if (!mode.is_string()) {
    return {};
  }

  return mode.get_string();
}

bool handle_available_content_scope(context* ctx, const json_value& params, content_scope scope,
                                    const char* tool_name, char* response_buffer,
                                    size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call %s", tool_name);

  const dmcp_snapshot_t      snapshot  = copy_latest_snapshot(ctx);
  const game_mode_resolution mode_info = resolve_game_mode(snapshot, requested_game_mode(params));
  const content_catalog      catalog   = build_content_catalog(snapshot, mode_info, scope);

  json_builder      payload = json_serializers::content_catalog_payload(catalog);
  const std::string text    = payload.finish();
  const std::string resp    = build_content_response(text, false);
  return write_json_response(resp, response_buffer, response_size);
}

}  // namespace

bool handle_tool_get_available_content(context* ctx, const json_value& params,
                                       char* response_buffer, size_t response_size) {
  return handle_available_content_scope(ctx, params, content_scope::all,
                                        DMCP_TOOL_GET_AVAILABLE_CONTENT, response_buffer,
                                        response_size);
}

bool handle_tool_get_available_enemies(context* ctx, const json_value& params,
                                       char* response_buffer, size_t response_size) {
  return handle_available_content_scope(ctx, params, content_scope::enemies,
                                        DMCP_TOOL_GET_AVAILABLE_ENEMIES, response_buffer,
                                        response_size);
}

bool handle_tool_get_available_entities(context* ctx, const json_value& params,
                                        char* response_buffer, size_t response_size) {
  return handle_available_content_scope(ctx, params, content_scope::entities,
                                        DMCP_TOOL_GET_AVAILABLE_ENTITIES, response_buffer,
                                        response_size);
}

bool handle_tool_get_available_items(context* ctx, const json_value& params, char* response_buffer,
                                     size_t response_size) {
  return handle_available_content_scope(ctx, params, content_scope::items,
                                        DMCP_TOOL_GET_AVAILABLE_ITEMS, response_buffer,
                                        response_size);
}

bool handle_tool_get_available_weapons(context* ctx, const json_value& params,
                                       char* response_buffer, size_t response_size) {
  return handle_available_content_scope(ctx, params, content_scope::weapons,
                                        DMCP_TOOL_GET_AVAILABLE_WEAPONS, response_buffer,
                                        response_size);
}

bool handle_tool_get_available_ammo(context* ctx, const json_value& params, char* response_buffer,
                                    size_t response_size) {
  return handle_available_content_scope(ctx, params, content_scope::ammo,
                                        DMCP_TOOL_GET_AVAILABLE_AMMO, response_buffer,
                                        response_size);
}

bool handle_tool_get_available_keys(context* ctx, const json_value& params, char* response_buffer,
                                    size_t response_size) {
  return handle_available_content_scope(ctx, params, content_scope::keys,
                                        DMCP_TOOL_GET_AVAILABLE_KEYS, response_buffer,
                                        response_size);
}

bool handle_tool_get_available_maps(context* ctx, const json_value& params, char* response_buffer,
                                    size_t response_size) {
  return handle_available_content_scope(ctx, params, content_scope::maps,
                                        DMCP_TOOL_GET_AVAILABLE_MAPS, response_buffer,
                                        response_size);
}

bool handle_tool_get_available_giveable(context* ctx, const json_value& params,
                                        char* response_buffer, size_t response_size) {
  return handle_available_content_scope(ctx, params, content_scope::giveable,
                                        DMCP_TOOL_GET_AVAILABLE_GIVEABLE, response_buffer,
                                        response_size);
}

json_builder build_get_available_content_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();

  json_builder mode_prop;
  mode_prop.start_object();
  mode_prop.add("type", "string");
  mode_prop.add("description",
                "Optional game mode override when the running engine has not supplied one: "
                "shareware, registered, commercial, retail");
  json_builder mode_enum;
  mode_enum.start_array();
  mode_enum.push("shareware");
  mode_enum.push("registered");
  mode_enum.push("commercial");
  mode_enum.push("retail");
  mode_prop.add("enum", std::move(mode_enum));
  props.add("game_mode", std::move(mode_prop));

  schema.add("properties", std::move(props));
  return schema;
}

}  // namespace dmcp
