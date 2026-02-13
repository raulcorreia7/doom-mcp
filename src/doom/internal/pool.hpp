#pragma once
#include <vector>

#include "dmcp/doom/types.h"

namespace dmcp {

struct pool_entry {
  dmcp_snapshot_t data{};
  bool            in_use = false;
};

pool_entry* acquire_snapshot(std::vector<pool_entry>& pool);
void        release_snapshot(std::vector<pool_entry>& pool, pool_entry* entry);

}  // namespace dmcp
