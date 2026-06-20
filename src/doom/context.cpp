#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <string>

#include "dmcp/doom/api.h"
#include "internal.hpp"
#include "internal/mcp_registration.hpp"
#include "internal/serialization.hpp"
#include "mcp/core/memory.h"
#include "mcp/core/string.h"
#include "mcp/generic/server.h"

// Bring dmcp_log into scope for use inside extern "C" block
using dmcp::dmcp_log;

extern "C" {

dmcp_context_t* dmcp_context_create(const dmcp_config_t* config) {
  auto ctx = std::make_unique<dmcp::context>();

  ctx->config = dmcp_config_default();
  if (config) {
    size_t copy_size = config->struct_size;
    if (copy_size == 0 || copy_size > sizeof(dmcp_config_t)) {
      copy_size = sizeof(dmcp_config_t);
    }
    mcp_memcpy_safe(&ctx->config, sizeof(ctx->config), config, copy_size);
  }

  if (ctx->config.target_hz == 0) {
    dmcp_log(ctx.get(), MCP_LOG_ERROR, "Invalid DMCP config: target_hz must be greater than zero");
    return nullptr;
  }
  if (ctx->config.snapshot_pool_size == 0) {
    dmcp_log(ctx.get(), MCP_LOG_ERROR,
             "Invalid DMCP config: snapshot_pool_size must be greater than zero");
    return nullptr;
  }
  if (ctx->config.command_queue_slots == 0 ||
      ctx->config.command_queue_slots > std::numeric_limits<uint32_t>::max()) {
    dmcp_log(ctx.get(), MCP_LOG_ERROR,
             "Invalid DMCP config: command_queue_slots must be in range 1..UINT32_MAX");
    return nullptr;
  }

  ctx->pool.resize(ctx->config.snapshot_pool_size);
  for (auto& entry : ctx->pool) {
    entry.in_use = false;
    dmcp_snapshot_clear(&entry.data);
  }

  ctx->min_interval       = std::chrono::nanoseconds{1'000'000'000ull / ctx->config.target_hz};
  ctx->last_snapshot_time = std::chrono::steady_clock::now() - ctx->min_interval;

  ctx->screenshot.enabled.store(ctx->config.screenshot.enable);

  ctx->cmd_queue             = std::make_unique<dmcp::command_queue>();
  ctx->input_queue           = std::make_unique<dmcp::command_queue>();
  ctx->cmd_queue->max_size   = static_cast<uint32_t>(ctx->config.command_queue_slots);
  ctx->input_queue->max_size = static_cast<uint32_t>(ctx->config.command_queue_slots);

  mcp_server_config_t server_config = mcp_default_config();
  server_config.port                = ctx->config.port;
  server_config.on_log              = ctx->config.on_log;
  server_config.log_user_data       = ctx->config.user_data;
  server_config.start_transport     = ctx->config.start_transport;
  mcp_strcpy_safe(server_config.server_name, sizeof(server_config.server_name), "doom-mcp");

  ctx->server = mcp_server_create(&server_config);
  if (!ctx->server) {
    dmcp_log(ctx.get(), MCP_LOG_ERROR, "Failed to create MCP server on port=%u", ctx->config.port);
    return nullptr;
  }

  if (!dmcp::register_mcp_surface(ctx.get())) {
    if (ctx->server) {
      mcp_server_destroy(ctx->server);
      ctx->server = nullptr;
    }
    return nullptr;
  }

  dmcp_log(ctx.get(), MCP_LOG_INFO, "DMCP context created (port=%u target_hz=%u screenshot=%s)",
           ctx->config.port, ctx->config.target_hz,
           ctx->config.screenshot.enable ? "enabled" : "disabled");

  return reinterpret_cast<dmcp_context_t*>(ctx.release());
}

void dmcp_context_destroy(dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  dmcp_log(ctx, MCP_LOG_INFO, "DMCP context destroying");

  if (ctx->server) {
    mcp_server_destroy(ctx->server);
    ctx->server = nullptr;
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

  {
    std::lock_guard<std::mutex> lock(ctx->last_snapshot_mutex);
    ctx->last_snapshot = snapshot->data;
  }

  const std::string json = dmcp::snapshot_to_json(snapshot->data);
  dmcp::broadcast_state_event(ctx, json);

  dmcp::release_snapshot(ctx->pool, snapshot);

  ctx->last_snapshot_time = now;
}

bool dmcp_screenshot_is_requested(const dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return false;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) return false;
  return ctx->screenshot.pending_requests.load() > 0;
}

bool dmcp_screenshot_request(dmcp_context_t* ctx_handle) {
  if (!ctx_handle) return false;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);
  if (!ctx->screenshot.enabled.load()) return false;

  uint32_t current = ctx->screenshot.pending_requests.load(std::memory_order_relaxed);
  while (current < 4) {
    if (ctx->screenshot.pending_requests.compare_exchange_weak(
            current, current + 1, std::memory_order_release, std::memory_order_relaxed)) {
      return true;
    }
  }

  ctx->dropped_screenshots.fetch_add(1, std::memory_order_release);
  return false;
}

mcp_status_t dmcp_screenshot_submit(dmcp_context_t*                ctx_handle,
                                    const dmcp_screenshot_frame_t* frame) {
  if (!ctx_handle || !frame) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  if (!frame->pixels || frame->width == 0 || frame->height == 0) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  const uint32_t row_bytes = frame->width * 4;
  if (frame->stride < row_bytes) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) {
    dmcp_log(ctx, MCP_LOG_WARN, "Screenshot submit rejected: screenshot feature disabled");
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_DISABLED, "Operation disabled");
  }

  {
    std::lock_guard<std::mutex> lock(ctx->screenshot.mutex);
    ctx->screenshot.width  = frame->width;
    ctx->screenshot.height = frame->height;

    const size_t total_size = static_cast<size_t>(row_bytes) * frame->height;
    ctx->screenshot.latest_pixels.resize(total_size);

    if (frame->stride == row_bytes) {
      if (!mcp_memcpy_safe(ctx->screenshot.latest_pixels.data(),
                           ctx->screenshot.latest_pixels.size(), frame->pixels, total_size)) {
        return MCP_STATUS_ERROR(MCP_STATUS_CODE_INTERNAL, "Screenshot copy failed");
      }
    } else {
      uint8_t* dest = ctx->screenshot.latest_pixels.data();
      for (uint32_t y = 0; y < frame->height; ++y) {
        const size_t dest_offset = static_cast<size_t>(y) * row_bytes;
        if (!mcp_memcpy_safe(dest + dest_offset, ctx->screenshot.latest_pixels.size() - dest_offset,
                             frame->pixels + static_cast<size_t>(y) * frame->stride, row_bytes)) {
          return MCP_STATUS_ERROR(MCP_STATUS_CODE_INTERNAL, "Screenshot row copy failed");
        }
      }
    }
  }

  if (ctx->screenshot.pending_requests.load() > 0) {
    ctx->screenshot.pending_requests.fetch_sub(1);
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "Screenshot submitted (%ux%u)", frame->width, frame->height);

  return MCP_STATUS_OK("Success");
}

const char* dmcp_screenshot_get_ascii(dmcp_context_t* ctx_handle, uint32_t target_width) {
  if (!ctx_handle) return nullptr;

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) return nullptr;

  thread_local std::string ascii;
  if (!ctx->screenshot.build_ascii(target_width, &ascii)) {
    return nullptr;
  }

  dmcp_log(ctx, MCP_LOG_DEBUG, "ASCII screenshot generated (target_width=%u)", target_width);
  return ascii.c_str();
}

int dmcp_screenshot_copy_ascii(dmcp_context_t* ctx_handle, char* buffer, size_t buffer_size,
                               uint32_t target_width) {
  if (!ctx_handle || !buffer || buffer_size == 0) {
    return -1;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) {
    return -1;
  }

  std::string ascii;
  if (!ctx->screenshot.build_ascii(target_width, &ascii)) {
    return -1;
  }

  if (ascii.size() >= buffer_size) {
    return -1;
  }

  if (!mcp_memcpy_safe(buffer, buffer_size, ascii.c_str(), ascii.size() + 1)) {
    return -1;
  }
  return static_cast<int>(ascii.size());
}

int dmcp_screenshot_to_json(dmcp_context_t* ctx_handle, char* buffer, size_t buffer_size,
                            uint32_t target_width) {
  if (!ctx_handle || !buffer || buffer_size == 0) {
    return -1;
  }

  auto* ctx = reinterpret_cast<dmcp::context*>(ctx_handle);

  if (!ctx->screenshot.enabled.load()) {
    return -1;
  }

  std::string ascii;
  uint32_t    frame_width  = 0;
  uint32_t    frame_height = 0;
  if (!ctx->screenshot.build_ascii(target_width, &ascii, &frame_width, &frame_height)) {
    return -1;
  }

  std::string json = "{";
  json += "\"width\":" + std::to_string(frame_width) + ",";
  json += "\"height\":" + std::to_string(frame_height) + ",";
  json += "\"ascii\":";

  json += "\"";
  for (char c : ascii) {
    if (c == '"') {
      json += "\\\"";
    } else if (c == '\\') {
      json += "\\\\";
    } else if (c == '\n') {
      json += "\\n";
    } else if (c == '\r') {
      json += "\\r";
    } else {
      json += c;
    }
  }
  json += "\"";

  json += "}";

  if (json.size() >= buffer_size) {
    return -1;
  }

  mcp_strcpy_safe(buffer, buffer_size, json.c_str());
  return static_cast<int>(json.size());
}

void dmcp_stats_get(const dmcp_context_t* ctx_handle, dmcp_stats_t* stats) {
  if (!ctx_handle || !stats) return;

  auto* ctx = reinterpret_cast<const dmcp::context*>(ctx_handle);

  stats->struct_size         = sizeof(dmcp_stats_t);
  stats->dropped_snapshots   = ctx->dropped_snapshots.load();
  stats->dropped_screenshots = ctx->dropped_screenshots.load();

  mcp_server_stats_t server_stats{};
  mcp_server_stats_get(ctx->server, &server_stats);
  stats->connected_clients = server_stats.connected_clients;
}

int dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot, char* buffer, size_t buffer_size) {
  if (!snapshot || !buffer || buffer_size == 0) {
    return -1;
  }

  const std::string json = dmcp::snapshot_to_json(*snapshot);
  if (json.size() >= buffer_size) {
    return -1;
  }

  mcp_strcpy_safe(buffer, buffer_size, json.c_str());
  return static_cast<int>(json.size());
}
}
