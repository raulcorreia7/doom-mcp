#include "internal/pool.hpp"

#include <cassert>

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

bool release_snapshot(std::vector<pool_entry>& pool, pool_entry* entry) {
  if (!entry) return false;

  for (auto& e : pool) {
    if (&e == entry) {
      assert(e.in_use && "releasing a pool entry that is not in use");
      e.in_use = false;
      dmcp_snapshot_clear(&e.data);
      return true;
    }
  }

  assert(false && "releasing a pool entry that does not belong to this pool");
  return false;
}

}  // namespace dmcp
