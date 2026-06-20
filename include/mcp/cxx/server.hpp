#pragma once

#include "mcp/cxx/status.hpp"
#include "mcp/generic/server.h"

#include <cstdint>
#include <utility>

namespace mcp {

class server {
 public:
  server() : server(mcp_default_config()) {}
  explicit server(const mcp_server_config_t& config) : handle_(mcp_server_create(&config)) {}
  explicit server(mcp_server_t* handle) noexcept : handle_(handle) {}

  server(const server&)            = delete;
  server& operator=(const server&) = delete;

  server(server&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
  server& operator=(server&& other) noexcept {
    if (this != &other) {
      reset(std::exchange(other.handle_, nullptr));
    }
    return *this;
  }

  ~server() { reset(); }

  [[nodiscard]] bool          valid() const noexcept { return handle_ != nullptr; }
  [[nodiscard]] explicit      operator bool() const noexcept { return valid(); }
  [[nodiscard]] mcp_server_t* native() const noexcept { return handle_; }
  [[nodiscard]] bool          is_running() const noexcept { return mcp_server_is_running(handle_); }
  [[nodiscard]] uint64_t      clients_count() const noexcept {
    return mcp_server_clients_count(handle_);
  }

  void reset(mcp_server_t* handle = nullptr) noexcept {
    if (handle_) {
      mcp_server_destroy(handle_);
    }
    handle_ = handle;
  }

  [[nodiscard]] mcp_server_t* release() noexcept { return std::exchange(handle_, nullptr); }

  [[nodiscard]] status register_method(const char* method, mcp_method_handler_t handler,
                                       void* user_data = nullptr) noexcept {
    return status(mcp_server_method_register(handle_, method, handler, user_data));
  }

  void unregister_method(const char* method) noexcept {
    mcp_server_method_unregister(handle_, method);
  }

  [[nodiscard]] status register_route(const char* method, const char* path,
                                      mcp_route_handler_t handler,
                                      void*               user_data = nullptr) noexcept {
    return status(mcp_server_route_register(handle_, method, path, handler, user_data));
  }

  void unregister_route(const char* method, const char* path) noexcept {
    mcp_server_route_unregister(handle_, method, path);
  }

  void broadcast_event(const char* event_type, const char* json_payload) noexcept {
    mcp_server_event_broadcast(handle_, event_type, json_payload);
  }

 private:
  mcp_server_t* handle_{nullptr};
};

}  // namespace mcp
