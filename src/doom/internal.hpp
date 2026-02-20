#pragma once
#include "internal/context.hpp"

namespace dmcp {

// Shared logging function (implemented in handlers/common.cpp)
void dmcp_log(const context* ctx, int level, const char* fmt, ...);

}  // namespace dmcp
