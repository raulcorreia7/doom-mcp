#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

#include "App.h"
#include "dmcp/pool.hpp"
#include "dmcp/screenshot.hpp"
#include "dmcp/schema.hpp"

namespace uWS {
struct Loop;
}

namespace moodycamel {
template <typename T, size_t MAX_BLOCK_SIZE>
class ReaderWriterQueue;
}

struct us_listen_socket_t;

namespace dmcp::detail {

using SnapshotQueueType = moodycamel::ReaderWriterQueue<Snapshot*, 512>;

class ServerRunner {
 public:
  ServerRunner(SnapshotQueueType* queue, SnapshotPool* pool,
               ScreenshotState* screenshot,
               bool screenshot_enabled);
  ~ServerRunner();

  bool start(uint16_t port);
  void request_stop();
  void join();
  bool is_running() const { return running_.load(std::memory_order_acquire); }

 private:
  void threadMain(uint16_t port);

  SnapshotQueueType* queue_;
  SnapshotPool* pool_;
  ScreenshotState* screenshot_;
  const bool screenshot_enabled_;

  std::atomic<bool> running_{false};
  std::atomic<bool> stopping_{false};

  std::thread thread_;
  std::unique_ptr<uWS::App> app_;
  std::atomic<uWS::App*> app_instance_{nullptr};
  std::atomic<us_listen_socket_t*> listen_socket_{nullptr};
  uWS::Loop* loop_ = nullptr;
};

}  // namespace dmcp::detail
