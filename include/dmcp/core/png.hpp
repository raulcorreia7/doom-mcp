#pragma once

#include <string>
#include <vector>

#include "dmcp/common.hpp"
#include "dmcp/screenshot.hpp"

namespace dmcp::detail {

expected<std::vector<uint8_t>, std::string> EncodePng(
    const ScreenshotFrame& frame);

}
