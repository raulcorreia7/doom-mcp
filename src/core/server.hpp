#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include "dmcp/schema.hpp"
#include "pool.hpp"
#include "screenshot.hpp"

namespace uWS {
template <bool SSL>
struct TemplatedApp;
using App = TemplatedApp<false>;
struct Loop;
}  // namespace uWS

namespace moodycamel {
template <typename T, size_t MAX_BLOCK_SIZE>
class ReaderWriterQueue;
}

struct us_listen_socket_t;

namespace dmcp::detail {

struct OwnedScreenshot {
  std::vector<uint8_t> pixels;
  uint32_t             width;
  uint32_t             height;
  uint32_t             stride;
};

using SnapshotQueueType = moodycamel::ReaderWriterQueue<Snapshot*, 512>;
using ScreenshotQueueType = moodycamel::ReaderWriterQueue<OwnedScreenshot, 4>;

class ServerRunner {
 public:
  ServerRunner(SnapshotQueueType* queue, ScreenshotQueueType* screenshot_queue,
               SnapshotPool* pool, ScreenshotState* screenshot,
               bool screenshot_enabled, std::atomic<uint64_t>* connected_clients_counter);
  ~ServerRunner();

  bool start(uint16_t port);
  void request_stop();
  void join();
  bool is_running() const { return running_.load(std::memory_order_acquire); }

 private:
  void threadMain(uint16_t port);

  SnapshotQueueType*   queue_;
  ScreenshotQueueType* screenshot_queue_;
  SnapshotPool*        pool_;
  ScreenshotState*     screenshot_;
  const bool           screenshot_enabled_;
  std::atomic<uint64_t>* connected_clients_counter_;

  std::atomic<bool> running_{false};
  std::atomic<bool> stopping_{false};

  std::thread                      thread_;
  std::unique_ptr<uWS::App>        app_;
  std::atomic<uWS::App*>           app_instance_{nullptr};
  std::atomic<us_listen_socket_t*> listen_socket_{nullptr};
  std::atomic<uWS::Loop*>          loop_{nullptr};

  std::condition_variable start_cv_;
  std::mutex              start_mutex_;
};

}  // namespace dmcp::detail
