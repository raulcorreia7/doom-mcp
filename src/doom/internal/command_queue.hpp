#pragma once
#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

#include "dmcp/doom/commands.h"

namespace dmcp {

constexpr float    DEFAULT_PLAYER_HEALTH  = 100.0f;
constexpr int32_t  DEFAULT_SKILL_LEVEL    = 3;

class command_queue {
 public:
  bool                        push(const dmcp_command_t& cmd, uint64_t* assigned_sequence = nullptr,
                                   dmcp_command_t* evicted_cmd = nullptr);
  std::vector<dmcp_command_t> remove_by_type(dmcp_command_type_t type);
  std::optional<dmcp_command_t> pop();
  bool                          empty() const;
  uint32_t                      size() const;
  void                          clear();
  uint64_t                      next_sequence();

  uint32_t max_size = DMCP_DEFAULT_QUEUE_SLOTS;

 private:
  std::deque<dmcp_command_t> queue_;
  mutable std::mutex         mutex_;
  std::atomic<uint64_t>      next_sequence_{1};
  std::atomic<uint32_t>      count_{0};
};

}  // namespace dmcp
