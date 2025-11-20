#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

#include "dmcp/schema.hpp"

namespace dmcp {

class SnapshotPool {
 public:
  SnapshotPool(size_t initial_count, size_t enemy_capacity,
               size_t inventory_capacity);

  Snapshot* Acquire();
  void      Release(Snapshot* snapshot);

 private:
  std::vector<Snapshot>  storage_;
  std::vector<Snapshot*> freelist_;
  std::mutex             mutex_;
  size_t                 enemy_capacity_;
  size_t                 inventory_capacity_;
};

}  // namespace dmcp
