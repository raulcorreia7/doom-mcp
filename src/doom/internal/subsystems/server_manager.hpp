#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>

#include "dmcp/doom/config.h"
#include "mcp/generic/server.h"

namespace dmcp {

// Forward declaration
struct context;

class server_manager {
 public:
  explicit server_manager(context* ctx);
  ~server_manager();

  bool initialize(const dmcp_config_t& config);
  void shutdown();

  bool          is_running() const;
  mcp_server_t* get_server() const { return server_; }

  void     broadcast_event(const char* event_type, const char* data);
  uint32_t get_client_count() const;

 private:
  context*      ctx_;
  mcp_server_t* server_ = nullptr;
};

}  // namespace dmcp
