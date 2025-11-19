#include "dmcp/dmcp.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <inttypes.h>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "dmcp/core/png.hpp"
#include "dmcp/core/server.hpp"
#include "dmcp/pool.hpp"
#include "dmcp/schema.hpp"
#include "dmcp/screenshot.hpp"
#include "readerwriterqueue.h"

using dmcp::detail::ServerRunner;
using SnapshotQueue = moodycamel::ReaderWriterQueue<dmcp::Snapshot*, 512>;

namespace {

struct RuntimeState {
  std::mutex mutex;
  bool initialized = false;
  dmcp_config_t config{};
  std::unique_ptr<dmcp::SnapshotPool> pool;
  std::unique_ptr<SnapshotQueue> queue;
  std::unique_ptr<ServerRunner> server;
  dmcp::ScreenshotState screenshot;
  std::chrono::nanoseconds min_interval{};
  std::chrono::steady_clock::time_point last_snapshot_time{};
  std::atomic<bool> screenshot_enabled{false};
  std::atomic<uint64_t> dropped_snapshots{0};
  std::atomic<bool> drop_warning_emitted{false};
} g_state;

dmcp_config_t normalize_config(const dmcp_config_t* config) {
  dmcp_config_t cfg = dmcp_default_config();
  if (config) {
    cfg = *config;
  }
  if (cfg.port == 0) {
    cfg.port = 9090;
  }
  if (cfg.target_hz == 0) {
    cfg.target_hz = 10;
  }
  if (cfg.snapshot_pool == 0) {
    cfg.snapshot_pool = 16;
  }
  if (cfg.queue_slots == 0) {
    cfg.queue_slots = 32;
  }
  if (cfg.enemy_capacity == 0) {
    cfg.enemy_capacity = 256;
  }
  if (cfg.inventory_capacity == 0) {
    cfg.inventory_capacity = 64;
  }
  if (cfg.screenshot.width == 0) {
    cfg.screenshot.width = 640;
  }
  if (cfg.screenshot.height == 0) {
    cfg.screenshot.height = 480;
  }
  return cfg;
}

}  // namespace

extern "C" void __attribute__((weak)) DMCP_Process_Tick_Impl(
    dmcp::Snapshot* snapshot) {
  if (snapshot) {
    snapshot->Clear();
  }
}

dmcp_config_t dmcp_default_config(void) {
  dmcp_config_t cfg{};
  cfg.port = 9090;
  cfg.target_hz = 10;
  cfg.snapshot_pool = 16;
  cfg.queue_slots = 32;
  cfg.enemy_capacity = 512;
  cfg.inventory_capacity = 64;
  cfg.screenshot.enable = true;
  cfg.screenshot.width = 640;
  cfg.screenshot.height = 480;
  return cfg;
}

int dmcp_init(const dmcp_config_t* config) {
  auto cfg = normalize_config(config);

  std::unique_lock lock(g_state.mutex);
  if (g_state.initialized) {
    return -1;
  }

  g_state.pool = std::make_unique<dmcp::SnapshotPool>(
      cfg.snapshot_pool, cfg.enemy_capacity, cfg.inventory_capacity);
  g_state.queue = std::make_unique<SnapshotQueue>(cfg.queue_slots);
  g_state.server = std::make_unique<ServerRunner>(
      g_state.queue.get(), g_state.pool.get(), &g_state.screenshot,
      cfg.screenshot.enable);

  if (!g_state.server->start(cfg.port)) {
    g_state.server.reset();
    g_state.queue.reset();
    g_state.pool.reset();
    return -2;
  }

  g_state.config = cfg;
  auto interval_ns = std::chrono::nanoseconds(1'000'000'000ull / cfg.target_hz);
  g_state.min_interval = interval_ns;
  g_state.last_snapshot_time =
      std::chrono::steady_clock::now() - g_state.min_interval;
  g_state.screenshot_enabled.store(cfg.screenshot.enable,
                                   std::memory_order_release);
  g_state.initialized = true;
  return 0;
}

void dmcp_shutdown(void) {
  std::unique_lock lock(g_state.mutex);
  if (!g_state.initialized) {
    return;
  }
  auto server = std::move(g_state.server);
  g_state.initialized = false;
  g_state.screenshot_enabled.store(false, std::memory_order_release);
  lock.unlock();

  if (server) {
    server->request_stop();
    server->join();
  }

  lock.lock();
  g_state.queue.reset();
  g_state.pool.reset();
}

bool dmcp_is_running(void) {
  std::lock_guard lock(g_state.mutex);
  return g_state.initialized && g_state.server && g_state.server->is_running();
}

void dmcp_process_tick(void) {
  SnapshotQueue* queue = nullptr;
  dmcp::SnapshotPool* pool = nullptr;
  std::chrono::nanoseconds min_interval;
  auto now = std::chrono::steady_clock::now();

  {
    std::unique_lock lock(g_state.mutex);
    if (!g_state.initialized || !g_state.queue || !g_state.pool) {
      return;
    }
    if (now - g_state.last_snapshot_time < g_state.min_interval) {
      return;
    }
    queue = g_state.queue.get();
    pool = g_state.pool.get();
  }

  dmcp::Snapshot* snapshot = pool->Acquire();
  if (!snapshot) {
    return;
  }

  DMCP_Process_Tick_Impl(snapshot);
  if (queue->try_enqueue(snapshot)) {
    std::lock_guard lock(g_state.mutex);
    g_state.last_snapshot_time = now;
  } else {
    pool->Release(snapshot);
    auto drops = g_state.dropped_snapshots.fetch_add(1, std::memory_order_relaxed) + 1;
    if (!g_state.drop_warning_emitted.exchange(true, std::memory_order_relaxed)) {
      std::fprintf(stderr,
                   "[dmcp] Snapshot queue full, dropping frames (total %" PRIu64 ")\n",
                   drops);
    }
  }
}

bool dmcp_consume_screenshot_request(void) {
  if (!g_state.screenshot_enabled.load(std::memory_order_acquire)) {
    return false;
  }
  auto pending =
      g_state.screenshot.pending_requests.load(std::memory_order_acquire);
  while (pending > 0) {
    if (g_state.screenshot.pending_requests.compare_exchange_weak(
            pending, pending - 1, std::memory_order_acq_rel,
            std::memory_order_acquire)) {
      return true;
    }
  }
  return false;
}

int dmcp_submit_screenshot(const dmcp_screenshot_frame_t* frame) {
  if (!frame) {
    return -1;
  }
  if (!g_state.screenshot_enabled.load(std::memory_order_acquire)) {
    return -3;
  }
  dmcp::ScreenshotFrame view{frame->pixels, frame->width, frame->height,
                             frame->stride};
  auto png = dmcp::detail::EncodePng(view);
  if (!png.has_value()) {
    return -2;
  }
  {
    std::scoped_lock lock(g_state.screenshot.data_mutex);
    g_state.screenshot.latest_png = std::move(png.value());
    g_state.screenshot.width = frame->width;
    g_state.screenshot.height = frame->height;
    g_state.screenshot.captured_at = std::chrono::system_clock::now();
  }
  g_state.screenshot.version.fetch_add(1, std::memory_order_release);
  return 0;
}

uint64_t dmcp_dropped_snapshot_count(void) {
  return g_state.dropped_snapshots.load(std::memory_order_relaxed);
}
