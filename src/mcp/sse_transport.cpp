#include "mcp/generic/transport.h"

#include <App.h>
#include <Loop.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ============================================================================
// SSE Transport Implementation using uWebSockets
// ============================================================================

namespace mcp {
namespace transport {

// Forward declaration
struct TransportContext;

// Client connection
struct Client {
  uWS::HttpResponse<false>* response;
  std::string id;
  std::chrono::steady_clock::time_point connected_at;
};

// Transport state
struct TransportContext {
  uint16_t port;
  mcp_transport_callbacks_t callbacks;
  void* user_data;
  
  // uWebSockets
  std::unique_ptr<uWS::App> app;
  us_listen_socket_t* listen_socket;
  uWS::Loop* loop;
  
  // Clients
  std::vector<Client*> clients;
  std::mutex clients_mutex;
  std::atomic<uint64_t> client_counter{0};
  
  // Threading
  std::thread thread;
  std::atomic<bool> running{false};
  std::atomic<bool> stopping{false};
  std::condition_variable start_cv;
  std::mutex start_mutex;
};

// Thread-local pointer for thread-safe access
thread_local TransportContext* g_current_ctx = nullptr;

// ============================================================================
// HTTP Handlers
// ============================================================================

static void HandlePostMCP(TransportContext* ctx, uWS::HttpResponse<false>* res, 
                          uWS::HttpRequest* req) {
  res->onAborted([]() {});
  
  res->onData([ctx, res, body = std::string()](
              std::string_view chunk, bool last) mutable {
    // Size limit check
    if (body.size() + chunk.size() > 1024 * 1024) {
      res->writeStatus("413 Payload Too Large");
      res->end("Payload too large");
      return;
    }
    
    body.append(chunk.data(), chunk.size());
    if (!last) return;
    
    // Process request
    char response[8192];
    int http_status = 200;
    
    if (ctx->callbacks.on_http_request) {
      bool handled = ctx->callbacks.on_http_request(
        ctx->user_data,
        "POST",
        "/mcp",
        body.c_str(),
        response,
        sizeof(response),
        &http_status
      );
      
      if (handled) {
        res->writeStatus(std::to_string(http_status));
        if (response[0] != '\0') {
          res->writeHeader("Content-Type", "application/json");
          res->end(response);
        } else {
          res->end();
        }
        return;
      }
    }
    
    res->writeStatus("404 Not Found");
    res->end("Not found");
  });
}

static void HandleGetSSE(TransportContext* ctx, uWS::HttpResponse<false>* res,
                         uWS::HttpRequest* req) {
  // Setup SSE headers
  res->cork([res]() {
    res->writeHeader("Content-Type", "text/event-stream");
    res->writeHeader("Cache-Control", "no-cache");
    res->writeHeader("Connection", "keep-alive");
    res->writeHeader("Access-Control-Allow-Origin", "*");
  });
  
  // Create client
  auto* client = new Client();
  client->response = res;
  client->id = "client_" + std::to_string(ctx->client_counter++);
  client->connected_at = std::chrono::steady_clock::now();
  
  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    ctx->clients.push_back(client);
  }
  
  // Send initial connection event
  std::string connect_msg = "event: connected\ndata: {\"client_id\":\"" 
                            + client->id + "\"}\n\n";
  res->write(connect_msg);
  
  // Handle disconnect
  res->onAborted([ctx, client]() {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    auto it = std::find_if(ctx->clients.begin(), ctx->clients.end(),
                           [client](Client* c) { return c == client; });
    if (it != ctx->clients.end()) {
      ctx->clients.erase(it);
    }
    delete client;
  });
}

static void HandleGetScreenshot(TransportContext* ctx, uWS::HttpResponse<false>* res,
                                uWS::HttpRequest* req) {
  // TODO: Implement screenshot endpoint
  res->writeStatus("501 Not Implemented");
  res->end("Screenshot endpoint not implemented in generic transport");
}

// ============================================================================
// Thread Main
// ============================================================================

static void TransportThreadMain(TransportContext* ctx) {
  g_current_ctx = ctx;
  
  ctx->app = std::make_unique<uWS::App>();
  ctx->loop = uWS::Loop::get();
  
  // Setup routes
  ctx->app->post("/mcp", [ctx](auto* res, auto* req) {
    HandlePostMCP(ctx, res, req);
  });
  
  ctx->app->get("/sse", [ctx](auto* res, auto* req) {
    HandleGetSSE(ctx, res, req);
  });
  
  ctx->app->get("/screenshot/latest.png", [ctx](auto* res, auto* req) {
    HandleGetScreenshot(ctx, res, req);
  });
  
  ctx->app->get("/health", [ctx](auto* res, auto* req) {
    char response[256];
    int status = 200;
    if (ctx->callbacks.on_http_request) {
      ctx->callbacks.on_http_request(ctx->user_data, "GET", "/health", nullptr,
                                     response, sizeof(response), &status);
    } else {
      std::strcpy(response, "{\"status\":\"ok\"}");
    }
    res->writeHeader("Content-Type", "application/json");
    res->writeStatus(std::to_string(status));
    res->end(response);
  });
  
  // Start listening
  ctx->app->listen(ctx->port, [ctx](us_listen_socket_t* token) {
    if (token) {
      ctx->listen_socket = token;
      ctx->running = true;
    } else {
      ctx->stopping = true;
    }
    ctx->start_cv.notify_one();
  });
  
  if (ctx->stopping) {
    return;
  }
  
  // Run event loop
  ctx->app->run();
  
  // Cleanup
  ctx->listen_socket = nullptr;
  ctx->loop = nullptr;
  ctx->app.reset();
  ctx->running = false;
}

}  // namespace transport
}  // namespace mcp

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

static mcp_transport_t* SSE_Create(uint16_t port,
                                   const mcp_transport_callbacks_t* callbacks,
                                   void* user_data) {
  auto* ctx = new mcp::transport::TransportContext();
  ctx->port = port;
  if (callbacks) {
    ctx->callbacks = *callbacks;
  }
  ctx->user_data = user_data;
  ctx->listen_socket = nullptr;
  ctx->loop = nullptr;
  
  return reinterpret_cast<mcp_transport_t*>(ctx);
}

static void SSE_Destroy(mcp_transport_t* transport) {
  if (!transport) return;
  auto* ctx = reinterpret_cast<mcp::transport::TransportContext*>(transport);
  
  // Clean up clients
  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    for (auto* client : ctx->clients) {
      delete client;
    }
    ctx->clients.clear();
  }
  
  delete ctx;
}

static bool SSE_Start(mcp_transport_t* transport) {
  if (!transport) return false;
  auto* ctx = reinterpret_cast<mcp::transport::TransportContext*>(transport);
  
  if (ctx->running) return false;
  
  std::unique_lock<std::mutex> lock(ctx->start_mutex);
  ctx->thread = std::thread(mcp::transport::TransportThreadMain, ctx);
  
  // Wait for startup with timeout
  if (!ctx->start_cv.wait_for(lock, std::chrono::seconds(2), 
                              [ctx] { return ctx->running || ctx->stopping; })) {
    ctx->thread.join();
    return false;
  }
  
  return ctx->running;
}

static void SSE_Stop(mcp_transport_t* transport) {
  if (!transport) return;
  auto* ctx = reinterpret_cast<mcp::transport::TransportContext*>(transport);
  
  if (!ctx->running || ctx->stopping.exchange(true)) return;
  
  if (ctx->loop && ctx->app) {
    ctx->loop->defer([ctx]() {
      if (ctx->listen_socket) {
        us_listen_socket_close(0, ctx->listen_socket);
        ctx->listen_socket = nullptr;
      }
      ctx->app->close();
    });
  }
  
  if (ctx->thread.joinable()) {
    ctx->thread.join();
  }
}

static bool SSE_IsRunning(const mcp_transport_t* transport) {
  if (!transport) return false;
  auto* ctx = reinterpret_cast<const mcp::transport::TransportContext*>(transport);
  return ctx->running.load();
}

static void SSE_Broadcast(mcp_transport_t* transport, const char* data, size_t len) {
  if (!transport || !data) return;
  auto* ctx = reinterpret_cast<mcp::transport::TransportContext*>(transport);
  
  std::lock_guard<std::mutex> lock(ctx->clients_mutex);
  std::vector<mcp::transport::Client*> dead_clients;
  
  for (auto* client : ctx->clients) {
    std::string_view sv(data, len);
    if (!client->response->write(sv)) {
      dead_clients.push_back(client);
    }
  }
  
  // Clean up dead clients
  for (auto* dead : dead_clients) {
    auto it = std::find(ctx->clients.begin(), ctx->clients.end(), dead);
    if (it != ctx->clients.end()) {
      ctx->clients.erase(it);
    }
    delete dead;
  }
}

static size_t SSE_GetClientCount(const mcp_transport_t* transport) {
  if (!transport) return 0;
  auto* ctx = reinterpret_cast<mcp::transport::TransportContext*>(
    const_cast<mcp_transport_t*>(transport));
  std::lock_guard<std::mutex> lock(ctx->clients_mutex);
  return ctx->clients.size();
}

// ============================================================================
// Transport Interface Export
// ============================================================================

const mcp_transport_interface_t mcp_sse_transport = {
  "sse-uws",           // name
  SSE_Create,          // create
  SSE_Destroy,         // destroy
  SSE_Start,           // start
  SSE_Stop,            // stop
  SSE_IsRunning,       // is_running
  SSE_Broadcast,       // broadcast
  SSE_GetClientCount,  // get_client_count
};

}  // extern "C"
