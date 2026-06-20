#include "tools.hpp"

#include "dmcp/doom/protocol.h"

#include <initializer_list>
#include <utility>

namespace dmcp {

namespace {

bool write_tool_payload(std::string_view payload, char* response_buffer, size_t response_size) {
  const std::string resp = build_content_response(payload, false);
  return write_json_response(resp, response_buffer, response_size);
}

void add_string_enum_property(json_builder* props, const char* name, const char* description,
                              std::initializer_list<const char*> values) {
  if (!props || !name || !description) {
    return;
  }

  json_builder prop;
  prop.start_object();
  prop.add("type", "string");
  prop.add("description", description);

  json_builder enum_values;
  enum_values.start_array();
  for (const char* value : values) {
    enum_values.push(value);
  }
  prop.add("enum", std::move(enum_values));

  props->add(name, std::move(prop));
}

void add_pagination_properties(json_builder* props, const char* limit_description) {
  if (!props || !limit_description) {
    return;
  }

  json_builder offset_prop;
  offset_prop.start_object();
  offset_prop.add("type", "integer");
  offset_prop.add("description", "Zero-based pagination offset");
  props->add("offset", std::move(offset_prop));

  json_builder limit_prop;
  limit_prop.start_object();
  limit_prop.add("type", "integer");
  limit_prop.add("description", limit_description);
  props->add("limit", std::move(limit_prop));
}

json_builder build_object_schema_with_properties(json_builder props) {
  json_builder schema;
  schema.start_object();
  schema.add("type", "object");
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

bool handle_tool_get_items(context* ctx, const json_value& params, char* response_buffer,
                           size_t response_size) {
  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call get_items");

  const dmcp_snapshot_t snapshot = copy_latest_snapshot(ctx);
  const json_value      args     = extract_tool_arguments(params);

  std::string payload;
  std::string error;
  if (!build_state_section_payload(snapshot, "items", args, &payload, &error)) {
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
            "section is required (player, enemies, entities, items, map, inventory, game)", true),
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
  json_builder props;
  props.start_object();
  add_pagination_properties(&props, "Maximum number of enemies to return");
  add_string_enum_property(&props, "status", "Enemy status filter: alive (default), dead, or all",
                           {DMCP_STATUS_ALIVE, DMCP_STATUS_DEAD, DMCP_STATUS_ALL});
  return build_object_schema_with_properties(std::move(props));
}

json_builder build_get_entities_schema() {
  json_builder props;
  props.start_object();
  add_pagination_properties(&props, "Maximum number of entities to return");
  add_string_enum_property(
      &props, "kind",
      "Entity kind filter: all, enemy, item, weapon, ammo, key, health, armor, powerup, world",
      {DMCP_KIND_ALL, DMCP_KIND_ENEMY, DMCP_KIND_ITEM, DMCP_KIND_WEAPON, DMCP_KIND_AMMO,
       DMCP_KIND_KEY, DMCP_KIND_HEALTH, DMCP_KIND_ARMOR, DMCP_KIND_POWERUP, DMCP_KIND_WORLD});
  add_string_enum_property(&props, "status",
                           "Enemy status filter when kind includes enemies: alive, dead, all",
                           {DMCP_STATUS_ALIVE, DMCP_STATUS_DEAD, DMCP_STATUS_ALL});
  return build_object_schema_with_properties(std::move(props));
}

json_builder build_get_items_schema() {
  json_builder props;
  props.start_object();
  add_pagination_properties(&props, "Maximum number of items to return");
  add_string_enum_property(&props, "kind",
                           "Item kind filter: item, weapon, ammo, key, health, armor, powerup",
                           {DMCP_KIND_ITEM, DMCP_KIND_WEAPON, DMCP_KIND_AMMO, DMCP_KIND_KEY,
                            DMCP_KIND_HEALTH, DMCP_KIND_ARMOR, DMCP_KIND_POWERUP});
  return build_object_schema_with_properties(std::move(props));
}

json_builder build_get_inventory_schema() {
  json_builder props;
  props.start_object();
  add_pagination_properties(&props, "Maximum number of inventory entries to return");
  return build_object_schema_with_properties(std::move(props));
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
  section_prop.add("description",
                   "State section: player, enemies, entities, items, map, inventory, game");
  json_builder section_enum;
  section_enum.start_array();
  section_enum.push("player");
  section_enum.push("enemies");
  section_enum.push("entities");
  section_enum.push("items");
  section_enum.push("map");
  section_enum.push("inventory");
  section_enum.push("game");
  section_prop.add("enum", std::move(section_enum));
  props.add("section", std::move(section_prop));

  add_pagination_properties(&props, "Pagination limit (for enemies/entities/items/inventory)");
  add_string_enum_property(
      &props, "kind",
      "Entity or item kind filter for section=entities/items: all, enemy, item, weapon, ammo, key, "
      "health, armor, powerup, world",
      {DMCP_KIND_ALL, DMCP_KIND_ENEMY, DMCP_KIND_ITEM, DMCP_KIND_WEAPON, DMCP_KIND_AMMO,
       DMCP_KIND_KEY, DMCP_KIND_HEALTH, DMCP_KIND_ARMOR, DMCP_KIND_POWERUP, DMCP_KIND_WORLD});
  add_string_enum_property(&props, "status",
                           "Enemy status filter for section=enemies/entities: alive, dead, all",
                           {DMCP_STATUS_ALIVE, DMCP_STATUS_DEAD, DMCP_STATUS_ALL});

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
  section_prop.add("description",
                   "State section: player, enemies, entities, items, map, inventory, game");
  json_builder section_enum;
  section_enum.start_array();
  section_enum.push("player");
  section_enum.push("enemies");
  section_enum.push("entities");
  section_enum.push("items");
  section_enum.push("map");
  section_enum.push("inventory");
  section_enum.push("game");
  section_prop.add("enum", std::move(section_enum));
  item_props.add("section", std::move(section_prop));

  add_pagination_properties(&item_props, "Pagination limit (enemies/entities/items/inventory)");
  add_string_enum_property(
      &item_props, "kind",
      "Entity or item kind filter for section=entities/items: all, enemy, item, weapon, ammo, key, "
      "health, armor, powerup, world",
      {DMCP_KIND_ALL, DMCP_KIND_ENEMY, DMCP_KIND_ITEM, DMCP_KIND_WEAPON, DMCP_KIND_AMMO,
       DMCP_KIND_KEY, DMCP_KIND_HEALTH, DMCP_KIND_ARMOR, DMCP_KIND_POWERUP, DMCP_KIND_WORLD});
  add_string_enum_property(&item_props, "status",
                           "Enemy status filter for section=enemies/entities: alive, dead, all",
                           {DMCP_STATUS_ALIVE, DMCP_STATUS_DEAD, DMCP_STATUS_ALL});

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
