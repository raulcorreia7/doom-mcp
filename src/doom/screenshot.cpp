#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "internal/screenshot.hpp"

namespace dmcp {

static const char* ASCII_CHARS =
    "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
static const int ASCII_CHARS_COUNT = 69;

static float calculate_brightness(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<float>(r) * 0.299f + static_cast<float>(g) * 0.587f +
         static_cast<float>(b) * 0.114f;
}

static char pixel_to_ascii(float brightness) {
  const int index = static_cast<int>((brightness / 255.0f) * (ASCII_CHARS_COUNT - 1));
  return ASCII_CHARS[std::clamp(index, 0, ASCII_CHARS_COUNT - 1)];
}

std::string convert_pixels_to_ascii(const uint8_t* pixels, uint32_t width, uint32_t height,
                                    uint32_t target_width) {
  if (!pixels || width == 0 || height == 0 || target_width == 0) {
    return {};
  }

  const float aspect_ratio  = 0.5f;
  uint32_t    target_height = static_cast<uint32_t>(
      static_cast<float>(height) * (static_cast<float>(target_width) / static_cast<float>(width)) *
      aspect_ratio);

  if (target_height == 0) {
    target_height = 1;
  }

  const float scale_x = static_cast<float>(width) / static_cast<float>(target_width);
  const float scale_y = static_cast<float>(height) / static_cast<float>(target_height);

  std::string result;
  result.reserve((target_width + 1) * target_height);

  for (uint32_t y = 0; y < target_height; ++y) {
    for (uint32_t x = 0; x < target_width; ++x) {
      const uint32_t px = static_cast<uint32_t>(static_cast<float>(x) * scale_x);
      const uint32_t py = static_cast<uint32_t>(static_cast<float>(y) * scale_y);

      const uint32_t idx = (py * width + px) * 4;

      const float brightness = calculate_brightness(pixels[idx], pixels[idx + 1], pixels[idx + 2]);
      result += pixel_to_ascii(brightness);
    }
    result += '\n';
  }

  return result;
}

}  // namespace dmcp
