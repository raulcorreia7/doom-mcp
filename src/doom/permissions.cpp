#include "doom/internal/permissions.hpp"

#include <cctype>
#include <cstring>
#include <string>

#include "dmcp/doom/protocol.h"

namespace dmcp {

namespace {

std::string normalize_console_command(const char* command) {
  std::string normalized;
  bool        previous_space = true;

  if (!command) {
    return normalized;
  }

  for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(command); *cursor;
       ++cursor) {
    if (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r') {
      if (!previous_space) {
        normalized.push_back(' ');
        previous_space = true;
      }
      continue;
    }

    normalized.push_back(static_cast<char>(std::tolower(*cursor)));
    previous_space = false;
  }

  if (!normalized.empty() && normalized.back() == ' ') {
    normalized.pop_back();
  }

  return normalized;
}

bool is_known_console_cheat(std::string_view command) {
  return command == "god" || command == "godmode" || command == "iddqd" ||
         command == "god on" || command == "godmode on" || command == "ungod" ||
         command == "god off" || command == "godmode off" || command == "noclip" ||
         command == "noclip on" || command == "idspispopd" || command == "clip" ||
         command == "noclip off" || command == "give all" || command == "killall";
}

bool command_requires_cheat_permission(const dmcp_command_t& cmd) {
  switch (cmd.type) {
    case DMCP_CMD_SET_PLAYER_HEALTH:
    case DMCP_CMD_SET_PLAYER_POSITION:
    case DMCP_CMD_DAMAGE_ENTITY:
    case DMCP_CMD_KILL_ENTITY:
      return true;
    case DMCP_CMD_EXECUTE_CONSOLE:
      return is_known_console_cheat(normalize_console_command(cmd.data.console.command));
    default:
      return false;
  }
}

}  // namespace

bool tool_requires_console_permission(std::string_view tool_name) {
  return tool_name == DMCP_TOOL_EXECUTE_CONSOLE;
}

bool tool_requires_cheat_permission(std::string_view tool_name) {
  return tool_name == DMCP_TOOL_SET_PLAYER_HEALTH || tool_name == DMCP_TOOL_SET_PLAYER_POSITION ||
         tool_name == DMCP_TOOL_DAMAGE_ENTITY || tool_name == DMCP_TOOL_KILL_ENTITY;
}

mcp_status_t validate_command_permissions(const context* ctx, const dmcp_command_t& cmd) {
  if (!ctx) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  if (cmd.type == DMCP_CMD_EXECUTE_CONSOLE && !ctx->config.permissions.allow_console_commands) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_DISABLED,
                            "execute_console is disabled; enable allow_console_commands");
  }

  if (command_requires_cheat_permission(cmd) && !ctx->config.permissions.allow_cheats) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_DISABLED,
                            "Cheat-style commands are disabled; enable allow_cheats");
  }

  return MCP_STATUS_OK("Success");
}

}  // namespace dmcp
