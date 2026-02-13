#pragma once
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "command_queue.hpp"
#include "dmcp/doom/types.h"
#include "mcp/generic/server.h"
#include "pool.hpp"
#include "screenshot.hpp"

namespace dmcp {

struct context {
  dmcp_config_t                  config{};
  mcp_server_t*                  server = nullptr;
  std::unique_ptr<command_queue> cmd_queue;

  std::vector<pool_entry> pool;
  std::mutex              pool_mutex;

  std::chrono::nanoseconds              min_interval{};
  std::chrono::steady_clock::time_point last_snapshot_time;

  screenshot_state screenshot;

  std::atomic<uint64_t> dropped_snapshots{0};
  std::atomic<uint64_t> dropped_screenshots{0};
  std::atomic<bool>     drop_warning_emitted{false};

  std::unordered_map<
      std::string, std::pair<dmcp_command_type_t, dmcp_custom_command_parser_t>>
	     custom_parsers;
  std::mutex custom_parsers_mutex;
};

}  // namespace dmcp
