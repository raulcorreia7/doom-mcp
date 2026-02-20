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

bool handle_tool_get_state_batch(context* ctx, const json_value& params, char* response_buffer,
                                 size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_state_batch");

  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  const json_value      args     = extract_tool_arguments(params);

  json_value requests = args["requests"];
  if (!requests.is_array() || requests.size() == 0) {
    return write_json_response(
        build_content_response("requests is required and must be a non-empty array", true),
        response_buffer, response_size);
  }

  json_builder results;
  results.start_array();

  json_builder errors;
  errors.start_array();

  int64_t success_count = 0;

  for (size_t i = 0; i < requests.size(); ++i) {
    const json_value request = requests[i];

    if (!request.is_object()) {
      json_builder error;
      error.start_object();
      error.add("index", static_cast<int64_t>(i));
      error.add("error", "request entry must be an object");
      errors.push(std::move(error));
      continue;
    }

    json_value section_val = request["section"];
    if (!section_val.is_string()) {
      json_builder error;
      error.start_object();
      error.add("index", static_cast<int64_t>(i));
      error.add("error", "section is required for each request");
      errors.push(std::move(error));
      continue;
    }

    std::string            payload;
    std::string            error_message;
    const std::string_view section_name = section_val.get_string();
    if (!build_state_section_payload(snapshot, section_name, request, &payload, &error_message)) {
      json_builder error;
      error.start_object();
      error.add("index", static_cast<int64_t>(i));
      error.add("section", section_name);
      error.add("error", error_message);
      errors.push(std::move(error));
      continue;
    }

    json_builder result_item;
    result_item.start_object();
    result_item.add("index", static_cast<int64_t>(i));
    result_item.add("section", section_name);
    result_item.add("result", payload);
    results.push(std::move(result_item));
    ++success_count;
  }

  json_builder payload;
  payload.start_object();
  payload.add("requested", static_cast<int64_t>(requests.size()));
  payload.add("returned", success_count);
  payload.add("failed", static_cast<int64_t>(requests.size()) - success_count);
  payload.add("results", std::move(results));
  payload.add("errors", std::move(errors));
  payload.add("status", success_count > 0 ? "ok" : "error");

  return write_tool_payload(payload.finish(), response_buffer, response_size);
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
  limit_prop.add("description", "Maximum number of enemies to return");
  props.add("limit", std::move(limit_prop));

  json_builder status_prop;
  status_prop.start_object();
  status_prop.add("type", "string");
  status_prop.add("description", "Enemy status filter: alive (default), dead, or all");
  json_builder status_enum;
  status_enum.start_array();
  status_enum.push("alive");
  status_enum.push("dead");
  status_enum.push("all");
  status_prop.add("enum", std::move(status_enum));
  props.add("status", std::move(status_prop));

  schema.add("properties", std::move(props));
  return schema;
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

  json_builder status_prop;
  status_prop.start_object();
  status_prop.add("type", "string");
  status_prop.add("description", "Enemy status filter for section=enemies: alive, dead, all");
  json_builder status_enum;
  status_enum.start_array();
  status_enum.push("alive");
  status_enum.push("dead");
  status_enum.push("all");
  status_prop.add("enum", std::move(status_enum));
  props.add("status", std::move(status_prop));

  schema.add("properties", std::move(props));

  json_builder required;
  required.start_array();
  required.push("section");
  schema.add("required", std::move(required));

  return schema;
}

json_builder build_get_state_batch_schema() {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");

  json_builder props;
  props.start_object();

  json_builder requests_prop;
  requests_prop.start_object();
  requests_prop.add("type", "array");
  requests_prop.add("description", "Array of read-only state queries");

  json_builder item_schema;
  item_schema.start_object();
  item_schema.add("type", "object");

  json_builder item_props;
  item_props.start_object();

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
  item_props.add("section", std::move(section_prop));

  json_builder offset_prop;
  offset_prop.start_object();
  offset_prop.add("type", "integer");
  offset_prop.add("description", "Pagination offset (enemies/entities/inventory)");
  item_props.add("offset", std::move(offset_prop));

  json_builder limit_prop;
  limit_prop.start_object();
  limit_prop.add("type", "integer");
  limit_prop.add("description", "Pagination limit (enemies/entities/inventory)");
  item_props.add("limit", std::move(limit_prop));

  json_builder status_prop;
  status_prop.start_object();
  status_prop.add("type", "string");
  status_prop.add("description", "Enemy status filter for section=enemies: alive, dead, all");
  json_builder status_enum;
  status_enum.start_array();
  status_enum.push("alive");
  status_enum.push("dead");
  status_enum.push("all");
  status_prop.add("enum", std::move(status_enum));
  item_props.add("status", std::move(status_prop));

  item_schema.add("properties", std::move(item_props));

  json_builder item_required;
  item_required.start_array();
  item_required.push("section");
  item_schema.add("required", std::move(item_required));

  requests_prop.add("items", std::move(item_schema));
  props.add("requests", std::move(requests_prop));

  schema.add("properties", std::move(props));

  json_builder required;
  required.start_array();
  required.push("requests");
  schema.add("required", std::move(required));

  return schema;
}

}  // namespace dmcp
