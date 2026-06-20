#pragma once

#include "dmcp/doom/dmcp.h"
#include "mcp/cxx/status.hpp"

#include <utility>

namespace dmcp::doom {

[[nodiscard]] inline dmcp_config_t default_config() noexcept { return dmcp_config_default(); }

class context {
 public:
  context() : context(default_config()) {}
  explicit context(const dmcp_config_t& config) : handle_(dmcp_context_create(&config)) {}
  explicit context(dmcp_context_t* handle) noexcept : handle_(handle) {}

  context(const context&)            = delete;
  context& operator=(const context&) = delete;

  context(context&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
  context& operator=(context&& other) noexcept {
    if (this != &other) {
      reset(std::exchange(other.handle_, nullptr));
    }
    return *this;
  }

  ~context() { reset(); }

  [[nodiscard]] bool            valid() const noexcept { return handle_ != nullptr; }
  [[nodiscard]] explicit        operator bool() const noexcept { return valid(); }
  [[nodiscard]] dmcp_context_t* native() const noexcept { return handle_; }
  [[nodiscard]] bool is_running() const noexcept { return dmcp_context_is_running(handle_); }

  void reset(dmcp_context_t* handle = nullptr) noexcept {
    if (handle_) {
      dmcp_context_destroy(handle_);
    }
    handle_ = handle;
  }

  [[nodiscard]] dmcp_context_t* release() noexcept { return std::exchange(handle_, nullptr); }

  void tick() noexcept { dmcp_context_tick(handle_); }

  [[nodiscard]] mcp::status push_command(dmcp_command_t& command) noexcept {
    return mcp::status(dmcp_command_push(handle_, &command));
  }

  [[nodiscard]] bool pop_command(dmcp_command_t& out_command) noexcept {
    return dmcp_command_pop(handle_, &out_command);
  }

  [[nodiscard]] uint32_t command_count() const noexcept { return dmcp_command_count(handle_); }

 private:
  dmcp_context_t* handle_{nullptr};
};

}  // namespace dmcp::doom
