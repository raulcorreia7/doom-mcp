#pragma once

#include <string>
#include <string_view>

#include "dmcp/doom/commands.h"
#include "doom/internal/context.hpp"

namespace dmcp {

bool tool_requires_console_permission(std::string_view tool_name);
bool tool_requires_cheat_permission(std::string_view tool_name);

mcp_status_t validate_command_permissions(const context* ctx, const dmcp_command_t& cmd);

}  // namespace dmcp
