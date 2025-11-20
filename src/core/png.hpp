#pragma once

#include <string>
#include <vector>
#include <optional>

#include "screenshot.hpp"

namespace dmcp::detail {

std::optional<std::vector<uint8_t>> EncodePng(
    const ScreenshotFrame& frame);

}
