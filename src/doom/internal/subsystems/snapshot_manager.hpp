#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

#include "../pool.hpp"
#include "dmcp/doom/types.h"

namespace dmcp {

// Forward declaration
struct context;

class snapshot_manager {
 public:
  explicit snapshot_manager(context* ctx);
  ~snapshot_manager();

  bool initialize(size_t pool_size);
  void shutdown();

  pool_entry* acquire();
  void        release(pool_entry* entry);

  void update(const dmcp_snapshot_t& snapshot);
  bool get_latest(dmcp_snapshot_t* out_snapshot) const;

  bool should_update(std::chrono::steady_clock::time_point now) const;
  void set_target_interval(std::chrono::nanoseconds interval);

  uint64_t get_dropped_count() const { return dropped_count_.load(); }
  bool     was_warning_emitted() const { return warning_emitted_.load(); }

 private:
  context* ctx_;

  std::vector<pool_entry> pool_;
  std::mutex              pool_mutex_;

  dmcp_snapshot_t    latest_snapshot_;
  mutable std::mutex latest_snapshot_mutex_;

  std::chrono::nanoseconds              target_interval_;
  std::chrono::steady_clock::time_point last_update_time_;

  std::atomic<uint64_t> dropped_count_{0};
  std::atomic<bool>     warning_emitted_{false};
};

}  // namespace dmcp
