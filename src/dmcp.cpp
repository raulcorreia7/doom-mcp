#include "dmcp/dmcp.h"

#include <inttypes.h>

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "core/png.hpp"
#include "core/pool.hpp"
#include "core/screenshot.hpp"
#include "core/server.hpp"
#include "core/schema.hpp"
#include "readerwriterqueue.h"

using dmcp::detail::ServerRunner;
using dmcp::detail::OwnedScreenshot;
using SnapshotQueue = moodycamel::ReaderWriterQueue<dmcp::Snapshot*, 512>;
using ScreenshotQueue = moodycamel::ReaderWriterQueue<OwnedScreenshot, 4>;

struct dmcp_context_s {
  std::mutex                            mutex;
  bool                                  initialized = false;
  dmcp_config_t                         config{};
  std::unique_ptr<dmcp::SnapshotPool>   pool;
  std::unique_ptr<SnapshotQueue>        queue;
  std::unique_ptr<ScreenshotQueue>      screenshot_queue;
  std::unique_ptr<ServerRunner>         server;
  dmcp::ScreenshotState                 screenshot;
  std::chrono::nanoseconds              min_interval{};
  std::chrono::steady_clock::time_point last_snapshot_time{};
  std::atomic<bool>                     screenshot_enabled{false};
  std::atomic<uint64_t>                 dropped_snapshots{0};
  std::atomic<uint64_t>                 dropped_screenshots{0};
  std::atomic<uint64_t>                 connected_clients{0};
  std::atomic<bool>                     drop_warning_emitted{false};
};

namespace {

dmcp_config_t normalize_config(const dmcp_config_t* config) {
  dmcp_config_t cfg = dmcp_default_config();
  if (config) {
    cfg = *config;
  }
  if (cfg.port == 0) cfg.port = 9090;
  if (cfg.target_hz == 0) cfg.target_hz = 10;
  if (cfg.snapshot_pool == 0) cfg.snapshot_pool = 16;
  if (cfg.queue_slots == 0) cfg.queue_slots = 32;
  if (cfg.enemy_capacity == 0) cfg.enemy_capacity = 256;
  if (cfg.inventory_capacity == 0) cfg.inventory_capacity = 64;
  if (cfg.screenshot.width == 0) cfg.screenshot.width = 640;
  if (cfg.screenshot.height == 0) cfg.screenshot.height = 480;
  return cfg;
}

// Internal Logging Helper
void Log(dmcp_context_t* ctx, dmcp_log_level_t level, const char* fmt, ...) {
    if (!ctx) return;

    // 1. Setup va_list
    va_list args;
    va_start(args, fmt);

    // 2. Determine required size
    va_list args_copy;
    va_copy(args_copy, args);
    int len = std::vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    if (len < 0) {
        va_end(args);
        return; // Encoding error
    }

    // 3. Format string into buffer
    std::vector<char> buf(static_cast<size_t>(len) + 1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args);
    va_end(args);

    // 4. Route to appropriate output
    if (ctx->config.on_log) {
        // A. Use Engine's Logger (Override)
        ctx->config.on_log(ctx->config.user_data, level, buf.data());
    } else {
        // B. Default Fallback (stdout/stderr)
        // Only print if we don't have a custom logger
        FILE* out = (level >= DMCP_LOG_ERROR) ? stderr : stdout;
        fprintf(out, "[DMCP] %s\n", buf.data());
    }
}

} // namespace

// Helper Macros for internal use within libdmcp
#define LOG_INFO(ctx, ...)  Log(ctx, DMCP_LOG_INFO, __VA_ARGS__)
#define LOG_WARN(ctx, ...)  Log(ctx, DMCP_LOG_WARN, __VA_ARGS__)
#define LOG_ERROR(ctx, ...) Log(ctx, DMCP_LOG_ERROR, __VA_ARGS__)
#define LOG_DEBUG(ctx, ...) Log(ctx, DMCP_LOG_DEBUG, __VA_ARGS__)

dmcp_config_t dmcp_default_config(void) {
  dmcp_config_t cfg{};
  cfg.struct_size        = sizeof(dmcp_config_t);
  cfg.port               = 9090;
  cfg.target_hz          = 10;
  cfg.snapshot_pool      = 16;
  cfg.queue_slots        = 32;
  cfg.enemy_capacity     = 512;
  cfg.inventory_capacity = 64;
  cfg.screenshot.enable  = true;
  cfg.screenshot.width   = 640;
  cfg.screenshot.height  = 480;
  cfg.on_tick            = nullptr;
  cfg.on_log             = nullptr;
  cfg.user_data          = nullptr;
  return cfg;
}

dmcp_context_t* dmcp_create(const dmcp_config_t* config) {
  auto* ctx = new dmcp_context_s();
  auto  cfg = normalize_config(config);

  ctx->pool = std::make_unique<dmcp::SnapshotPool>(
      cfg.snapshot_pool, cfg.enemy_capacity, cfg.inventory_capacity);
  ctx->queue = std::make_unique<SnapshotQueue>(cfg.queue_slots);
  ctx->screenshot_queue = std::make_unique<ScreenshotQueue>(4);
  ctx->server =
      std::make_unique<ServerRunner>(ctx->queue.get(), ctx->screenshot_queue.get(),
                                     ctx->pool.get(), &ctx->screenshot, cfg.screenshot.enable,
                                     &ctx->connected_clients);

  if (!ctx->server->start(cfg.port)) {
    delete ctx;
    return nullptr;
  }

  ctx->config      = cfg;
  auto interval_ns = std::chrono::nanoseconds(1'000'000'000ull / cfg.target_hz);
  ctx->min_interval = interval_ns;
  ctx->last_snapshot_time =
      std::chrono::steady_clock::now() - ctx->min_interval;
  ctx->screenshot_enabled.store(cfg.screenshot.enable,
                                std::memory_order_release);
  ctx->initialized = true;
  return ctx;
}

void dmcp_destroy(dmcp_context_t* ctx) {
  if (!ctx) return;

  // Stop the server first to ensure no one is reading the queue/pool
  if (ctx->server) {
    ctx->server->request_stop();
    ctx->server->join();
  }

  delete ctx;
}

bool dmcp_is_running(dmcp_context_t* ctx) {
  if (!ctx) return false;
  std::lock_guard lock(ctx->mutex);
  return ctx->initialized && ctx->server && ctx->server->is_running();
}

void dmcp_update(dmcp_context_t* ctx) {
  if (!ctx) return;

  SnapshotQueue*      queue = nullptr;
  dmcp::SnapshotPool* pool  = nullptr;
  auto                now   = std::chrono::steady_clock::now();

  {
    // We only need to protect the pointers if we expect concurrent destroy,
    // but for now we assume update/destroy are serialized by the user.
    // However, checking initialized is good practice.
    if (!ctx->initialized) return;

    if (now - ctx->last_snapshot_time < ctx->min_interval) {
      return;
    }
    queue = ctx->queue.get();
    pool  = ctx->pool.get();
  }

  dmcp::Snapshot* snapshot = pool->Acquire();
  if (!snapshot) {
    return;
  }

  if (ctx->config.on_tick) {
    ctx->config.on_tick(ctx->config.user_data, snapshot);
  }

  if (queue->try_enqueue(snapshot)) {
    ctx->last_snapshot_time = now;
  } else {
    pool->Release(snapshot);
    auto drops =
	ctx->dropped_snapshots.fetch_add(1, std::memory_order_relaxed) + 1;
    if (!ctx->drop_warning_emitted.exchange(true, std::memory_order_relaxed)) {
      Log(ctx, DMCP_LOG_WARN,
          "[dmcp] Snapshot queue full, dropping frames (total %" PRIu64 ")",
          drops);
    }
  }
}

bool dmcp_has_screenshot_request(dmcp_context_t* ctx) {
  if (!ctx) return false;
  if (!ctx->screenshot_enabled.load(std::memory_order_acquire)) {
    return false;
  }
  return ctx->screenshot.pending_requests.load(std::memory_order_acquire) > 0;
}

dmcp_result_t dmcp_submit_screenshot(dmcp_context_t*                ctx,
                                     const dmcp_screenshot_frame_t* frame) {
  if (!ctx || !frame) {
    return DMCP_ERROR_INVALID_ARGS;
  }
  if (!ctx->screenshot_enabled.load(std::memory_order_acquire)) {
    return DMCP_ERROR_DISABLED;
  }

  OwnedScreenshot owned;
  owned.width = frame->width;
  owned.height = frame->height;
  owned.stride = frame->stride;

  size_t size = frame->height * frame->stride;
  try {
    owned.pixels.resize(size);
    std::copy_n(frame->pixels, size, owned.pixels.begin());
  } catch (...) {
    return DMCP_ERROR_ENCODING_FAILED; // Allocation failed
  }

  if (!ctx->screenshot_queue->try_enqueue(std::move(owned))) {
    ctx->dropped_screenshots.fetch_add(1, std::memory_order_relaxed);
    return DMCP_ERROR_QUEUE_FULL;
  }

  auto prev = ctx->screenshot.pending_requests.fetch_sub(1, std::memory_order_relaxed);
  if (prev == 0) {
    ctx->screenshot.pending_requests.fetch_add(1, std::memory_order_relaxed);
  }

  return DMCP_OK;
}

void dmcp_get_stats(const dmcp_context_t* ctx, dmcp_stats_t* stats) {
  if (!ctx || !stats) return;
  stats->dropped_snapshots = ctx->dropped_snapshots.load(std::memory_order_relaxed);
  stats->dropped_screenshots = ctx->dropped_screenshots.load(std::memory_order_relaxed);
  stats->connected_clients = ctx->connected_clients.load(std::memory_order_relaxed);
}
