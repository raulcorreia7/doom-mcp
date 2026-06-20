#pragma once

#include "mcp/core/status.h"

#include <cstdint>
#include <string_view>

namespace mcp {

/**
 * Source-stable C++ view over the C status API.
 */
class status {
 public:
  constexpr status() noexcept = default;
  constexpr explicit status(mcp_status_t value) noexcept : value_(value) {}

  [[nodiscard]] constexpr bool ok() const noexcept { return value_.code == MCP_STATUS_CODE_OK; }
  [[nodiscard]] constexpr int32_t code() const noexcept { return value_.code; }
  [[nodiscard]] constexpr const char* c_message() const noexcept { return value_.message; }
  [[nodiscard]] constexpr std::string_view message() const noexcept {
    return value_.message ? std::string_view(value_.message) : std::string_view{};
  }
  [[nodiscard]] constexpr mcp_status_t native() const noexcept { return value_; }

  [[nodiscard]] static status success() noexcept { return status(mcp_status_ok()); }
  [[nodiscard]] static status error(int32_t code, const char* message) noexcept {
    return status(mcp_status_error(code, message));
  }

 private:
  mcp_status_t value_{MCP_STATUS_CODE_OK, "Success"};
};

[[nodiscard]] constexpr status from_c(mcp_status_t value) noexcept { return status(value); }

}  // namespace mcp
