#pragma once

#include <cstdarg>
#include <cstddef>

#include "dmcp_zdoom.h"

namespace dmcp::zdoom {

struct AdapterContext {
  dmcp_context_t*     dmcp_ctx;
  dmcp_zdoom_config_t user_cfg;
  dmcp_config_t       dmcp_cfg;
  bool                log_not_running_emitted;
};

constexpr size_t kLogBufferSize = 256;

void adapter_log(AdapterContext* ctx, int level, const char* fmt, ...);

}  // namespace dmcp::zdoom
