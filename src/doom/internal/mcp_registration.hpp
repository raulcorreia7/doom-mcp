#pragma once

#include <string>

#include "doom/internal/context.hpp"

namespace dmcp {

bool register_mcp_surface(context* ctx);
void broadcast_state_event(context* ctx, const std::string& json);

}  // namespace dmcp
