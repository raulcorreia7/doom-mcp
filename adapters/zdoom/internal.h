#pragma once

#include <cstdarg>
#include <cstddef>

#include "adapter.h"

namespace dmcp::zdoom {

struct AdapterContext {
  dmcp_context_t*     dmcp_ctx;
  dmcp_zdoom_config_t user_cfg;
  dmcp_config_t       dmcp_cfg;
  bool                log_not_running_emitted;
};

constexpr size_t kLogBufferSize = 256;

void Log(AdapterContext* ctx, int level, const char* fmt, ...);

}  // namespace dmcp::zdoom
