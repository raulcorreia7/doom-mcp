#include "core/pool.hpp"

namespace dmcp {

SnapshotPool::SnapshotPool(size_t initial_count, size_t enemy_capacity,
                           size_t inventory_capacity)
    : enemy_capacity_(enemy_capacity), inventory_capacity_(inventory_capacity) {
  storage_.resize(initial_count);
  freelist_.reserve(initial_count);
  for (auto& snapshot : storage_) {
    snapshot.enemies.reserve(enemy_capacity_);
    snapshot.player.inventory.reserve(inventory_capacity_);
    freelist_.push_back(&snapshot);
  }
}

Snapshot* SnapshotPool::Acquire() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (freelist_.empty()) {
    return nullptr;
  }
  Snapshot* snapshot = freelist_.back();
  freelist_.pop_back();
  snapshot->Clear();
  return snapshot;
}

void SnapshotPool::Release(Snapshot* snapshot) {
  if (!snapshot) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  snapshot->Clear();
  freelist_.push_back(snapshot);
}

}  // namespace dmcp
