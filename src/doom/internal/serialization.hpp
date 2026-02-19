#pragma once

#include <string>

#include "dmcp/doom/types.h"

namespace dmcp {

std::string snapshot_to_json(const dmcp_snapshot_t& snapshot);

}  // namespace dmcp
