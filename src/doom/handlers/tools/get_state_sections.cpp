#include "tools.hpp"

namespace dmcp {

namespace {

bool write_tool_payload(std::string_view payload, char* response_buffer, size_t response_size) {
  const std::string resp = build_content_response(payload, false);
  return write_json_response(resp, response_buffer, response_size);
}

json_builder build_pagination_schema(const char* description) {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();

  json_builder offset_prop;
  offset_prop.start_object();
  offset_prop.add("type", "integer");
  offset_prop.add("description", "Zero-based pagination offset");
  props.add("offset", std::move(offset_prop));

  json_builder limit_prop;
  limit_prop.start_object();
  limit_prop.add("type", "integer");
  limit_prop.add("description", description);
  props.add("limit", std::move(limit_prop));

  schema.add("properties", std::move(props));
  return schema;
}

}  // namespace

bool handle_tool_get_player(context* ctx, char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_player");
  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  std::string           payload;
  std::string           error;
  json_value            args;
  if (!build_state_section_payload(snapshot, "player", args, &payload, &error)) {
    return write_json_response(build_content_response(error, true), response_buffer, response_size);
  }

  return write_tool_payload(payload, response_buffer, response_size);
}

bool handle_tool_get_map(context* ctx, char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_map");
  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  std::string           payload;
  std::string           error;
  json_value            args;
  if (!build_state_section_payload(snapshot, "map", args, &payload, &error)) {
    return write_json_response(build_content_response(error, true), response_buffer, response_size);
  }

  return write_tool_payload(payload, response_buffer, response_size);
}

bool handle_tool_get_game_info(context* ctx, char* response_buffer, size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_game_info");
  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  std::string           payload;
  std::string           error;
  json_value            args;
  if (!build_state_section_payload(snapshot, "game", args, &payload, &error)) {
    return write_json_response(build_content_response(error, true), response_buffer, response_size);
  }

  return write_tool_payload(payload, response_buffer, response_size);
}

bool handle_tool_get_enemies(context* ctx, const json_value& params, char* response_buffer,
                             size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_enemies");

  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  const json_value      args     = extract_tool_arguments(params);

  std::string payload;
  std::string error;
  if (!build_state_section_payload(snapshot, "enemies", args, &payload, &error)) {
    const std::string resp = build_content_response(error, true);
    return write_json_response(resp, response_buffer, response_size);
  }

  return write_tool_payload(payload, response_buffer, response_size);
}

bool handle_tool_get_entities(context* ctx, const json_value& params, char* response_buffer,
                              size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_entities");

  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  const json_value      args     = extract_tool_arguments(params);

  std::string payload;
  std::string error;
  if (!build_state_section_payload(snapshot, "entities", args, &payload, &error)) {
    const std::string resp = build_content_response(error, true);
    return write_json_response(resp, response_buffer, response_size);
  }

  return write_tool_payload(payload, response_buffer, response_size);
}

bool handle_tool_get_inventory(context* ctx, const json_value& params, char* response_buffer,
                               size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_inventory");

  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  const json_value      args     = extract_tool_arguments(params);

  std::string payload;
  std::string error;
  if (!build_state_section_payload(snapshot, "inventory", args, &payload, &error)) {
    const std::string resp = build_content_response(error, true);
    return write_json_response(resp, response_buffer, response_size);
  }

  return write_tool_payload(payload, response_buffer, response_size);
}

bool handle_tool_get_state(context* ctx, const json_value& params, char* response_buffer,
                           size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_state");

  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  const json_value      args     = extract_tool_arguments(params);

  json_value section_val = args["section"];
  if (!section_val.is_string()) {
    return write_json_response(
        build_content_response(
            "section is required (player, enemies, entities, map, inventory, game)", true),
        response_buffer, response_size);
  }

  std::string payload;
  std::string error;
  if (!build_state_section_payload(snapshot, section_val.get_string(), args, &payload, &error)) {
    return write_json_response(build_content_response(error, true), response_buffer, response_size);
  }

  return write_tool_payload(payload, response_buffer, response_size);
}

json_builder build_get_player_schema() {
  json_builder schema;
  add_empty_object_schema(&schema);
  return schema;
}

json_builder build_get_map_schema() {
  json_builder schema;
  add_empty_object_schema(&schema);
  return schema;
}

json_builder build_get_game_info_schema() {
  json_builder schema;
  add_empty_object_schema(&schema);
  return schema;
}

json_builder build_get_enemies_schema() {
  return build_pagination_schema("Maximum number of enemies to return");
}

json_builder build_get_entities_schema() {
  return build_pagination_schema("Maximum number of entities to return");
}

json_builder build_get_inventory_schema() {
  return build_pagination_schema("Maximum number of inventory entries to return");
}

json_builder build_get_state_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();

  json_builder section_prop;
  section_prop.start_object();
  section_prop.add("type", "string");
  section_prop.add("description", "State section: player, enemies, entities, map, inventory, game");
  json_builder section_enum;
  section_enum.start_array();
  section_enum.push("player");
  section_enum.push("enemies");
  section_enum.push("entities");
  section_enum.push("map");
  section_enum.push("inventory");
  section_enum.push("game");
  section_prop.add("enum", std::move(section_enum));
  props.add("section", std::move(section_prop));

  json_builder offset_prop;
  offset_prop.start_object();
  offset_prop.add("type", "integer");
  offset_prop.add("description", "Pagination offset (for enemies/entities/inventory)");
  props.add("offset", std::move(offset_prop));

  json_builder limit_prop;
  limit_prop.start_object();
  limit_prop.add("type", "integer");
  limit_prop.add("description", "Pagination limit (for enemies/entities/inventory)");
  props.add("limit", std::move(limit_prop));

  schema.add("properties", std::move(props));

  json_builder required;
  required.start_array();
  required.push("section");
  schema.add("required", std::move(required));

  return schema;
}

}  // namespace dmcp
