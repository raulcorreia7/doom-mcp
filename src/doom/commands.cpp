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
#include "doom/commands/json_parsers.hpp"
#include "doom/commands/types/command_parsers.hpp"
#include "internal.hpp"
#include "mcp/generic/protocol.h"

namespace dmcp {

namespace {

mcp_result_generic_t invalid_command(const char* message) {
  return MCP_RESULT_ERROR(MCP_RESULT_CODE_INVALID_ARGS, message);
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

  if (type_str == "spawn_entity") {
    if (!dmcp::parse_spawn_command(params, out_cmd)) {
      return dmcp::invalid_command("spawn_entity requires non-empty string entity_class");
    }
  } else if (type_str == "change_level") {
    if (!dmcp::parse_change_level_command(params, out_cmd)) {
      return dmcp::invalid_command("change_level requires map_name like E1M1 or MAP01");
    }
  } else if (type_str == "give_item") {
    if (!dmcp::parse_give_item_command(params, out_cmd)) {
      return dmcp::invalid_command("give_item requires non-empty string item_class");
    }
  } else if (type_str == "set_player_health") {
    if (!dmcp::parse_set_health_command(params, out_cmd)) {
      return dmcp::invalid_command("set_player_health health must be numeric in (0, 200]");
    }
  } else if (type_str == "set_player_position" || type_str == "teleport_player") {
    if (!dmcp::parse_set_position_command(params, out_cmd)) {
      return dmcp::invalid_command("set_player_position requires numeric x and y");
    }
  } else if (type_str == "execute_console") {
    if (!dmcp::parse_console_command(params, out_cmd)) {
      return dmcp::invalid_command("execute_console requires non-empty string command");
    }
  } else if (type_str == "pause_game") {
    if (!dmcp::parse_pause_command(params, out_cmd)) {
      return dmcp::invalid_command("pause_game requires boolean paused");
    }
  } else if (type_str == "set_timescale") {
    if (!dmcp::parse_timescale_command(params, out_cmd)) {
      return dmcp::invalid_command("set_timescale scale must be numeric in (0, 10]");
    }
  } else if (type_str == "damage_entity") {
    if (!dmcp::parse_damage_command(params, out_cmd)) {
      return dmcp::invalid_command("damage_entity target_tid must be an integer >= 0");
    }
  } else if (type_str == "kill_entity") {
    if (!dmcp::parse_kill_command(params, out_cmd)) {
      return dmcp::invalid_command("kill_entity target_tid must be an integer >= 0");
    }
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
