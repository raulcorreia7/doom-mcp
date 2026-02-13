#include <chrono>
#include <cstring>
#include <memory>
#include <string>

#include "dmcp/doom/api.h"
#include "internal.hpp"
#include "mcp/generic/server.h"

namespace dmcp {

bool handle_tools_list(void* user_data, const char* method,
                       const char* request_json, char* response_buffer,
                       size_t response_size);

bool handle_tools_call(void* user_data, const char* method,
                       const char* request_json, char* response_buffer,
                       size_t response_size);

std::string snapshot_to_json(const dmcp_snapshot_t& snapshot);

pool_entry* acquire_snapshot(std::vector<pool_entry>& pool);
void        release_snapshot(std::vector<pool_entry>& pool, pool_entry* entry);

}  // namespace dmcp

extern "C" {

dmcp_context_t* dmcp_context_create(const dmcp_config_t* config) {
  auto ctx = std::make_unique<dmcp::context>();

  ctx->config = config ? *config : dmcp_config_default();

  ctx->pool.resize(ctx->config.snapshot_pool_size);
  for (auto& entry : ctx->pool) {
    entry.in_use = false;
    dmcp_snapshot_clear(&entry.data);
  }

  ctx->min_interval =
      std::chrono::nanoseconds{1'000'000'000ull / ctx->config.target_hz};
  ctx->last_snapshot_time =
      std::chrono::steady_clock::now() - ctx->min_interval;

  ctx->screenshot.enabled.store(ctx->config.screenshot.enable);

  ctx->cmd_queue = std::make_unique<dmcp::command_queue>();

  mcp_server_config_t server_config = mcp_default_config();
  server_config.port                = ctx->config.port;
  server_config.on_log              = ctx->config.on_log;
  server_config.log_user_data       = ctx->config.user_data;

  ctx->server = mcp_server_create(&server_config);
  if (!ctx->server) {
    return nullptr;
  }

  mcp_server_method_register(ctx->server, "tools/list", dmcp::handle_tools_list,
                             ctx.get());
  mcp_server_method_register(ctx->server, "tools/call", dmcp::handle_tools_call,
                             ctx.get());

  return reinterpret_cast<dmcp_context_t*>(ctx.release());
}

void dmcp_context_destroy(dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (ctx->server) {
    mcp_server_destroy(ctx->server);
  }

  delete ctx;
}

bool dmcp_context_is_running(const dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return false;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);
  return mcp_server_is_running(ctx->server);
}

void dmcp_context_tick(dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  const auto now = std::chrono::steady_clock::now();
  if (now - ctx->last_snapshot_time < ctx->min_interval) {
    return;
  }

  dmcp::pool_entry* snapshot = dmcp::acquire_snapshot(ctx->pool);
  if (!snapshot) {
    ctx->dropped_snapshots.fetch_add(1);
    if (!ctx->drop_warning_emitted.exchange(true)) {
      if (ctx->config.on_log) {
	ctx->config.on_log(ctx->config.user_data, MCP_LOG_WARN,
	                   "DMCP: Snapshot pool exhausted, dropping frames");
      }
    }
    return;
  }

  if (ctx->config.on_snapshot) {
    ctx->config.on_snapshot(ctx->config.user_data, &snapshot->data);
  }

  const std::string json = dmcp::snapshot_to_json(snapshot->data);
  if (!json.empty()) {
    mcp_server_event_broadcast(ctx->server, "state", json.c_str());
  }

  dmcp::release_snapshot(ctx->pool, snapshot);

  ctx->last_snapshot_time = now;
}

bool dmcp_screenshot_is_requested(const dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return false;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) return false;
  return ctx->screenshot.pending_requests.load() > 0;
}

mcp_result_generic_t dmcp_screenshot_submit(
    dmcp_context_t* ctx_handle, const dmcp_screenshot_frame_t* frame) {
  if (!ctx_handle || !frame) {
    return MCP_ERROR_INVALID_ARGS;
  }

  if (!frame->pixels || frame->width == 0 || frame->height == 0) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) {
    return MCP_ERROR_DISABLED;
  }

  return MCP_RESULT_ERROR(MCP_RESULT_CODE_DISABLED,
                          "Screenshot feature not yet implemented");
}

void dmcp_stats_get(const dmcp_context_t* ctx_handle, dmcp_stats_t* stats) {
  if (!ctx_handle || !stats) return;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);

  stats->dropped_snapshots   = ctx->dropped_snapshots.load();
  stats->dropped_screenshots = ctx->dropped_screenshots.load();

  mcp_server_stats_t server_stats{};
  mcp_server_stats_get(ctx->server, &server_stats);
  stats->connected_clients = server_stats.connected_clients;
}

int dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot, char* buffer,
                          size_t buffer_size) {
  if (!snapshot || !buffer || buffer_size == 0) {
    return -1;
  }

  const std::string json = dmcp::snapshot_to_json(*snapshot);
  if (json.size() >= buffer_size) {
    return -1;
  }

  std::strcpy(buffer, json.c_str());
  return static_cast<int>(json.size());
}
}
