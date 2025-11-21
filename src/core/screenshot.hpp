#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace dmcp {

struct ScreenshotFrame {
  const uint8_t* pixels = nullptr;
  uint32_t       width  = 0;
  uint32_t       height = 0;
  uint32_t       stride = 0;
};

struct ScreenshotState {
  std::mutex                            data_mutex;
  std::vector<uint8_t>                  latest_png;
  uint32_t                              width  = 0;
  uint32_t                              height = 0;
  std::chrono::system_clock::time_point captured_at =
      std::chrono::system_clock::from_time_t(0);
  std::atomic<uint64_t> version{0};
  std::atomic<uint32_t> pending_requests{0};
};

}  // namespace dmcp
