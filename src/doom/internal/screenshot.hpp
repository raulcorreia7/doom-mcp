#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace dmcp {

std::string convert_pixels_to_ascii(const uint8_t* pixels, uint32_t width, uint32_t height,
                                    uint32_t target_width);

struct screenshot_state {
  std::atomic<bool>     enabled{false};
  std::atomic<uint32_t> pending_requests{0};
  std::vector<uint8_t>  latest_pixels;
  std::string           latest_ascii;
  std::mutex            mutex;
  uint32_t              width  = 0;
  uint32_t              height = 0;

  void convert_to_ascii(uint32_t target_width = 160) {
    if (latest_pixels.empty() || width == 0 || height == 0) {
      std::lock_guard<std::mutex> lock(mutex);
      latest_ascii.clear();
      return;
    }
    std::lock_guard<std::mutex> lock(mutex);
    latest_ascii = convert_pixels_to_ascii(latest_pixels.data(), width, height, target_width);
  }

  const std::string& get_ascii() const { return latest_ascii; }
};

}  // namespace dmcp
