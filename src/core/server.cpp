#include "dmcp/core/server.hpp"

#include <readerwriterqueue.h>

#include <libusockets.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "App.h"
#include "Loop.h"
#include "dmcp/common.hpp"

namespace dmcp::detail {
namespace {

using json = nlohmann::json;

std::string_view trim_cstr(const char* data, size_t max_len) {
  size_t len = 0;
  while (len < max_len && data[len] != '\0') {
    ++len;
  }
  return std::string_view(data, len);
}

template <size_t N>
std::string_view trim_array(const std::array<char, N>& arr) {
  return trim_cstr(arr.data(), N);
}

std::string to_iso8601(std::chrono::system_clock::time_point tp) {
  if (tp.time_since_epoch().count() == 0) {
    return "";
  }
  std::time_t t = std::chrono::system_clock::to_time_t(tp);
  std::tm tm = *std::gmtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

json snapshot_to_json(const Snapshot& snapshot) {
  json payload;

  json player_json = {
      {"health", snapshot.player.health},
      {"armor", snapshot.player.armor},
      {"ammo", snapshot.player.ammo},
      {"position",
       {{"x", snapshot.player.position.x}, {"y", snapshot.player.position.y}}}};

  json inventory = json::array();
  for (const auto& item : snapshot.player.inventory) {
    inventory.push_back({{"name", trim_array(item.name)},
                         {"amount", item.amount}});
  }
  player_json["inventory"] = std::move(inventory);
  payload["player"] = std::move(player_json);

  json level_json = {
      {"tic", snapshot.level.tic},
      {"name", trim_array(snapshot.level.name)},
      {"kill_count", snapshot.level.kill_count},
      {"item_count", snapshot.level.item_count},
      {"secret_count", snapshot.level.secret_count},
  };
  payload["level"] = std::move(level_json);

  json enemies = json::array();
  for (const auto& enemy : snapshot.enemies) {
    enemies.push_back({{"id", enemy.id},
                       {"health", enemy.health},
                       {"position",
                        {{"x", enemy.position.x}, {"y", enemy.position.y}}},
                       {"type", trim_array(enemy.type)}});
  }
  payload["enemies"] = std::move(enemies);

  return payload;
}

std::string make_sse_event(std::string_view event_name, const json& payload) {
  std::string msg;
  msg.reserve(event_name.size() + payload.dump().size() + 16);
  msg.append("event: ");
  msg.append(event_name);
  msg.append("\ndata: ");
  msg.append(payload.dump());
  msg.append("\n\n");
  return msg;
}

void respond_json(uWS::HttpResponse<false>* res, const json& payload) {
  auto data = payload.dump();
  res->writeHeader("Content-Type", "application/json");
  res->end(data);
}

}  // namespace

ServerRunner::ServerRunner(moodycamel::ReaderWriterQueue<Snapshot*>* queue,
                           SnapshotPool* pool, ScreenshotState* screenshot,
                           bool screenshot_enabled)
    : queue_(queue),
      pool_(pool),
      screenshot_(screenshot),
      screenshot_enabled_(screenshot_enabled) {}

ServerRunner::~ServerRunner() {
  request_stop();
  join();
}

bool ServerRunner::start(uint16_t port) {
  if (running_.load(std::memory_order_acquire)) {
    return false;
  }
  stopping_.store(false);
  thread_ = std::thread([this, port]() { threadMain(port); });

  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (!running_.load(std::memory_order_acquire) &&
         !stopping_.load(std::memory_order_acquire) &&
         std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return running_.load(std::memory_order_acquire);
}

void ServerRunner::request_stop() {
  if (!running_.load(std::memory_order_acquire)) {
    return;
  }
  if (stopping_.exchange(true, std::memory_order_acq_rel)) {
    return;
  }
  auto loop = loop_;
  auto app = app_instance_.load(std::memory_order_acquire);
  if (loop && app) {
    loop->defer([this]() {
      if (auto* socket =
              listen_socket_.exchange(nullptr, std::memory_order_acq_rel)) {
        us_listen_socket_close(0, socket);
      }
      if (auto* app_ptr = app_instance_.load(std::memory_order_acquire)) {
        app_ptr->close();
      }
    });
  }
}

void ServerRunner::join() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

void ServerRunner::threadMain(uint16_t port) {
  struct ServerContext {
    moodycamel::ReaderWriterQueue<Snapshot*>* queue;
    SnapshotPool* pool;
    ScreenshotState* screenshot;
    std::vector<uWS::HttpResponse<false>*> subs;
    uint64_t last_screenshot_version = 0;
    bool screenshot_enabled = false;
  } ctx{queue_, pool_, screenshot_, {}, 0, screenshot_enabled_};

  app_ = std::make_unique<uWS::App>();
  loop_ = uWS::Loop::get();
  app_instance_.store(app_.get(), std::memory_order_release);

  loop_->addPostHandler(this, [&ctx](uWS::Loop* /*loop*/) {
    Snapshot* snapshot = nullptr;
    while (ctx.queue->try_dequeue(snapshot)) {
      auto payload = snapshot_to_json(*snapshot);
      std::string msg = make_sse_event("state", payload);
      std::vector<uWS::HttpResponse<false>*> closed;
      for (auto* sub : ctx.subs) {
        if (!sub->write(msg)) {
          closed.push_back(sub);
        }
      }
      if (!closed.empty()) {
        for (auto* dead : closed) {
          ctx.subs.erase(std::remove(ctx.subs.begin(), ctx.subs.end(), dead),
                         ctx.subs.end());
        }
      }
      ctx.pool->Release(snapshot);
    }

    if (ctx.screenshot_enabled) {
      auto version = ctx.screenshot->version.load(std::memory_order_acquire);
      if (version != ctx.last_screenshot_version) {
        ctx.last_screenshot_version = version;
        json payload = {
            {"uri", "/screenshot/latest.png"},
            {"width", ctx.screenshot->width},
            {"height", ctx.screenshot->height},
            {"captured_at", to_iso8601(ctx.screenshot->captured_at)},
        };
        std::string msg = make_sse_event("screenshot", payload);
        for (auto* sub : ctx.subs) {
          sub->write(msg);
        }
      }
    }
  });

  auto& app = *app_;

  app.post("/mcp",
           [&ctx](auto* res, auto* /*req*/) {
             json capabilities = {
                 {"jsonrpc", "2.0"},
                 {"result", {{"capabilities", {{"notifications", true}}}}}};
             if (ctx.screenshot_enabled) {
               capabilities["result"]["capabilities"]["screenshot"] = "png";
             }
             respond_json(res, capabilities);
           })
      .post("/tools/call",
            [&ctx](auto* res, auto* /*req*/) {
              res->onAborted([]() {});
              res->onData([res, &ctx, payload = std::string()](
                              std::string_view chunk, bool last) mutable {
                payload.append(chunk.data(), chunk.size());
                if (!last) {
                  return;
                }
                auto doc = json::parse(payload, nullptr, false);
                if (doc.is_discarded()) {
                  res->writeStatus("400 Bad Request");
                  res->end("invalid json");
                  return;
                }
                const auto name = doc.value("params", json::object())
                                      .value("name", std::string());
                if (name == "capture_screenshot" && ctx.screenshot_enabled) {
                  ctx.screenshot->pending_requests.fetch_add(
                      1, std::memory_order_relaxed);
                  json reply = {{"jsonrpc", "2.0"},
                                {"result", {{"status", "queued"}}}};
                  if (doc.contains("id")) {
                    reply["id"] = doc["id"];
                  }
                  respond_json(res, reply);
                } else if (name == "capture_screenshot") {
                  res->writeStatus("400 Bad Request");
                  res->end("screenshot disabled");
                } else {
                  res->writeStatus("404 Not Found");
                  res->end("unknown tool");
                }
              });
            })
      .get("/sse",
           [&ctx](auto* res, auto* /*req*/) {
             res->cork([res]() {
               res->writeHeader("Content-Type", "text/event-stream");
               res->writeHeader("Cache-Control", "no-cache");
               res->writeHeader("Connection", "keep-alive");
             });
             ctx.subs.push_back(res);
             res->onAborted([&ctx, res]() {
               ctx.subs.erase(
                   std::remove(ctx.subs.begin(), ctx.subs.end(), res),
                   ctx.subs.end());
             });
           })
      .get("/screenshot/latest.png", [&ctx](auto* res, auto* /*req*/) {
        if (!ctx.screenshot_enabled) {
          res->writeStatus("404 Not Found");
          res->end("screenshot disabled");
          return;
        }
        std::vector<uint8_t> copy;
        uint32_t width = 0;
        uint32_t height = 0;
        {
          std::scoped_lock lock(ctx.screenshot->data_mutex);
          copy = ctx.screenshot->latest_png;
          width = ctx.screenshot->width;
          height = ctx.screenshot->height;
        }
        if (copy.empty()) {
          res->writeStatus("404 Not Found");
          res->end("no screenshot");
          return;
        }
        res->writeHeader("Content-Type", "image/png");
        res->writeHeader("X-Width", std::to_string(width));
        res->writeHeader("X-Height", std::to_string(height));
        res->end(std::string_view(reinterpret_cast<const char*>(copy.data()),
                                  copy.size()));
      });

  app.listen(port, [this](auto* token) {
    if (token) {
      listen_socket_.store(token, std::memory_order_release);
      running_.store(true, std::memory_order_release);
    } else {
      stopping_.store(true, std::memory_order_release);
    }
  });

  app.run();
  if (loop_) {
    loop_->removePostHandler(this);
    loop_ = nullptr;
  }
  listen_socket_.store(nullptr, std::memory_order_release);
  app_instance_.store(nullptr, std::memory_order_release);
  app_.reset();
  running_.store(false, std::memory_order_release);
}

}  // namespace dmcp::detail
