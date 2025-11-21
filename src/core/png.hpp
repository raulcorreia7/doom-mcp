#pragma once

#include <optional>
#include <string>
#include <vector>

#include "screenshot.hpp"

namespace dmcp::detail {

std::optional<std::vector<uint8_t>> EncodePng(const ScreenshotFrame& frame);

}
