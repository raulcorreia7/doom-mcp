#include "dmcp/doom/commands.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <queue>
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

// ============================================================================
// CommandQueue Implementation
// ============================================================================

bool command_queue::push(const dmcp_command_t& cmd) {
  std::lock_guard<std::mutex> lock{mutex_};

  if (count_.load() >= max_size) {
    return false;
  }

  auto mutable_cmd     = cmd;
  mutable_cmd.sequence = next_sequence_++;
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

bool command_queue::empty() const {
  return count_.load(std::memory_order_acquire) == 0;
}

uint32_t command_queue::size() const {
  return count_.load(std::memory_order_acquire);
}

uint64_t command_queue::next_sequence() { return next_sequence_.fetch_add(1); }

}  // namespace dmcp

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

mcp_result_generic_t dmcp_push_command(dmcp_context_t*       ctx_handle,
                                       const dmcp_command_t* cmd) {
  if (!ctx_handle || !cmd) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);
  if (!ctx->cmd_queue) {
    return MCP_ERROR_DISABLED;
  }

  if (!ctx->cmd_queue->push(*cmd)) {
    return MCP_RESULT_ERROR(MCP_RESULT_CODE_QUEUE_FULL,
                            "Command queue is full");
  }
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

mcp_result_generic_t dmcp_parse_command_json(const char*     json_str,
                                             dmcp_command_t* out_cmd) {
  if (!json_str || !out_cmd) {
    return MCP_ERROR_INVALID_ARGS;
  }

  std::memset(out_cmd, 0, sizeof(*out_cmd));

  dmcp::json_document doc;
  if (!doc.parse(json_str)) {
    return MCP_ERROR_ENCODING_FAILED;
  }

  dmcp::json_value       root     = doc.root();
  const std::string_view type_str = root["type"].get_string();
  dmcp::json_value       params   = root["params"];

  auto parse_string = [](dmcp::json_value val,
                         std::string_view default_val = {}) {
    return val ? val.get_string(default_val) : default_val;
  };

  auto parse_double = [](dmcp::json_value val, double default_val = 0.0) {
    return val ? val.get_double(default_val) : default_val;
  };

  auto parse_int = [](dmcp::json_value val, std::int64_t default_val = 0) {
    return val ? val.get_int(default_val) : default_val;
  };

  auto parse_bool = [](dmcp::json_value val, bool default_val = false) {
    return val ? val.get_bool(default_val) : default_val;
  };

  if (type_str == "spawn_entity") {
    out_cmd->type = DMCP_CMD_SPAWN_ENTITY;

    const std::string_view entity_class = parse_string(params["entity_class"]);
    dmcp_strcpy(out_cmd->data.spawn.entity_class, entity_class.data(),
                sizeof(out_cmd->data.spawn.entity_class));

    out_cmd->data.spawn.position.x = parse_double(params["position"]["x"]);
    out_cmd->data.spawn.position.y = parse_double(params["position"]["y"]);
    out_cmd->data.spawn.angle      = parse_double(params["angle"]);
    out_cmd->data.spawn.tid =
	static_cast<std::int32_t>(parse_int(params["tid"]));

  } else if (type_str == "change_level") {
    out_cmd->type = DMCP_CMD_CHANGE_LEVEL;

    const std::string_view map_name = parse_string(params["map_name"]);
    dmcp_strcpy(out_cmd->data.change_level.map_name, map_name.data(),
                sizeof(out_cmd->data.change_level.map_name));

    out_cmd->data.change_level.skill_level =
	static_cast<std::int32_t>(parse_int(params["skill_level"], 3));
    out_cmd->data.change_level.reset_inventory =
	parse_bool(params["reset_inventory"]);

  } else if (type_str == "give_item") {
    out_cmd->type = DMCP_CMD_GIVE_ITEM;

    const std::string_view item_class = parse_string(params["item_class"]);
    dmcp_strcpy(out_cmd->data.give_item.item_class, item_class.data(),
                sizeof(out_cmd->data.give_item.item_class));

    out_cmd->data.give_item.amount =
	static_cast<std::int32_t>(parse_int(params["amount"], 1));

  } else if (type_str == "set_player_health") {
    out_cmd->type                   = DMCP_CMD_SET_PLAYER_HEALTH;
    out_cmd->data.set_health.health = parse_double(params["health"], 100.0);

  } else if (type_str == "set_player_position") {
    out_cmd->type                         = DMCP_CMD_SET_PLAYER_POSITION;
    out_cmd->data.set_position.position.x = parse_double(params["x"]);
    out_cmd->data.set_position.position.y = parse_double(params["y"]);
    out_cmd->data.set_position.angle      = parse_double(params["angle"]);

  } else if (type_str == "execute_console") {
    out_cmd->type = DMCP_CMD_EXECUTE_CONSOLE;

    const std::string_view command = parse_string(params["command"]);
    dmcp_strcpy(out_cmd->data.console.command, command.data(),
                sizeof(out_cmd->data.console.command));

  } else if (type_str == "pause_game") {
    out_cmd->type              = DMCP_CMD_PAUSE_GAME;
    out_cmd->data.pause.paused = parse_bool(params["paused"], true);

  } else if (type_str == "set_timescale") {
    out_cmd->type                 = DMCP_CMD_SET_TIMESCALE;
    out_cmd->data.timescale.scale = parse_double(params["scale"], 1.0);

  } else if (type_str == "damage_entity") {
    out_cmd->type = DMCP_CMD_DAMAGE_ENTITY;
    out_cmd->data.damage.target_tid =
	static_cast<std::int32_t>(parse_int(params["target_tid"]));
    out_cmd->data.damage.damage = parse_double(params["damage"]);

    const std::string_view damage_type =
	parse_string(params["damage_type"], "Normal");
    dmcp_strcpy(out_cmd->data.damage.damage_type, damage_type.data(),
                sizeof(out_cmd->data.damage.damage_type));

  } else if (type_str == "kill_entity") {
    out_cmd->type = DMCP_CMD_KILL_ENTITY;
    out_cmd->data.kill.target_tid =
	static_cast<std::int32_t>(parse_int(params["target_tid"]));

  } else {
    out_cmd->type = DMCP_CMD_UNKNOWN;
    return MCP_ERROR_INVALID_ARGS;
  }

  out_cmd->flags = static_cast<std::uint32_t>(parse_int(root["flags"]));

  return MCP_OK;
}

mcp_result_generic_t dmcp_parse_command_json_ex(dmcp_context_t* ctx_handle,
                                                const char*     json_str,
                                                dmcp_command_t* out_cmd) {
  if (!json_str || !out_cmd) {
    return MCP_ERROR_INVALID_ARGS;
  }

  std::memset(out_cmd, 0, sizeof(*out_cmd));

  dmcp::json_document doc;
  if (!doc.parse(json_str)) {
    return MCP_ERROR_ENCODING_FAILED;
  }

  dmcp::json_value       root     = doc.root();
  const std::string_view type_str = root["type"].get_string();
  dmcp::json_value       params   = root["params"];

  auto* ctx =
      ctx_handle ? reinterpret_cast<dmcp::context*>(ctx_handle) : nullptr;
  if (ctx) {
    std::lock_guard<std::mutex> lock(ctx->custom_parsers_mutex);
    auto it = ctx->custom_parsers.find(std::string(type_str));
    if (it != ctx->custom_parsers.end()) {
      std::string params_json = params ? params.dump() : "{}";
      return it->second.second(params_json.c_str(), out_cmd);
    }
  }

  return dmcp_parse_command_json(json_str, out_cmd);
}

mcp_result_generic_t dmcp_format_command_result(const dmcp_command_t* cmd,
                                                bool                  success,
                                                const char*           message,
                                                char*                 buffer,
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

mcp_result_generic_t dmcp_register_command_parser(
    dmcp_context_t* ctx_handle, const char* type_name, dmcp_command_type_t type,
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
