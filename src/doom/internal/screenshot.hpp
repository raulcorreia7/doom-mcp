#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace dmcp {

struct screenshot_state {
  std::atomic<bool>     enabled{false};
  std::atomic<uint32_t> pending_requests{0};
  std::vector<uint8_t>  latest_png;
  std::mutex            png_mutex;
  uint32_t              width  = 0;
  uint32_t              height = 0;
};

}  // namespace dmcp
