#include "dmcp/pool.hpp"

namespace dmcp {

SnapshotPool::SnapshotPool(size_t initial_count, size_t enemy_capacity,
                           size_t inventory_capacity)
    : enemy_capacity_(enemy_capacity),
      inventory_capacity_(inventory_capacity) {
  storage_.reserve(initial_count);
  freelist_.reserve(initial_count);
  for (size_t i = 0; i < initial_count; ++i) {
    auto* snapshot = createSnapshot();
    freelist_.push_back(snapshot);
  }
}

Snapshot* SnapshotPool::createSnapshot() {
  auto snap = std::make_unique<Snapshot>();
  snap->enemies.reserve(enemy_capacity_);
  snap->player.inventory.reserve(inventory_capacity_);
  auto* ptr = snap.get();
  storage_.push_back(std::move(snap));
  return ptr;
}

Snapshot* SnapshotPool::Acquire() {
  std::lock_guard<std::mutex> lock(mutex_);
  Snapshot* snapshot = nullptr;
  if (freelist_.empty()) {
    snapshot = createSnapshot();
  } else {
    snapshot = freelist_.back();
    freelist_.pop_back();
  }
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
