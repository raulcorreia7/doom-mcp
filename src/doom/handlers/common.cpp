#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include "dmcp/adapter/content.h"
#include "dmcp/doom/api.h"
#include "doom/handlers/tools/tools.hpp"
#include "doom/internal/context.hpp"
#include "doom/internal/json_types.hpp"
#include "mcp/generic/constants.h"

namespace dmcp {

static const command_tool_definition k_command_tools[] = {
    {"spawn_entity", "spawn_entity", "Spawn an entity in the current level"},
    {"change_level", "change_level", "Change to another map/level"},
    {"give_item", "give_item", "Give an item to the player"},
    {"set_player_health", "set_player_health", "Set player health value"},
    {"teleport_player", "teleport_player", "Teleport player to coordinates"},
    {"set_player_position", "set_player_position", "Set player position and facing"},
    {"execute_console", "execute_console", "Execute an engine console command"},
    {"pause_game", "pause_game", "Pause or unpause game simulation"},
    {"damage_entity", "damage_entity", "Apply damage to a target entity"},
    {"kill_entity", "kill_entity", "Kill a target entity"},
};

namespace {

dmcp_gamemode_t parse_game_mode_name(std::string_view mode_name) {
  if (mode_name == "shareware") return DMCP_GAMEMODE_SHAREWARE;
  if (mode_name == "registered") return DMCP_GAMEMODE_REGISTERED;
  if (mode_name == "commercial" || mode_name == "doom2") return DMCP_GAMEMODE_COMMERCIAL;
  if (mode_name == "retail" || mode_name == "ultimate") return DMCP_GAMEMODE_RETAIL;
  return DMCP_GAMEMODE_UNKNOWN;
}

dmcp_gamemode_t get_snapshot_game_mode(context* ctx) {
  if (!ctx) {
    return DMCP_GAMEMODE_UNKNOWN;
  }

  std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
  if (ctx->last_snapshot.game.version[0] == '\0') {
    return DMCP_GAMEMODE_UNKNOWN;
  }

  return parse_game_mode_name(ctx->last_snapshot.game.version);
}

bool validate_command_for_mode(context* ctx, const dmcp_command_t& cmd, std::string* out_error) {
  if (!ctx || !out_error) {
    return false;
  }

  const dmcp_gamemode_t mode = get_snapshot_game_mode(ctx);
  if (mode == DMCP_GAMEMODE_UNKNOWN) {
    // No snapshot yet: do not over-restrict. Adapter will validate on execution.
    return true;
  }

  switch (cmd.type) {
    case DMCP_CMD_SPAWN_ENTITY:
      if (!dmcp_is_enemy_spawnable(cmd.data.spawn.entity_class, mode) &&
          !dmcp_is_item_available(cmd.data.spawn.entity_class, mode)) {
        *out_error =
            dmcp_content_unavailable_message("Entity/Item", cmd.data.spawn.entity_class, mode);
        return false;
      }
      break;
    case DMCP_CMD_GIVE_ITEM:
      if (!dmcp_is_item_available(cmd.data.give_item.item_class, mode)) {
        *out_error = dmcp_content_unavailable_message("Item", cmd.data.give_item.item_class, mode);
        return false;
      }
      break;
    case DMCP_CMD_CHANGE_LEVEL:
      if (!dmcp_is_map_available(cmd.data.change_level.map_name, mode)) {
        *out_error = dmcp_content_unavailable_message("Map", cmd.data.change_level.map_name, mode);
        return false;
      }
      break;
    default:
      break;
  }

  return true;
}

}  // namespace

const command_tool_definition* find_command_tool(std::string_view tool_name) {
  for (const auto& tool : k_command_tools) {
    if (tool_name == tool.tool_name) {
      return &tool;
    }
  }
  return nullptr;
}

const char* resolve_command_type_for_method(std::string_view method_name) {
  if (method_name == "execute_command") {
    return "execute_command";
  }

  const command_tool_definition* tool = find_command_tool(method_name);
  return tool ? tool->command_type : nullptr;
}

void dmcp_log(const context* ctx, int level, const char* fmt, ...) {
  if (!ctx || !ctx->config.on_log || !fmt) {
    return;
  }

  // Note: messages longer than 511 chars are silently truncated
  char    buffer[512];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  ctx->config.on_log(ctx->config.user_data, level, buffer);
}

std::string build_content_response(std::string_view text, bool is_error) {
  json_builder result;
  result.start_object();
  if (is_error) {
    result.add("isError", true);
  }
  json_builder content;
  content.start_array();
  json_builder item;
  item.start_object();
  item.add("type", "text");
  item.add("text", text);
  content.push(std::move(item));
  result.add("content", std::move(content));
  return result.finish();
}

bool write_json_response(std::string_view payload, char* response_buffer, size_t response_size) {
  if (!response_buffer || response_size == 0) {
    return false;
  }
  if (payload.size() >= response_size) {
    return false;
  }

  std::memcpy(response_buffer, payload.data(), payload.size());
  response_buffer[payload.size()] = '\0';
  return true;
}

dmcp_snapshot_t copy_latest_snapshot(context* ctx) {
  dmcp_snapshot_t snapshot_copy{};
  if (!ctx) {
    return snapshot_copy;
  }

  std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
  snapshot_copy = ctx->last_snapshot;
  return snapshot_copy;
}

namespace {

json_builder build_vec3_json(const dmcp_vec3_t& position) {
  json_builder obj;
  obj.start_object();
  obj.add("x", static_cast<double>(position.x));
  obj.add("y", static_cast<double>(position.y));
  obj.add("z", static_cast<double>(position.z));
  return obj;
}

json_builder build_player_json(const dmcp_player_t& player) {
  json_builder obj;
  obj.start_object();
  obj.add("hp", static_cast<int64_t>(player.hp));
  obj.add("armor", static_cast<int64_t>(player.armor));
  obj.add("armortype", player.armortype);
  obj.add("position", build_vec3_json(player.position));
  obj.add("angle", static_cast<double>(player.angle));
  obj.add("readyweapon", player.readyweapon);
  obj.add("pendingweapon", player.pendingweapon);

  json_builder weapon_owned;
  weapon_owned.start_array();
  for (size_t i = 0; i < DMCP_MAX_WEAPONS; ++i) {
    weapon_owned.push(static_cast<int64_t>(player.weaponowned[i]));
  }
  obj.add("weaponowned", std::move(weapon_owned));

  json_builder ammo;
  ammo.start_array();
  for (size_t i = 0; i < DMCP_MAX_AMMO_TYPES; ++i) {
    ammo.push(static_cast<int64_t>(player.ammo[i]));
  }
  obj.add("ammo", std::move(ammo));

  json_builder maxammo;
  maxammo.start_array();
  for (size_t i = 0; i < DMCP_MAX_AMMO_TYPES; ++i) {
    maxammo.push(static_cast<int64_t>(player.maxammo[i]));
  }
  obj.add("maxammo", std::move(maxammo));

  obj.add("backpack", player.backpack != 0);

  json_builder powers;
  powers.start_array();
  for (size_t i = 0; i < DMCP_MAX_POWERUPS; ++i) {
    powers.push(static_cast<int64_t>(player.powers[i]));
  }
  obj.add("powers", std::move(powers));

  json_builder cards;
  cards.start_array();
  for (size_t i = 0; i < DMCP_MAX_KEYS; ++i) {
    cards.push(static_cast<int64_t>(player.cards[i]));
  }
  obj.add("cards", std::move(cards));

  obj.add("playerstate", player.playerstate);
  obj.add("cheats", static_cast<int64_t>(player.cheats));
  obj.add("damagecount", static_cast<int64_t>(player.damagecount));
  return obj;
}

json_builder build_level_json(const dmcp_level_t& level) {
  json_builder obj;
  obj.start_object();
  obj.add("tic", static_cast<int64_t>(level.tic));
  obj.add("leveltime", static_cast<int64_t>(level.leveltime));
  obj.add("level_id", level.level_id);
  obj.add("level_name", level.level_name);
  obj.add("kill_count", static_cast<int64_t>(level.kill_count));
  obj.add("item_count", static_cast<int64_t>(level.item_count));
  obj.add("secret_count", static_cast<int64_t>(level.secret_count));
  obj.add("totalkills", static_cast<int64_t>(level.totalkills));
  obj.add("totalitems", static_cast<int64_t>(level.totalitems));
  obj.add("totalsecrets", static_cast<int64_t>(level.totalsecrets));
  obj.add("skill", level.skill);
  obj.add("gamestate", level.gamestate);
  obj.add("paused", level.paused != 0);
  return obj;
}

json_builder build_game_json(const dmcp_game_t& game) {
  json_builder obj;
  obj.start_object();
  obj.add("mode", game.mode);
  obj.add("version", game.version);
  obj.add("respawnmonsters", game.respawnmonsters != 0);
  obj.add("consoleplayer", static_cast<int64_t>(game.consoleplayer));
  return obj;
}

json_builder build_enemy_json(const dmcp_enemy_t& enemy) {
  json_builder obj;
  obj.start_object();
  obj.add("id", static_cast<int64_t>(enemy.id));
  obj.add("hp", static_cast<int64_t>(enemy.hp));
  obj.add("max_hp", static_cast<int64_t>(enemy.max_hp));
  obj.add("position", build_vec3_json(enemy.position));
  obj.add("angle", static_cast<double>(enemy.angle));
  obj.add("target_id", static_cast<int64_t>(enemy.target_id));
  obj.add("type", enemy.type);
  return obj;
}

json_builder build_item_json(const dmcp_item_t& item) {
  json_builder obj;
  obj.start_object();
  obj.add("name", item.name);
  obj.add("amount", static_cast<int64_t>(item.amount));
  return obj;
}

json_builder build_entity_json(const dmcp_entity_t& entity) {
  json_builder obj;
  obj.start_object();
  obj.add("id", static_cast<int64_t>(entity.id));
  obj.add("hp", static_cast<int64_t>(entity.hp));
  obj.add("max_hp", static_cast<int64_t>(entity.max_hp));
  obj.add("position", build_vec3_json(entity.position));
  obj.add("angle", static_cast<double>(entity.angle));
  obj.add("type", entity.type);
  return obj;
}

bool parse_size_from_number(const json_value& value, size_t* out) {
  if (!out || !value || !value.is_number()) {
    return false;
  }

  const std::string number_text = value.dump();
  if (number_text.empty()) {
    return false;
  }

  char* end_ptr                   = nullptr;
  errno                           = 0;
  const unsigned long long parsed = std::strtoull(number_text.c_str(), &end_ptr, 10);
  if (!end_ptr || end_ptr == number_text.c_str() || *end_ptr != '\0' || errno == ERANGE) {
    return false;
  }

  *out = static_cast<size_t>(parsed);
  return true;
}

}  // namespace

std::string build_player_state_json(const dmcp_snapshot_t& snapshot) {
  json_builder payload;
  payload.start_object();
  payload.add("player", build_player_json(snapshot.player));
  return payload.finish();
}

std::string build_map_state_json(const dmcp_snapshot_t& snapshot) {
  json_builder payload;
  payload.start_object();
  payload.add("map", build_level_json(snapshot.level));
  return payload.finish();
}

std::string build_game_info_json(const dmcp_snapshot_t& snapshot) {
  json_builder payload;
  payload.start_object();
  payload.add("game", build_game_json(snapshot.game));
  return payload.finish();
}

std::string build_enemies_state_json(const dmcp_snapshot_t& snapshot, size_t offset, size_t limit) {
  const size_t total = snapshot.enemy_count;
  const size_t start = std::min(offset, total);
  const size_t end   = std::min(start + limit, total);

  json_builder payload;
  payload.start_object();
  payload.add("enemy_count", static_cast<int64_t>(total));
  payload.add("offset", static_cast<int64_t>(start));
  payload.add("limit", static_cast<int64_t>(limit));
  payload.add("returned", static_cast<int64_t>(end - start));

  json_builder enemies;
  enemies.start_array();
  for (size_t i = start; i < end; ++i) {
    enemies.push(build_enemy_json(snapshot.enemies[i]));
  }
  payload.add("enemies", std::move(enemies));
  return payload.finish();
}

std::string build_inventory_state_json(const dmcp_snapshot_t& snapshot, size_t offset,
                                       size_t limit) {
  const size_t total = snapshot.inventory_count;
  const size_t start = std::min(offset, total);
  const size_t end   = std::min(start + limit, total);

  json_builder payload;
  payload.start_object();
  payload.add("inventory_count", static_cast<int64_t>(total));
  payload.add("offset", static_cast<int64_t>(start));
  payload.add("limit", static_cast<int64_t>(limit));
  payload.add("returned", static_cast<int64_t>(end - start));

  json_builder items;
  items.start_array();
  for (size_t i = start; i < end; ++i) {
    items.push(build_item_json(snapshot.inventory[i]));
  }
  payload.add("inventory", std::move(items));
  return payload.finish();
}

std::string build_entities_state_json(const dmcp_snapshot_t& snapshot, size_t offset,
                                      size_t limit) {
  const size_t total = snapshot.entity_count;
  const size_t start = std::min(offset, total);
  const size_t end   = std::min(start + limit, total);

  json_builder payload;
  payload.start_object();
  payload.add("entity_count", static_cast<int64_t>(total));
  payload.add("offset", static_cast<int64_t>(start));
  payload.add("limit", static_cast<int64_t>(limit));
  payload.add("returned", static_cast<int64_t>(end - start));

  json_builder entities;
  entities.start_array();
  for (size_t i = start; i < end; ++i) {
    entities.push(build_entity_json(snapshot.entities[i]));
  }
  payload.add("entities", std::move(entities));
  return payload.finish();
}

bool parse_offset_limit_from_json(const json_value& args, size_t total_count, size_t default_limit,
                                  size_t* out_offset, size_t* out_limit, std::string* out_error) {
  if (!out_offset || !out_limit || !out_error) {
    return false;
  }

  *out_offset = 0;
  *out_limit  = default_limit;
  *out_error  = "Invalid pagination params";

  if (!args.is_object()) {
    return true;
  }

  json_value offset_val = args["offset"];
  if (offset_val) {
    if (!parse_size_from_number(offset_val, out_offset)) {
      *out_error = "offset must be a non-negative integer";
      return false;
    }
  }

  json_value limit_val = args["limit"];
  if (limit_val) {
    if (!parse_size_from_number(limit_val, out_limit) || *out_limit == 0) {
      *out_error = "limit must be a positive integer";
      return false;
    }
  }

  if (*out_offset > total_count) {
    *out_offset = total_count;
  }

  if (*out_limit > total_count && total_count > 0) {
    *out_limit = total_count;
  }

  if (total_count == 0) {
    *out_offset = 0;
  }

  return true;
}

bool build_state_section_payload(const dmcp_snapshot_t& snapshot, std::string_view section,
                                 const json_value& args, std::string* out_payload,
                                 std::string* out_error) {
  if (!out_payload || !out_error) {
    return false;
  }

  constexpr size_t k_default_limit = 64;

  if (section.empty()) {
    *out_error = "section is required (player, enemies, entities, map, inventory, game)";
    return false;
  }

  if (section == "player") {
    *out_payload = build_player_state_json(snapshot);
    return true;
  }

  if (section == "map" || section == "level") {
    *out_payload = build_map_state_json(snapshot);
    return true;
  }

  if (section == "game" || section == "game_info") {
    *out_payload = build_game_info_json(snapshot);
    return true;
  }

  if (section == "enemies") {
    size_t offset = 0;
    size_t limit  = k_default_limit;
    if (!parse_offset_limit_from_json(args, snapshot.enemy_count, k_default_limit, &offset, &limit,
                                      out_error)) {
      return false;
    }

    *out_payload = build_enemies_state_json(snapshot, offset, limit);
    return true;
  }

  if (section == "entities") {
    size_t offset = 0;
    size_t limit  = k_default_limit;
    if (!parse_offset_limit_from_json(args, snapshot.entity_count, k_default_limit, &offset, &limit,
                                      out_error)) {
      return false;
    }

    *out_payload = build_entities_state_json(snapshot, offset, limit);
    return true;
  }

  if (section == "inventory") {
    size_t offset = 0;
    size_t limit  = k_default_limit;
    if (!parse_offset_limit_from_json(args, snapshot.inventory_count, k_default_limit, &offset,
                                      &limit, out_error)) {
      return false;
    }

    *out_payload = build_inventory_state_json(snapshot, offset, limit);
    return true;
  }

  *out_error = "Unknown section. Use one of: player, enemies, entities, map, inventory, game";
  return false;
}

bool parse_sequence_field(const json_value& value, uint64_t* out_sequence) {
  if (!out_sequence || !value || !value.is_number()) {
    return false;
  }

  const std::string number_text = value.dump();
  if (number_text.empty()) {
    return false;
  }

  char* end_ptr                   = nullptr;
  errno                           = 0;
  const unsigned long long parsed = std::strtoull(number_text.c_str(), &end_ptr, 10);
  if (!end_ptr || end_ptr == number_text.c_str() || *end_ptr != '\0' || errno == ERANGE ||
      parsed == 0) {
    return false;
  }

  *out_sequence = static_cast<uint64_t>(parsed);
  return true;
}

bool parse_sequence_from_params(const char* request_json, uint64_t* out_sequence,
                                std::string* out_error) {
  if (!out_sequence || !out_error) {
    return false;
  }

  *out_sequence = 0;
  *out_error    = "Invalid command sequence";

  json_document doc;
  if (!doc.parse(request_json ? request_json : "{}")) {
    *out_error = "Invalid request params";
    return false;
  }

  json_value root = doc.root();
  if (!root.is_object()) {
    *out_error = "Request params must be an object";
    return false;
  }

  if (!parse_sequence_field(root["sequence"], out_sequence)) {
    *out_error = "sequence must be a positive integer";
    return false;
  }

  return true;
}

std::string build_command_result_json(const dmcp_command_result_t& result) {
  json_builder payload;
  payload.start_object();
  payload.add("sequence", static_cast<int64_t>(result.sequence));
  payload.add("command_type", static_cast<int64_t>(result.command_type));

  if (!result.completed) {
    payload.add("status", "pending");
  } else if (result.success) {
    payload.add("status", "success");
  } else {
    payload.add("status", "failed");
  }

  payload.add("completed", result.completed);
  payload.add("success", result.success);
  if (result.entity_id >= 0) {
    payload.add("entity_id", static_cast<int64_t>(result.entity_id));
  }
  if (result.message[0] != '\0') {
    payload.add("message", result.message);
  }

  return payload.finish();
}

bool queue_command_from_json(context* ctx, std::string_view command_json, dmcp_command_t* out_cmd,
                             std::string* error_message) {
  if (!ctx || !out_cmd || !error_message) {
    return false;
  }

  *error_message = "Invalid command";
  if (command_json.empty()) {
    return false;
  }

  const std::string    command_payload(command_json);
  dmcp_command_t       cmd{};
  mcp_result_generic_t parse_result = dmcp_parse_command_json_ex(
      reinterpret_cast<dmcp_context_t*>(ctx), command_payload.c_str(), &cmd);

  if (parse_result.code != MCP_RESULT_CODE_OK) {
    if (parse_result.message && parse_result.message[0] != '\0') {
      *error_message = parse_result.message;
    }
    return false;
  }

  if (!validate_command_for_mode(ctx, cmd, error_message)) {
    return false;
  }

  mcp_result_generic_t push_result =
      dmcp_push_command(reinterpret_cast<dmcp_context_t*>(ctx), &cmd);
  if (push_result.code != MCP_RESULT_CODE_OK) {
    if (push_result.message && push_result.message[0] != '\0') {
      *error_message = push_result.message;
    } else {
      *error_message = "Failed to queue command";
    }
    return false;
  }

  dmcp_command_result_mark_queued(reinterpret_cast<dmcp_context_t*>(ctx), &cmd);

  *out_cmd = cmd;
  return true;
}

json_value extract_tool_arguments(const json_value& params) {
  json_value args = params["arguments"];
  if (!args.is_object()) {
    args = params["params"];
  }
  if (!args.is_object()) {
    args = params;
  }
  return args;
}

std::string extract_command_json(const json_value& params) {
  json_value command_args = extract_tool_arguments(params);
  if (!command_args.is_object() && params["type"].is_string()) {
    command_args = params;
  }
  if (!command_args.is_object()) {
    return {};
  }
  return command_args.dump();
}

bool queue_command_and_respond(context* ctx, std::string_view command_json,
                               std::string_view command_name, char* response_buffer,
                               size_t response_size) {
  dmcp_command_t cmd{};
  std::string    error_message = "Invalid command";
  if (queue_command_from_json(ctx, command_json, &cmd, &error_message)) {
    if (!command_name.empty()) {
      dmcp_log(ctx, MCP_LOG_INFO, "Command queued via %.*s (type=%d)",
               static_cast<int>(command_name.size()), command_name.data(), cmd.type);
    }

    json_builder payload;
    payload.start_object();
    payload.add("status", "queued");
    payload.add("sequence", static_cast<int64_t>(cmd.sequence));
    payload.add("command_type", static_cast<int64_t>(cmd.type));
    if (!command_name.empty()) {
      payload.add("command_name", std::string(command_name));
    }

    const std::string resp = build_content_response(payload.finish(), false);
    return write_json_response(resp, response_buffer, response_size);
  }

  if (!command_name.empty()) {
    dmcp_log(ctx, MCP_LOG_WARN, "%.*s failed: %s", static_cast<int>(command_name.size()),
             command_name.data(), error_message.c_str());
  }

  json_builder error_payload;
  error_payload.start_object();
  error_payload.add("status", "error");
  error_payload.add("message", error_message);
  const std::string resp = build_content_response(error_payload.finish(), true);
  return write_json_response(resp, response_buffer, response_size);
}

bool write_route_response(std::string_view json, int status, char* response_buffer,
                          size_t response_size, int* http_status) {
  if (!response_buffer || response_size == 0 || !http_status) {
    return false;
  }
  if (json.size() >= response_size) {
    return false;
  }

  std::memcpy(response_buffer, json.data(), json.size());
  response_buffer[json.size()] = '\0';
  *http_status                 = status;
  return true;
}

std::string build_route_error(std::string_view code, std::string_view message) {
  json_builder error;
  error.start_object();
  error.add("code", code);
  error.add("message", message);

  json_builder root;
  root.start_object();
  root.add("error", std::move(error));
  return root.finish();
}

const command_tool_definition* get_command_tools_array() { return k_command_tools; }

size_t get_command_tools_count() { return sizeof(k_command_tools) / sizeof(k_command_tools[0]); }

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

  if (std::strcmp(tool_name, "spawn_entity") == 0) {
    add_property(&props, "entity_class", "string",
                 "Entity class (enemy or pickup), eg DoomImp, Zombieman, Medikit");
    add_property(&props, "x", "number", "Spawn X coordinate");
    add_property(&props, "y", "number", "Spawn Y coordinate");
    add_property(&props, "angle", "number", "Facing angle in degrees");
    required.push("entity_class");
    has_required = true;
  } else if (std::strcmp(tool_name, "change_level") == 0) {
    add_property(&props, "map_name", "string", "Map name, eg E1M2 or MAP01");
    add_property(&props, "skill_level", "integer", "Difficulty level 1-5");
    add_property(&props, "reset_inventory", "boolean", "Reset inventory on map load");
    required.push("map_name");
    has_required = true;
  } else if (std::strcmp(tool_name, "give_item") == 0) {
    add_property(&props, "item_class", "string", "Item class, eg Shotgun or Medikit");
    add_property(&props, "amount", "integer", "Quantity to give");
    required.push("item_class");
    has_required = true;
  } else if (std::strcmp(tool_name, "set_player_health") == 0) {
    add_property(&props, "health", "number", "Health value (alias: value)");
    required.push("health");
    has_required = true;
  } else if (std::strcmp(tool_name, "teleport_player") == 0 ||
             std::strcmp(tool_name, "set_player_position") == 0) {
    add_property(&props, "x", "number", "Destination X coordinate");
    add_property(&props, "y", "number", "Destination Y coordinate");
    add_property(&props, "angle", "number", "Facing angle in degrees");
    required.push("x");
    required.push("y");
    has_required = true;
  } else if (std::strcmp(tool_name, "execute_console") == 0) {
    add_property(&props, "command", "string", "Raw console command to execute");
    required.push("command");
    has_required = true;
  } else if (std::strcmp(tool_name, "pause_game") == 0) {
    add_property(&props, "paused", "boolean", "Pause state (alias: pause)");
    required.push("paused");
    has_required = true;
  } else if (std::strcmp(tool_name, "damage_entity") == 0) {
    add_property(&props, "target_tid", "integer", "Target enemy id from game state");
    add_property(&props, "damage", "number", "Damage amount");
    add_property(&props, "damage_type", "string", "Damage type label");
    required.push("target_tid");
    required.push("damage");
    has_required = true;
  } else if (std::strcmp(tool_name, "kill_entity") == 0) {
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

bool handle_tool_command_alias(context* ctx, const command_tool_definition* command_tool,
                               const json_value& params, char* response_buffer,
                               size_t response_size) {
  if (!command_tool) {
    return false;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "tools/call %s", command_tool->tool_name);

  json_value        command_args = extract_tool_arguments(params);
  const std::string params_json  = command_args.is_object() ? command_args.dump() : "{}";
  const std::string command_json = std::string("{\"type\":\"") + command_tool->command_type +
                                   "\",\"params\":" + params_json + "}";

  return queue_command_and_respond(ctx, command_json, command_tool->tool_name, response_buffer,
                                   response_size);
}

}  // namespace dmcp
