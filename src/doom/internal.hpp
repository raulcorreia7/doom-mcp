#pragma once

#include "dmcp/doom/api.h"
#include "dmcp/doom/commands.h"
#include "mcp/generic/server.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <vector>

// ============================================================================
// Internal Doom MCP Types - Standard Library Style
// Types: snake_case | Functions: snake_case | Members: trailing_underscore_
// ============================================================================

namespace dmcp {

// Snapshot pool entry
struct pool_entry {
  dmcp_snapshot_t data{};
  bool in_use = false;
};

// Command queue - thread-safe
class command_queue {
public:
  bool push(const dmcp_command_t& cmd);
  std::optional<dmcp_command_t> pop();
  bool empty() const { return count_.load(std::memory_order_acquire) == 0; }
  std::uint32_t size() const { return count_.load(std::memory_order_acquire); }
  void clear();
  
  std::uint64_t next_sequence() { return next_sequence_.fetch_add(1); }

private:
  std::queue<dmcp_command_t> queue_;
  mutable std::mutex mutex_;
  std::atomic<std::uint64_t> next_sequence_{1};
  std::atomic<std::uint32_t> count_{0};
};

// Screenshot state
struct screenshot_state {
  std::atomic<bool> enabled{false};
  std::atomic<std::uint32_t> pending_requests{0};
  std::vector<std::uint8_t> latest_png;
  std::mutex png_mutex;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
};

// Context state
struct context {
  dmcp_config_t config{};
  mcp_server_t* server = nullptr;
  std::unique_ptr<command_queue> cmd_queue;
  
  // Snapshot pool
  std::vector<pool_entry> pool;
  std::mutex pool_mutex;
  
  // Rate limiting
  std::chrono::nanoseconds min_interval{};
  std::chrono::steady_clock::time_point last_snapshot_time;
  
  // Screenshot
  screenshot_state screenshot;
  
  // Statistics
  std::atomic<std::uint64_t> dropped_snapshots{0};
  std::atomic<std::uint64_t> dropped_screenshots{0};
  std::atomic<bool> drop_warning_emitted{false};
};

// ============================================================================
// RAII Helpers
// ============================================================================

class snapshot_guard {
public:
  snapshot_guard(context* ctx, dmcp_snapshot_t* snapshot);
  ~snapshot_guard();
  
  // Non-copyable, movable
  snapshot_guard(const snapshot_guard&) = delete;
  snapshot_guard& operator=(const snapshot_guard&) = delete;
  snapshot_guard(snapshot_guard&&) noexcept;
  snapshot_guard& operator=(snapshot_guard&&) noexcept;

private:
  context* ctx_ = nullptr;
  dmcp_snapshot_t* snapshot_ = nullptr;
};

// ============================================================================
// JSON Serialization (internal)
// ============================================================================

std::string snapshot_to_json(const dmcp_snapshot_t& snapshot);

}  // namespace dmcp
