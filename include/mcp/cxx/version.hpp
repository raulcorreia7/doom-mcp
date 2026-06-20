#pragma once

#include "mcp/core/api.h"
#include "mcp/core/version.h"

#include <cstdint>
#include <string_view>

namespace mcp {

struct version {
  uint32_t         major;
  uint32_t         minor;
  uint32_t         patch;
  std::string_view text;
};

[[nodiscard]] inline version core_version() noexcept {
  return version{
      mcp_core_version_major(),
      mcp_core_version_minor(),
      mcp_core_version_patch(),
      mcp_core_version_string(),
  };
}

[[nodiscard]] inline mcp_core_api_info_t core_api() noexcept {
  mcp_core_api_info_t info{};
  mcp_core_api_get(&info);
  return info;
}

}  // namespace mcp
