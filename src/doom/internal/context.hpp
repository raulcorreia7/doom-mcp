#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "command_queue.hpp"
#include "dmcp/doom/commands.h"
#include "dmcp/doom/config.h"
#include "dmcp/doom/types.h"
#include "mcp/generic/server.h"
#include "screenshot.hpp"

namespace dmcp {

constexpr size_t   k_snapshot_buffer_slots  = 3;
constexpr uint64_t k_snapshot_token_invalid = ~uint64_t{0};
constexpr uint64_t k_snapshot_slot_mask     = 0xFFu;

inline uint64_t make_snapshot_token(size_t slot, uint64_t generation) {
  return (generation << 8u) | static_cast<uint64_t>(slot);
}

inline int snapshot_slot_from_token(uint64_t token) {
  if (token == k_snapshot_token_invalid) {
    return -1;
  }

  const uint64_t slot = token & k_snapshot_slot_mask;
  return slot < k_snapshot_buffer_slots ? static_cast<int>(slot) : -1;
}

struct context {
  dmcp_config_t                  config{};
  mcp_server_t*                  server = nullptr;
  std::unique_ptr<command_queue> cmd_queue;
  std::unique_ptr<command_queue> input_queue;

  // Writer-priority snapshot handoff:
  // - the game thread writes only to an inactive slot with no readers;
  // - publishing is a single atomic index store;
  // - readers copy from the published slot and retry if a newer slot appears.
  std::array<dmcp_snapshot_t, k_snapshot_buffer_slots>       snapshot_buffers{};
  std::array<std::atomic<uint32_t>, k_snapshot_buffer_slots> snapshot_readers{};
  std::atomic<uint64_t>                 published_snapshot_token{k_snapshot_token_invalid};
  uint64_t                              next_snapshot_generation = 1;
  size_t                                next_snapshot_slot       = 0;
  std::chrono::nanoseconds              snapshot_min_interval{};
  std::chrono::steady_clock::time_point last_snapshot_time;

  screenshot_state screenshot;

  std::atomic<uint64_t> dropped_screenshots{0};

  // Results are bounded so long-running agent sessions cannot grow memory
  // without limit while polling historical command sequences.
  std::unordered_map<uint64_t, dmcp_command_result_t> command_results;
  std::deque<uint64_t>                                command_result_order;
  mutable std::mutex                                  command_results_mutex;
  size_t                                              max_command_results = 256;
};

}  // namespace dmcp
