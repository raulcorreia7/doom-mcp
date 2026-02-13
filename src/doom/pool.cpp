#include "internal/pool.hpp"

#include "dmcp/doom/api.h"

namespace dmcp {

pool_entry* acquire_snapshot(std::vector<pool_entry>& pool) {
  for (auto& entry : pool) {
    if (!entry.in_use) {
      entry.in_use = true;
      dmcp_snapshot_clear(&entry.data);
      return &entry;
    }
  }
  return nullptr;
}

void release_snapshot(std::vector<pool_entry>& pool, pool_entry* entry) {
  if (!entry) return;

  for (auto& e : pool) {
    if (&e == entry) {
      e.in_use = false;
      dmcp_snapshot_clear(&e.data);
      return;
    }
  }
}

}  // namespace dmcp
