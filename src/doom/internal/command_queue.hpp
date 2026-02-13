#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>

#include "dmcp/doom/commands.h"

namespace dmcp {

constexpr uint32_t COMMAND_QUEUE_MAX_SIZE = 64;
constexpr float    DEFAULT_PLAYER_HEALTH  = 100.0f;
constexpr int32_t  DEFAULT_SKILL_LEVEL    = 3;
constexpr float    DEFAULT_TIMESCALE      = 1.0f;

class command_queue {
 public:
  bool                          push(const dmcp_command_t& cmd);
  std::optional<dmcp_command_t> pop();
  bool                          empty() const;
  uint32_t                      size() const;
  void                          clear();
  uint64_t                      next_sequence();

  uint32_t max_size = COMMAND_QUEUE_MAX_SIZE;

 private:
  std::queue<dmcp_command_t> queue_;
  mutable std::mutex         mutex_;
  std::atomic<uint64_t>      next_sequence_{1};
  std::atomic<uint32_t>      count_{0};
};

}  // namespace dmcp
