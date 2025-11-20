#include "server.hpp"

#include <readerwriterqueue.h>

// Include uWebSockets implementation headers here, not in the header file
#include <App.h>
#include <Loop.h>

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

#include "core/png.hpp"
#include "dmcp/common.hpp"

namespace dmcp::detail {

using json = nlohmann::json;

namespace {

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
  std::time_t        t  = std::chrono::system_clock::to_time_t(tp);
  std::tm            tm = *std::gmtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

json snapshot_to_json(const Snapshot& snapshot) {
  json payload;

  json player_json = {
      {      "hp",		       snapshot.player.hp                  },
      {   "armor",				      snapshot.player.armor},
      {    "ammo",				       snapshot.player.ammo},
      {"position",
       {{"x", snapshot.player.position.x}, {"y", snapshot.player.position.y}}}
  };

  json inventory = json::array();
  for (const auto& item : snapshot.player.inventory) {
    inventory.push_back({
	{  "name", trim_array(item.name)},
        {"amount",           item.amount}
    });
  }
  player_json["inventory"] = std::move(inventory);
  payload["player"]        = std::move(player_json);

  json level_json = {
      {	 "tic",	      snapshot.level.tic},
      {	"name", trim_array(snapshot.level.name)},
      {  "kill_count",       snapshot.level.kill_count},
      {  "item_count",       snapshot.level.item_count},
      {"secret_count",     snapshot.level.secret_count},
  };
  payload["level"] = std::move(level_json);

  json enemies = json::array();
  for (const auto& enemy : snapshot.enemies) {
    enemies.push_back({
	{      "id",					   enemy.id},
	{      "hp",					   enemy.hp},
	{  "max_hp",				       enemy.max_hp},
	{"position", {{"x", enemy.position.x}, {"y", enemy.position.y}}},
	{    "type",			     trim_array(enemy.type)}
    });
  }
  payload["enemies"] = std::move(enemies);

  return payload;
}

std::string make_sse_event(std::string_view event_name, const json& payload) {
  std::string msg;
  // Estimate size with indentation
  msg.reserve(event_name.size() + payload.dump(2).size() + 16);
  msg.append("event: ");
  msg.append(event_name);
  msg.append("\ndata: ");
  msg.append(payload.dump(2));
  msg.append("\n\n");
  return msg;
}

void respond_json(uWS::HttpResponse<false>* res, const json& payload) {
  auto data = payload.dump(2);
  res->writeHeader("Content-Type", "application/json");
  res->end(data);
}

}  // namespace

ServerRunner::ServerRunner(SnapshotQueueType*   queue,
                           ScreenshotQueueType* screenshot_queue,
                           SnapshotPool* pool, ScreenshotState* screenshot,
                           bool                   screenshot_enabled,
                           std::atomic<uint64_t>* connected_clients_counter)
    : queue_(queue),
      screenshot_queue_(screenshot_queue),
      pool_(pool),
      screenshot_(screenshot),
      screenshot_enabled_(screenshot_enabled),
      connected_clients_counter_(connected_clients_counter) {}

ServerRunner::~ServerRunner() {
  request_stop();
  join();
}

bool ServerRunner::start(uint16_t port) {
  if (running_.load(std::memory_order_acquire)) {
    return false;
  }
  stopping_.store(false);

  std::unique_lock<std::mutex> lock(start_mutex_);
  thread_ = std::thread([this, port]() { threadMain(port); });

  start_cv_.wait_for(lock, std::chrono::seconds(2), [this] {
    return running_.load(std::memory_order_acquire) ||
           stopping_.load(std::memory_order_acquire);
  });

  // If we timed out and running is still false, it means listen failed or
  // thread didn't start. However, if listen failed, threadMain sets stopping_
  // to true.
  if (!running_.load(std::memory_order_acquire)) {
    // Ensure we join the thread if it was started but failed
    if (thread_.joinable()) {
      thread_.join();
    }
    return false;
  }

  return true;
}

void ServerRunner::request_stop() {
  // We must check running_ OR stopping_ to ensure we can clean up a failed
  // start But typically if start failed, the thread is already joined.
  if (!running_.load(std::memory_order_acquire)) {
    return;
  }
  if (stopping_.exchange(true, std::memory_order_acq_rel)) {
    return;
  }
  auto loop = loop_.load(std::memory_order_acquire);
  auto app  = app_instance_.load(std::memory_order_acquire);
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
    SnapshotQueueType*                     queue;
    ScreenshotQueueType*                   screenshot_queue;
    SnapshotPool*                          pool;
    ScreenshotState*                       screenshot;
    std::vector<uWS::HttpResponse<false>*> subs;
    uint64_t                               last_screenshot_version = 0;
    bool                                   screenshot_enabled      = false;
  } ctx{queue_, screenshot_queue_,  pool_, screenshot_, {},
        0,      screenshot_enabled_};

  app_       = std::make_unique<uWS::App>();
  auto* loop = uWS::Loop::get();
  loop_.store(loop, std::memory_order_release);
  app_instance_.store(app_.get(), std::memory_order_release);

  loop->addPostHandler(this, [&ctx, this](uWS::Loop* /*loop*/) {
    Snapshot* snapshot = nullptr;
    while (ctx.queue->try_dequeue(snapshot)) {
      auto        payload = snapshot_to_json(*snapshot);
      std::string msg     = make_sse_event("state", payload);
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
	  if (connected_clients_counter_) {
	    connected_clients_counter_->fetch_sub(1, std::memory_order_relaxed);
	  }
	}
      }
      ctx.pool->Release(snapshot);
    }

    if (ctx.screenshot_enabled) {
      // Process screenshot queue
      OwnedScreenshot frame;
      if (ctx.screenshot_queue->try_dequeue(frame)) {
	dmcp::ScreenshotFrame view{frame.pixels.data(), frame.width,
	                           frame.height, frame.stride};
	auto                  png = dmcp::detail::EncodePng(view);
	if (png) {
	  std::scoped_lock lock(ctx.screenshot->data_mutex);
	  ctx.screenshot->latest_png  = std::move(*png);
	  ctx.screenshot->width       = frame.width;
	  ctx.screenshot->height      = frame.height;
	  ctx.screenshot->captured_at = std::chrono::system_clock::now();
	  ctx.screenshot->version.fetch_add(1, std::memory_order_release);
	}
      }

      auto version = ctx.screenshot->version.load(std::memory_order_acquire);
      if (version != ctx.last_screenshot_version) {
	ctx.last_screenshot_version = version;

	// Lock to read metadata safely
	uint32_t    width  = 0;
	uint32_t    height = 0;
	std::string captured_at;
	{
	  std::scoped_lock lock(ctx.screenshot->data_mutex);
	  width       = ctx.screenshot->width;
	  height      = ctx.screenshot->height;
	  captured_at = to_iso8601(ctx.screenshot->captured_at);
	}

	json payload = {
	    {        "uri", "/screenshot/latest.png"},
	    {      "width",                    width},
	    {     "height",                   height},
	    {"captured_at",              captured_at},
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
		 {"jsonrpc",					 "2.0"},
		 { "result", {{"capabilities", {{"notifications", true}}}}}
             };
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
		// DoS Protection: Limit payload size (e.g., 1MB)
		if (payload.size() + chunk.size() > 1024 * 1024) {
		  res->writeStatus("413 Payload Too Large");
		  res->end("payload too large");
		  return;
		}

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
		  json reply = {
		      {"jsonrpc",                  "2.0"},
                      { "result", {{"status", "queued"}}}
                  };
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
           [&ctx, this](auto* res, auto* /*req*/) {
	     res->cork([res]() {
	       res->writeHeader("Content-Type", "text/event-stream");
	       res->writeHeader("Cache-Control", "no-cache");
	       res->writeHeader("Connection", "keep-alive");
	     });
	     ctx.subs.push_back(res);
	     if (connected_clients_counter_) {
	       connected_clients_counter_->fetch_add(1,
                                                     std::memory_order_relaxed);
	     }
	     res->onAborted([&ctx, res, this]() {
	       ctx.subs.erase(
		   std::remove(ctx.subs.begin(), ctx.subs.end(), res),
		   ctx.subs.end());
	       if (connected_clients_counter_) {
		 connected_clients_counter_->fetch_sub(
		     1, std::memory_order_relaxed);
	       }
	     });
	   })
      .get("/screenshot/latest.png", [&ctx](auto* res, auto* /*req*/) {
	if (!ctx.screenshot_enabled) {
	  res->writeStatus("404 Not Found");
	  res->end("screenshot disabled");
	  return;
	}
	std::vector<uint8_t> copy;
	uint32_t             width  = 0;
	uint32_t             height = 0;
	{
	  std::scoped_lock lock(ctx.screenshot->data_mutex);
	  copy   = ctx.screenshot->latest_png;
	  width  = ctx.screenshot->width;
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
    start_cv_.notify_one();
  });

  // If listen failed, we shouldn't run the app loop as it might block or be
  // useless. However, uWS::App::run() is what drives the loop. If listen
  // failed, stopping_ is true.
  if (stopping_.load(std::memory_order_acquire)) {
    // Cleanup immediately
    auto* loop_ptr = loop_.load(std::memory_order_relaxed);
    if (loop_ptr) {
      loop_ptr->removePostHandler(this);
      loop_.store(nullptr, std::memory_order_release);
    }
    app_instance_.store(nullptr, std::memory_order_release);
    app_.reset();
    return;
  }

  app.run();

  auto* loop_ptr = loop_.load(std::memory_order_relaxed);
  if (loop_ptr) {
    loop_ptr->removePostHandler(this);
    loop_.store(nullptr, std::memory_order_release);
  }
  listen_socket_.store(nullptr, std::memory_order_release);
  app_instance_.store(nullptr, std::memory_order_release);
  app_.reset();
  running_.store(false, std::memory_order_release);
}

}  // namespace dmcp::detail
