#include "dmcp/core/png.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MALLOC(sz) std::malloc(sz)
#define STBI_FREE(p) std::free(p)
#include <cstdlib>
#include <string>
#include <vector>

#include "stb_image_write.h"

namespace dmcp::detail {

namespace {

void write_callback(void* context, void* data, int size) {
  auto* buffer = static_cast<std::vector<uint8_t>*>(context);
  auto* bytes = static_cast<uint8_t*>(data);
  buffer->insert(buffer->end(), bytes, bytes + size);
}

}  // namespace

expected<std::vector<uint8_t>, std::string> EncodePng(
    const ScreenshotFrame& frame) {
  if (!frame.pixels || frame.width == 0 || frame.height == 0 ||
      frame.stride == 0) {
    return tl::unexpected<std::string>("invalid screenshot frame");
  }

  std::vector<uint8_t> buffer;
  buffer.reserve(static_cast<size_t>(frame.width) *
                 static_cast<size_t>(frame.height) * 4);

  int result = stbi_write_png_to_func(
      write_callback, &buffer, static_cast<int>(frame.width),
      static_cast<int>(frame.height), 4, frame.pixels,
      static_cast<int>(frame.stride));

  if (result == 0) {
    return tl::unexpected<std::string>("failed to encode png");
  }

  return buffer;
}

}  // namespace dmcp::detail
