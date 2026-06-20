#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace dmcp {

std::string convert_pixels_to_ascii(const uint8_t* pixels, size_t pixels_size, uint32_t width,
                                    uint32_t height, uint32_t target_width);

struct screenshot_state {
  std::atomic<bool>     enabled{false};
  std::atomic<uint32_t> pending_requests{0};
  std::vector<uint8_t>  latest_pixels;
  std::string           latest_ascii;
  std::mutex            mutex;
  uint32_t              width  = 0;
  uint32_t              height = 0;

  bool build_ascii(uint32_t target_width, std::string* out_ascii, uint32_t* out_width = nullptr,
                   uint32_t* out_height = nullptr) {
    std::lock_guard<std::mutex> lock(mutex);

    if (latest_pixels.empty() || width == 0 || height == 0) {
      latest_ascii.clear();
      if (out_ascii) {
        out_ascii->clear();
      }
      if (out_width) {
        *out_width = 0;
      }
      if (out_height) {
        *out_height = 0;
      }
      return false;
    }

    latest_ascii = convert_pixels_to_ascii(latest_pixels.data(), latest_pixels.size(), width,
                                           height, target_width);

    if (out_ascii) {
      *out_ascii = latest_ascii;
    }
    if (out_width) {
      *out_width = width;
    }
    if (out_height) {
      *out_height = height;
    }

    return !latest_ascii.empty();
  }
};

}  // namespace dmcp
