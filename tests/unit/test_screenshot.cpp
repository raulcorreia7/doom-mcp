#include <cstdint>
#include <string>
#include <vector>

#include "dmcp/doom/api.h"
#include "doom/internal/screenshot.hpp"
#include "test_utils.hpp"

namespace dmcp {

TEST_CASE("ASCII conversion basics", "[screenshot][ascii]") {
  SECTION("empty pixels returns empty string") {
    std::string result = convert_pixels_to_ascii(nullptr, 0, 100, 100, 80);
    REQUIRE(result.empty());
  }

  SECTION("zero width returns empty") {
    std::vector<uint8_t> pixels(100 * 100 * 4, 128);
    std::string result = convert_pixels_to_ascii(pixels.data(), pixels.size(), 100, 100, 0);
    REQUIRE(result.empty());
  }

  SECTION("zero height returns empty") {
    std::vector<uint8_t> pixels(100 * 100 * 4, 128);
    std::string          result = convert_pixels_to_ascii(pixels.data(), pixels.size(), 100, 0, 80);
    REQUIRE(result.empty());
  }

  SECTION("undersized buffer returns empty") {
    std::vector<uint8_t> pixels(10, 128);
    std::string result = convert_pixels_to_ascii(pixels.data(), pixels.size(), 100, 100, 80);
    REQUIRE(result.empty());
  }
}

TEST_CASE("ASCII conversion output dimensions", "[screenshot][ascii]") {
  SECTION("target width respected") {
    std::vector<uint8_t> pixels(320 * 240 * 4, 128);
    std::string result = convert_pixels_to_ascii(pixels.data(), pixels.size(), 320, 240, 80);

    REQUIRE_FALSE(result.empty());

    size_t first_newline = result.find('\n');
    REQUIRE(first_newline == 80);
  }

  SECTION("aspect ratio correction applied") {
    std::vector<uint8_t> pixels(320 * 240 * 4, 128);
    std::string result = convert_pixels_to_ascii(pixels.data(), pixels.size(), 320, 240, 160);

    int line_count = 0;
    for (char c : result) {
      if (c == '\n') line_count++;
    }

    REQUIRE(line_count > 0);
    REQUIRE(line_count < 160);
  }
}

TEST_CASE("ASCII conversion produces valid output", "[screenshot][ascii]") {
  SECTION("solid color produces consistent output") {
    std::vector<uint8_t> pixels(320 * 240 * 4, 128);
    std::string result = convert_pixels_to_ascii(pixels.data(), pixels.size(), 320, 240, 80);

    REQUIRE_FALSE(result.empty());
    REQUIRE(result.size() > 80);
  }

  SECTION("grayscale gradient produces variety") {
    std::vector<uint8_t> pixels;
    for (int i = 0; i < 256; i++) {
      pixels.push_back(static_cast<uint8_t>(i));
      pixels.push_back(static_cast<uint8_t>(i));
      pixels.push_back(static_cast<uint8_t>(i));
      pixels.push_back(255);
    }

    std::string result = convert_pixels_to_ascii(pixels.data(), pixels.size(), 256, 1, 64);
    REQUIRE_FALSE(result.empty());

    bool has_variety = false;
    for (size_t i = 1; i < result.size() && !has_variety; i++) {
      if (result[i] != result[i - 1] && result[i] != '\n') {
        has_variety = true;
      }
    }
    REQUIRE(has_variety);
  }

  SECTION("color image converts correctly") {
    std::vector<uint8_t> pixels;
    for (int y = 0; y < 100; y++) {
      for (int x = 0; x < 100; x++) {
        pixels.push_back(static_cast<uint8_t>(x * 2));
        pixels.push_back(static_cast<uint8_t>(y * 2));
        pixels.push_back(128);
        pixels.push_back(255);
      }
    }

    std::string result = convert_pixels_to_ascii(pixels.data(), pixels.size(), 100, 100, 40);
    REQUIRE_FALSE(result.empty());

    size_t newline_count = 0;
    for (char c : result) {
      if (c == '\n') newline_count++;
    }
    REQUIRE(newline_count > 0);
  }
}

TEST_CASE("screenshot_state conversion", "[screenshot][state]") {
  screenshot_state state;

  SECTION("empty state returns empty ascii") {
    state.convert_to_ascii(80);
    REQUIRE(state.get_ascii().empty());
  }

  SECTION("with pixels converts successfully") {
    std::vector<uint8_t> pixels(320 * 240 * 4, 128);
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      state.latest_pixels = pixels;
      state.width         = 320;
      state.height        = 240;
    }

    state.convert_to_ascii(80);
    REQUIRE_FALSE(state.get_ascii().empty());
  }

  SECTION("different target widths produce different sizes") {
    std::vector<uint8_t> pixels(320 * 240 * 4, 200);
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      state.latest_pixels = pixels;
      state.width         = 320;
      state.height        = 240;
    }

    state.convert_to_ascii(80);
    std::string ascii80 = state.get_ascii();

    state.convert_to_ascii(160);
    std::string ascii160 = state.get_ascii();

    REQUIRE(ascii80.size() < ascii160.size());
  }
}

}  // namespace dmcp
