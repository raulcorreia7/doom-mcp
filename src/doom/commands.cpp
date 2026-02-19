#include "dmcp/doom/commands.h"

#include <atomic>
#include <cmath>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>

#include "dmcp/doom/api.h"
#include "internal.hpp"
#include "mcp/generic/protocol.h"
#include "mcp/json/json.hpp"

namespace dmcp {

using json_builder  = ::mcp::json::Builder;
using json_document = ::mcp::json::Document;
using json_value    = ::mcp::json::Value;

namespace {

mcp_result_generic_t invalid_command(const char* message) {
  return MCP_RESULT_ERROR(MCP_RESULT_CODE_INVALID_ARGS, message);
}

bool parse_json_number(const json_value& val, double* out) {
  if (!out || !val || !val.is_number()) {
    return false;
  }

  const std::string text = val.dump();
  if (text.empty()) {
    return false;
  }

  char* end_ptr       = nullptr;
  errno               = 0;
  const double parsed = std::strtod(text.c_str(), &end_ptr);
  if (!end_ptr || end_ptr == text.c_str() || *end_ptr != '\0' || errno == ERANGE ||
      !std::isfinite(parsed)) {
    return false;
  }

  *out = parsed;
  return true;
}

bool parse_json_integer(const json_value& val, std::int64_t* out) {
  if (!out) {
    return false;
  }

  double parsed = 0.0;
  if (!parse_json_number(val, &parsed)) {
    return false;
  }
  if (std::floor(parsed) != parsed) {
    return false;
  }
  if (parsed < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      parsed > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }

  *out = static_cast<std::int64_t>(parsed);
  return true;
}

bool parse_json_bool(const json_value& val, bool* out) {
  if (!out || !val) {
    return false;
  }

  if (val.is_bool()) {
    *out = val.get_bool();
    return true;
  }

  if (!val.is_string()) {
    return false;
  }

  const std::string_view text = val.get_string();
  if (text == "true" || text == "True" || text == "TRUE" || text == "1" || text == "yes" ||
      text == "Yes" || text == "on") {
    *out = true;
    return true;
  }
  if (text == "false" || text == "False" || text == "FALSE" || text == "0" || text == "no" ||
      text == "No" || text == "off") {
    *out = false;
    return true;
  }

  return false;
}

json_value first_present_field(const json_value& obj, std::initializer_list<const char*> keys) {
  for (const char* key : keys) {
    if (obj.has_member(key)) {
      return obj[key];
    }
  }
  return {};
}

bool is_valid_map_name(std::string_view map_name) {
  if (map_name.size() == 4 && map_name[0] == 'E' && map_name[2] == 'M' && map_name[1] >= '1' &&
      map_name[1] <= '9' && map_name[3] >= '1' && map_name[3] <= '9') {
    return true;
  }

  if (map_name.size() == 5 && map_name[0] == 'M' && map_name[1] == 'A' && map_name[2] == 'P' &&
      map_name[3] >= '0' && map_name[3] <= '9' && map_name[4] >= '0' && map_name[4] <= '9') {
    return true;
  }

  return false;
}

}  // namespace

// ============================================================================
// CommandQueue Implementation
// ============================================================================

bool command_queue::push(const dmcp_command_t& cmd, uint64_t* assigned_sequence) {
  std::lock_guard<std::mutex> lock{mutex_};

  if (count_.load() >= max_size) {
    return false;
  }

  auto mutable_cmd     = cmd;
  mutable_cmd.sequence = next_sequence_++;
  if (assigned_sequence) {
    *assigned_sequence = mutable_cmd.sequence;
  }
  queue_.push(mutable_cmd);
  count_.fetch_add(1, std::memory_order_release);

  return true;
}

std::optional<dmcp_command_t> command_queue::pop() {
  std::lock_guard<std::mutex> lock{mutex_};

  if (queue_.empty()) {
    return std::nullopt;
  }

  auto cmd = queue_.front();
  queue_.pop();
  count_.fetch_sub(1, std::memory_order_release);

  return cmd;
}

void command_queue::clear() {
  std::lock_guard<std::mutex> lock{mutex_};

  while (!queue_.empty()) {
    queue_.pop();
  }
  count_.store(0, std::memory_order_release);
}

bool command_queue::empty() const { return count_.load(std::memory_order_acquire) == 0; }

uint32_t command_queue::size() const { return count_.load(std::memory_order_acquire); }

uint64_t command_queue::next_sequence() { return next_sequence_.fetch_add(1); }

}  // namespace dmcp

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

mcp_result_generic_t dmcp_push_command(dmcp_context_t* ctx_handle, dmcp_command_t* cmd) {
  if (!ctx_handle || !cmd) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);
  if (!ctx->cmd_queue) {
    return MCP_ERROR_DISABLED;
  }

  uint64_t sequence = 0;
  if (!ctx->cmd_queue->push(*cmd, &sequence)) {
    return MCP_RESULT_ERROR(MCP_RESULT_CODE_QUEUE_FULL, "Command queue is full");
  }
  cmd->sequence = sequence;
  return MCP_OK;
}

mcp_result_generic_t dmcp_command_result_mark_queued(dmcp_context_t*       ctx_handle,
                                                     const dmcp_command_t* cmd) {
  if (!ctx_handle || !cmd || cmd->sequence == 0) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  dmcp_command_result_t entry = {};
  entry.sequence              = cmd->sequence;
  entry.command_type          = cmd->type;
  entry.completed             = false;
  entry.success               = false;
  dmcp_strcpy(entry.message, "Command queued (execution pending)", sizeof(entry.message));

  std::lock_guard<std::mutex> lock(ctx->command_results_mutex);
  auto                        it = ctx->command_results.find(entry.sequence);
  if (it == ctx->command_results.end()) {
    if (ctx->command_result_order.size() >= ctx->max_command_results) {
      const uint64_t evicted_sequence = ctx->command_result_order.front();
      ctx->command_result_order.pop_front();
      ctx->command_results.erase(evicted_sequence);
    }
    ctx->command_result_order.push_back(entry.sequence);
    ctx->command_results.emplace(entry.sequence, entry);
  } else {
    it->second = entry;
  }

  return MCP_OK;
}

mcp_result_generic_t dmcp_command_result_complete(dmcp_context_t*       ctx_handle,
                                                  const dmcp_command_t* cmd, bool success,
                                                  const char* message) {
  if (!ctx_handle || !cmd || cmd->sequence == 0) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  dmcp_command_result_t entry = {};
  entry.sequence              = cmd->sequence;
  entry.command_type          = cmd->type;
  entry.completed             = true;
  entry.success               = success;

  if (message && message[0] != '\0') {
    dmcp_strcpy(entry.message, message, sizeof(entry.message));
  } else {
    dmcp_strcpy(entry.message,
                success ? "Command executed successfully" : "Command execution failed",
                sizeof(entry.message));
  }

  std::lock_guard<std::mutex> lock(ctx->command_results_mutex);
  auto                        it = ctx->command_results.find(entry.sequence);
  if (it == ctx->command_results.end()) {
    if (ctx->command_result_order.size() >= ctx->max_command_results) {
      const uint64_t evicted_sequence = ctx->command_result_order.front();
      ctx->command_result_order.pop_front();
      ctx->command_results.erase(evicted_sequence);
    }
    ctx->command_result_order.push_back(entry.sequence);
    ctx->command_results.emplace(entry.sequence, entry);
  } else {
    it->second = entry;
  }

  return MCP_OK;
}

mcp_result_generic_t dmcp_command_result_get(const dmcp_context_t* ctx_handle, uint64_t sequence,
                                             dmcp_command_result_t* out_result) {
  if (!ctx_handle || !out_result || sequence == 0) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);

  std::lock_guard<std::mutex> lock(ctx->command_results_mutex);
  auto                        it = ctx->command_results.find(sequence);
  if (it == ctx->command_results.end()) {
    return MCP_ERROR_NOT_FOUND;
  }

  *out_result = it->second;
  return MCP_OK;
}

bool dmcp_pop_command(dmcp_context_t* ctx_handle, dmcp_command_t* out_cmd) {
  if (!ctx_handle || !out_cmd) {
    return false;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);
  if (!ctx->cmd_queue) {
    return false;
  }

  auto cmd = ctx->cmd_queue->pop();
  if (!cmd) {
    return false;
  }

  *out_cmd = *cmd;
  return true;
}

bool dmcp_has_commands(const dmcp_context_t* ctx_handle) {
  if (!ctx_handle) {
    return false;
  }

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);
  if (!ctx->cmd_queue) {
    return false;
  }

  return !ctx->cmd_queue->empty();
}

std::uint32_t dmcp_command_count(const dmcp_context_t* ctx_handle) {
  if (!ctx_handle) {
    return 0;
  }

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);
  if (!ctx->cmd_queue) {
    return 0;
  }

  return ctx->cmd_queue->size();
}

void dmcp_clear_commands(dmcp_context_t* ctx_handle) {
  if (!ctx_handle) {
    return;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);
  if (!ctx->cmd_queue) {
    return;
  }

  ctx->cmd_queue->clear();
}

// ============================================================================
// JSON Parsing
// ============================================================================

mcp_result_generic_t dmcp_parse_command_json(const char* json_str, dmcp_command_t* out_cmd) {
  if (!json_str || !out_cmd) {
    return MCP_ERROR_INVALID_ARGS;
  }

  std::memset(out_cmd, 0, sizeof(*out_cmd));

  dmcp::json_document doc;
  if (!doc.parse(json_str)) {
    return MCP_ERROR_ENCODING_FAILED;
  }

  dmcp::json_value root = doc.root();
  if (!root.is_object()) {
    return dmcp::invalid_command("Command payload must be an object");
  }

  dmcp::json_value type_value = root["type"];
  if (!type_value.is_string()) {
    return dmcp::invalid_command("Command type must be a string");
  }

  const std::string_view type_str = type_value.get_string();
  if (type_str.empty()) {
    return dmcp::invalid_command("Command type cannot be empty");
  }

  const bool       has_params = root.has_member("params");
  dmcp::json_value params     = root["params"];
  if (has_params) {
    if (!params.is_object()) {
      return dmcp::invalid_command("Command params must be an object");
    }
  } else {
    params = root;
  }

  auto read_required_string = [](const dmcp::json_value&            obj,
                                 std::initializer_list<const char*> keys,
                                 std::string_view*                  out) -> bool {
    if (!out) {
      return false;
    }

    const dmcp::json_value candidate = dmcp::first_present_field(obj, keys);
    if (!candidate || !candidate.is_string()) {
      return false;
    }

    const std::string_view value = candidate.get_string();
    if (value.empty()) {
      return false;
    }

    *out = value;
    return true;
  };

  auto read_optional_string = [](const dmcp::json_value&            obj,
                                 std::initializer_list<const char*> keys,
                                 std::string_view default_value, std::string_view* out) -> bool {
    if (!out) {
      return false;
    }

    const dmcp::json_value candidate = dmcp::first_present_field(obj, keys);
    if (!candidate) {
      *out = default_value;
      return true;
    }
    if (!candidate.is_string()) {
      return false;
    }

    const std::string_view value = candidate.get_string();
    if (value.empty()) {
      return false;
    }

    *out = value;
    return true;
  };

  auto read_required_number = [](const dmcp::json_value&            obj,
                                 std::initializer_list<const char*> keys, double* out) -> bool {
    const dmcp::json_value candidate = dmcp::first_present_field(obj, keys);
    return dmcp::parse_json_number(candidate, out);
  };

  auto read_optional_number = [](const dmcp::json_value&            obj,
                                 std::initializer_list<const char*> keys, double default_value,
                                 double* out) -> bool {
    if (!out) {
      return false;
    }

    const dmcp::json_value candidate = dmcp::first_present_field(obj, keys);
    if (!candidate) {
      *out = default_value;
      return true;
    }
    return dmcp::parse_json_number(candidate, out);
  };

  auto read_required_int = [](const dmcp::json_value& obj, std::initializer_list<const char*> keys,
                              std::int64_t* out) -> bool {
    const dmcp::json_value candidate = dmcp::first_present_field(obj, keys);
    return dmcp::parse_json_integer(candidate, out);
  };

  auto read_optional_int = [](const dmcp::json_value& obj, std::initializer_list<const char*> keys,
                              std::int64_t default_value, std::int64_t* out) -> bool {
    if (!out) {
      return false;
    }

    const dmcp::json_value candidate = dmcp::first_present_field(obj, keys);
    if (!candidate) {
      *out = default_value;
      return true;
    }
    return dmcp::parse_json_integer(candidate, out);
  };

  auto read_required_bool = [](const dmcp::json_value& obj, std::initializer_list<const char*> keys,
                               bool* out) -> bool {
    const dmcp::json_value candidate = dmcp::first_present_field(obj, keys);
    return dmcp::parse_json_bool(candidate, out);
  };

  auto read_optional_bool = [](const dmcp::json_value& obj, std::initializer_list<const char*> keys,
                               bool default_value, bool* out) -> bool {
    if (!out) {
      return false;
    }

    const dmcp::json_value candidate = dmcp::first_present_field(obj, keys);
    if (!candidate) {
      *out = default_value;
      return true;
    }
    return dmcp::parse_json_bool(candidate, out);
  };

  auto copy_checked_string = [](char* dst, size_t dst_size, std::string_view value) -> bool {
    if (!dst || dst_size == 0 || value.empty() || value.size() >= dst_size) {
      return false;
    }
    dmcp_strcpy(dst, value.data(), dst_size);
    return true;
  };

  if (type_str == "spawn_entity") {
    out_cmd->type = DMCP_CMD_SPAWN_ENTITY;

    std::string_view entity_class;
    if (!read_required_string(params, {"entity_class", "entity", "class"}, &entity_class) ||
        !copy_checked_string(out_cmd->data.spawn.entity_class,
                             sizeof(out_cmd->data.spawn.entity_class), entity_class)) {
      return dmcp::invalid_command("spawn_entity requires non-empty string entity_class");
    }

    dmcp::json_value position     = params["position"];
    dmcp::json_value coord_source = params;
    if (params.has_member("position")) {
      if (!position.is_object()) {
        return dmcp::invalid_command("spawn_entity position must be an object");
      }
      coord_source = position;
    }

    double x     = 0.0;
    double y     = 0.0;
    double angle = 0.0;
    if (!read_required_number(coord_source, {"x"}, &x) ||
        !read_required_number(coord_source, {"y"}, &y) || !std::isfinite(x) || !std::isfinite(y)) {
      return dmcp::invalid_command("spawn_entity requires numeric x and y");
    }
    if (!read_optional_number(params, {"angle"}, 0.0, &angle) || !std::isfinite(angle)) {
      return dmcp::invalid_command("spawn_entity angle must be numeric");
    }

    std::int64_t tid = 0;
    if (!read_optional_int(params, {"tid"}, 0, &tid) || tid < 0 ||
        tid > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
      return dmcp::invalid_command("spawn_entity tid must be an integer >= 0");
    }

    out_cmd->data.spawn.position.x = static_cast<float>(x);
    out_cmd->data.spawn.position.y = static_cast<float>(y);
    out_cmd->data.spawn.angle      = static_cast<float>(angle);
    out_cmd->data.spawn.tid        = static_cast<std::int32_t>(tid);

  } else if (type_str == "change_level") {
    out_cmd->type = DMCP_CMD_CHANGE_LEVEL;

    std::string_view map_name;
    if (!read_required_string(params, {"map_name", "level"}, &map_name) ||
        !dmcp::is_valid_map_name(map_name) ||
        !copy_checked_string(out_cmd->data.change_level.map_name,
                             sizeof(out_cmd->data.change_level.map_name), map_name)) {
      return dmcp::invalid_command("change_level requires map_name like E1M1 or MAP01");
    }

    std::int64_t skill_level = 3;
    if (!read_optional_int(params, {"skill_level"}, 3, &skill_level) || skill_level < 1 ||
        skill_level > 5) {
      return dmcp::invalid_command("change_level skill_level must be an integer in [1, 5]");
    }

    bool reset_inventory = false;
    if (!read_optional_bool(params, {"reset_inventory"}, false, &reset_inventory)) {
      return dmcp::invalid_command("change_level reset_inventory must be boolean");
    }

    out_cmd->data.change_level.skill_level     = static_cast<std::int32_t>(skill_level);
    out_cmd->data.change_level.reset_inventory = reset_inventory;

  } else if (type_str == "give_item") {
    out_cmd->type = DMCP_CMD_GIVE_ITEM;

    std::string_view item_class;
    if (!read_required_string(params, {"item_class", "item"}, &item_class) ||
        !copy_checked_string(out_cmd->data.give_item.item_class,
                             sizeof(out_cmd->data.give_item.item_class), item_class)) {
      return dmcp::invalid_command("give_item requires non-empty string item_class");
    }

    std::int64_t amount = 1;
    if (!read_optional_int(params, {"amount", "quantity"}, 1, &amount) || amount < 1 ||
        amount > 1000) {
      return dmcp::invalid_command("give_item amount must be an integer in [1, 1000]");
    }
    out_cmd->data.give_item.amount = static_cast<std::int32_t>(amount);

  } else if (type_str == "set_player_health") {
    out_cmd->type = DMCP_CMD_SET_PLAYER_HEALTH;

    double health = 0.0;
    if (!read_required_number(params, {"health", "value"}, &health) || !std::isfinite(health) ||
        health <= 0.0 || health > 200.0) {
      return dmcp::invalid_command("set_player_health health must be numeric in (0, 200]");
    }
    out_cmd->data.set_health.health = static_cast<float>(health);

  } else if (type_str == "set_player_position" || type_str == "teleport_player") {
    out_cmd->type = DMCP_CMD_SET_PLAYER_POSITION;

    dmcp::json_value position     = params["position"];
    dmcp::json_value coord_source = params;
    if (params.has_member("position")) {
      if (!position.is_object()) {
        return dmcp::invalid_command("set_player_position position must be an object");
      }
      coord_source = position;
    }

    double x     = 0.0;
    double y     = 0.0;
    double angle = 0.0;
    if (!read_required_number(coord_source, {"x"}, &x) ||
        !read_required_number(coord_source, {"y"}, &y) || !std::isfinite(x) || !std::isfinite(y)) {
      return dmcp::invalid_command("set_player_position requires numeric x and y");
    }
    if (!read_optional_number(params, {"angle"}, 0.0, &angle) || !std::isfinite(angle)) {
      return dmcp::invalid_command("set_player_position angle must be numeric");
    }

    out_cmd->data.set_position.position.x = static_cast<float>(x);
    out_cmd->data.set_position.position.y = static_cast<float>(y);
    out_cmd->data.set_position.angle      = static_cast<float>(angle);

  } else if (type_str == "execute_console") {
    out_cmd->type = DMCP_CMD_EXECUTE_CONSOLE;

    std::string_view command;
    if (!read_required_string(params, {"command"}, &command) ||
        !copy_checked_string(out_cmd->data.console.command, sizeof(out_cmd->data.console.command),
                             command)) {
      return dmcp::invalid_command("execute_console requires non-empty string command");
    }

  } else if (type_str == "pause_game") {
    out_cmd->type = DMCP_CMD_PAUSE_GAME;

    bool paused = false;
    if (!read_required_bool(params, {"paused", "pause"}, &paused)) {
      return dmcp::invalid_command("pause_game requires boolean paused");
    }
    out_cmd->data.pause.paused = paused;

  } else if (type_str == "set_timescale") {
    out_cmd->type = DMCP_CMD_SET_TIMESCALE;

    double scale = 0.0;
    if (!read_required_number(params, {"scale"}, &scale) || !std::isfinite(scale) || scale <= 0.0 ||
        scale > 10.0) {
      return dmcp::invalid_command("set_timescale scale must be numeric in (0, 10]");
    }
    out_cmd->data.timescale.scale = static_cast<float>(scale);

  } else if (type_str == "damage_entity") {
    out_cmd->type = DMCP_CMD_DAMAGE_ENTITY;

    std::int64_t target_tid = 0;
    if (!read_required_int(params, {"target_tid"}, &target_tid) || target_tid < 0 ||
        target_tid > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
      return dmcp::invalid_command("damage_entity target_tid must be an integer >= 0");
    }

    double damage = 0.0;
    if (!read_required_number(params, {"damage"}, &damage) || !std::isfinite(damage) ||
        damage <= 0.0 || damage > 10000.0) {
      return dmcp::invalid_command("damage_entity damage must be numeric in (0, 10000]");
    }

    std::string_view damage_type;
    if (!read_optional_string(params, {"damage_type"}, "Normal", &damage_type) ||
        !copy_checked_string(out_cmd->data.damage.damage_type,
                             sizeof(out_cmd->data.damage.damage_type), damage_type)) {
      return dmcp::invalid_command("damage_entity damage_type must be a non-empty string");
    }

    out_cmd->data.damage.target_tid = static_cast<std::int32_t>(target_tid);
    out_cmd->data.damage.damage     = static_cast<float>(damage);

  } else if (type_str == "kill_entity") {
    out_cmd->type = DMCP_CMD_KILL_ENTITY;

    std::int64_t target_tid = 0;
    if (!read_required_int(params, {"target_tid"}, &target_tid) || target_tid < 0 ||
        target_tid > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
      return dmcp::invalid_command("kill_entity target_tid must be an integer >= 0");
    }
    out_cmd->data.kill.target_tid = static_cast<std::int32_t>(target_tid);

  } else {
    return dmcp::invalid_command("Unsupported command type");
  }

  if (root.has_member("flags")) {
    std::int64_t flags = 0;
    if (!dmcp::parse_json_integer(root["flags"], &flags) || flags < 0 ||
        flags > static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max())) {
      return dmcp::invalid_command("flags must be a non-negative integer");
    }
    out_cmd->flags = static_cast<std::uint32_t>(flags);
  }

  return MCP_OK;
}

mcp_result_generic_t dmcp_parse_command_json_ex(dmcp_context_t* ctx_handle, const char* json_str,
                                                dmcp_command_t* out_cmd) {
  if (!json_str || !out_cmd) {
    return MCP_ERROR_INVALID_ARGS;
  }

  std::memset(out_cmd, 0, sizeof(*out_cmd));

  dmcp::json_document doc;
  if (!doc.parse(json_str)) {
    return MCP_ERROR_ENCODING_FAILED;
  }

  dmcp::json_value root = doc.root();
  if (!root.is_object()) {
    return dmcp::invalid_command("Command payload must be an object");
  }

  dmcp::json_value type_value = root["type"];
  if (!type_value.is_string()) {
    return dmcp::invalid_command("Command type must be a string");
  }

  const std::string_view type_str = type_value.get_string();
  if (type_str.empty()) {
    return dmcp::invalid_command("Command type cannot be empty");
  }

  const bool       has_params = root.has_member("params");
  dmcp::json_value params     = root["params"];
  if (has_params) {
    if (!params.is_object()) {
      return dmcp::invalid_command("Command params must be an object");
    }
  } else {
    params = root;
  }

  auto* ctx = ctx_handle ? reinterpret_cast<dmcp::context*>(ctx_handle) : nullptr;
  if (ctx) {
    std::lock_guard<std::mutex> lock(ctx->custom_parsers_mutex);
    auto                        it = ctx->custom_parsers.find(std::string(type_str));
    if (it != ctx->custom_parsers.end()) {
      std::string params_json = params.is_object() ? params.dump() : "{}";
      return it->second.second(params_json.c_str(), out_cmd);
    }
  }

  return dmcp_parse_command_json(json_str, out_cmd);
}

mcp_result_generic_t dmcp_format_command_result(const dmcp_command_t* cmd, bool success,
                                                const char* message, char* buffer,
                                                size_t buffer_size) {
  if (!cmd || !buffer || buffer_size == 0) {
    return MCP_ERROR_INVALID_ARGS;
  }

  dmcp::json_builder builder;
  builder.start_object();
  builder.add("sequence", static_cast<std::int64_t>(cmd->sequence));
  builder.add("success", success);
  if (message) {
    builder.add("message", message);
  }

  const std::string result = builder.finish();
  if (result.size() >= buffer_size) {
    return MCP_ERROR_ENCODING_FAILED;
  }

  std::strcpy(buffer, result.c_str());
  return MCP_OK;
}

mcp_result_generic_t dmcp_register_command_parser(dmcp_context_t* ctx_handle, const char* type_name,
                                                  dmcp_command_type_t          type,
                                                  dmcp_custom_command_parser_t parser) {
  if (!ctx_handle || !type_name || !parser) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);
  {
    std::lock_guard<std::mutex> lock(ctx->custom_parsers_mutex);
    ctx->custom_parsers[type_name] = {type, parser};
  }

  return MCP_OK;
}

}  // extern "C"
