#include <App.h>
#include <Loop.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "mcp/generic/constants.h"
#include "mcp/generic/transport.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

// ============================================================================
// HTTP + SSE Transport Implementation using uWebSockets
// ============================================================================

namespace mcp {
namespace transport {

static constexpr const char* kJsonErrorNotFound =
    "{\"error\":{\"code\":\"not_found\",\"message\":\"Endpoint not found\"}}";
static constexpr const char* kJsonErrorPayloadTooLarge =
    "{\"error\":{\"code\":\"payload_too_large\",\"message\":\"Payload too large\"}}";

static void WriteJsonErrorResponse(uWS::HttpResponse<false>* res, std::string_view status,
                                   std::string_view payload) {
  res->writeStatus(status);
  res->writeHeader("Content-Type", "application/json");
  res->writeHeader("Access-Control-Allow-Origin", "*");
  res->writeHeader("Cache-Control", "no-store");
  res->end(payload);
}

// Forward declaration
struct TransportContext;
static void HandleGetSSE(TransportContext* ctx, uWS::HttpResponse<false>* res,
                         uWS::HttpRequest* req);

// Client connection
struct Client {
  uWS::HttpResponse<false>*             response;
  std::string                           id;
  std::chrono::steady_clock::time_point connected_at;
  std::atomic<bool>                     zombie{false};
  void*                                 handle = nullptr;
};

// Transport state
struct TransportContext {
  uint16_t                  port;
  mcp_transport_callbacks_t callbacks;
  void*                     user_data;

  // uWebSockets
  std::unique_ptr<uWS::App> app;
  us_listen_socket_t*       listen_socket;
  uWS::Loop*                loop;

  // Clients
  std::vector<Client*>  clients;
  std::mutex            clients_mutex;
  std::atomic<uint64_t> client_counter{0};

  // Threading
  std::thread             thread;
  std::atomic<bool>       running{false};
  std::atomic<bool>       stopping{false};
  std::condition_variable start_cv;
  std::mutex              start_mutex;
};

// ============================================================================
// HTTP Handlers
// ============================================================================

static std::string NormalizeHttpMethod(std::string_view method) {
  std::string normalized(method);
  for (char& c : normalized) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return normalized;
}

static bool MethodCanHaveBody(std::string_view method) {
  return method == "POST" || method == "PUT" || method == "PATCH";
}

static void WriteOptionsResponse(uWS::HttpResponse<false>* res) {
  res->writeStatus("204");
  res->writeHeader("Access-Control-Allow-Origin", "*");
  res->writeHeader("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, OPTIONS");
  res->writeHeader("Access-Control-Allow-Headers", "Content-Type");
  res->end();
}

static void WriteHandledResponse(uWS::HttpResponse<false>* res, int http_status, const char* body) {
  res->writeStatus(std::to_string(http_status));
  res->writeHeader("Access-Control-Allow-Origin", "*");
  res->writeHeader("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, OPTIONS");
  res->writeHeader("Access-Control-Allow-Headers", "Content-Type");

  if (!body || body[0] == '\0' || http_status == 204) {
    res->end();
    return;
  }

  res->writeHeader("Content-Type", "application/json");
  res->end(body);
}

static void DispatchHttpRequest(TransportContext* ctx, uWS::HttpResponse<false>* res,
                                std::string_view method, std::string_view path,
                                const char* body_or_null) {
  if (!ctx->callbacks.on_http_request) {
    WriteJsonErrorResponse(res, "404", kJsonErrorNotFound);
    return;
  }

  char response[MCP_BUFFER_SIZE_DEFAULT] = {};
  int  http_status                       = 200;

  std::string method_str(method);
  std::string path_str(path);
  bool        handled =
      ctx->callbacks.on_http_request(ctx->user_data, method_str.c_str(), path_str.c_str(),
                                     body_or_null, response, sizeof(response), &http_status);

  if (!handled) {
    WriteJsonErrorResponse(res, "404", kJsonErrorNotFound);
    return;
  }

  WriteHandledResponse(res, http_status, response);
}

static void HandleHttpRequest(TransportContext* ctx, uWS::HttpResponse<false>* res,
                              uWS::HttpRequest* req) {
  std::string method = NormalizeHttpMethod(req->getMethod());
  std::string path(req->getUrl());

  if (method == "GET" && path == MCP_ENDPOINT_MCP) {
    HandleGetSSE(ctx, res, req);
    return;
  }

  if (method == "OPTIONS") {
    WriteOptionsResponse(res);
    return;
  }

  res->onAborted([]() {});

  if (!MethodCanHaveBody(method)) {
    DispatchHttpRequest(ctx, res, method, path, nullptr);
    return;
  }

  res->onData([ctx, res, method = std::move(method), path = std::move(path), body = std::string()](
                  std::string_view chunk, bool last) mutable {
    if (body.size() + chunk.size() > MCP_MAX_PAYLOAD_SIZE) {
      WriteJsonErrorResponse(res, "413", kJsonErrorPayloadTooLarge);
      return;
    }

    body.append(chunk.data(), chunk.size());
    if (!last) {
      return;
    }

    DispatchHttpRequest(ctx, res, method, path, body.c_str());
  });
}

static void HandleGetSSE(TransportContext* ctx, uWS::HttpResponse<false>* res,
                         uWS::HttpRequest* req) {
  (void)req;
  // Setup SSE headers
  res->cork([res]() {
    res->writeHeader("Content-Type", "text/event-stream");
    res->writeHeader("Cache-Control", "no-cache");
    res->writeHeader("Connection", "keep-alive");
    res->writeHeader("Access-Control-Allow-Origin", "*");
  });

  // Create client
  auto* client         = new Client();
  client->response     = res;
  client->id           = "client_" + std::to_string(ctx->client_counter++);
  client->connected_at = std::chrono::steady_clock::now();

  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    ctx->clients.push_back(client);
  }

  if (ctx->callbacks.on_sse_connect) {
    void* client_handle = client;
    ctx->callbacks.on_sse_connect(ctx->user_data, &client_handle);
    client->handle = client_handle;
  }

  // Send initial connection event
  std::string connect_msg = "event: connected\ndata: {\"client_id\":\"" + client->id + "\"}\n\n";
  res->write(connect_msg);

  // Handle disconnect
  res->onAborted([ctx, client]() {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    if (client->zombie.load(std::memory_order_acquire)) return;

    bool expected = false;
    if (!client->zombie.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
      return;
    }

    if (ctx->callbacks.on_sse_disconnect && client->handle) {
      ctx->callbacks.on_sse_disconnect(ctx->user_data, client->handle);
    }
    auto it = std::find_if(ctx->clients.begin(), ctx->clients.end(),
                           [client](Client* c) { return c == client; });
    if (it != ctx->clients.end()) {
      ctx->clients.erase(it);
    }
    delete client;
  });
}

// ============================================================================
// Thread Main
// ============================================================================

static void TransportThreadMain(TransportContext* ctx) {
  ctx->app  = std::make_unique<uWS::App>();
  ctx->loop = uWS::Loop::get();
  ctx->loop->setSilent(true);

  // Setup routes
  ctx->app->get(MCP_ENDPOINT_MCP, [ctx](auto* res, auto* req) { HandleGetSSE(ctx, res, req); });
  ctx->app->any("/*", [ctx](auto* res, auto* req) { HandleHttpRequest(ctx, res, req); });

  // Start listening
  ctx->app->listen(ctx->port, [ctx](us_listen_socket_t* token) {
    if (token) {
      ctx->listen_socket = token;
      ctx->running       = true;
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
  ctx->loop          = nullptr;
  ctx->app.reset();
  ctx->running = false;
}

}  // namespace transport
}  // namespace mcp

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

static mcp_transport_t* SSE_Create(uint16_t port, const mcp_transport_callbacks_t* callbacks,
                                   void* user_data) {
  auto* ctx = new mcp::transport::TransportContext();
  ctx->port = port;
  if (callbacks) {
    ctx->callbacks = *callbacks;
  }
  ctx->user_data     = user_data;
  ctx->listen_socket = nullptr;
  ctx->loop          = nullptr;

  return reinterpret_cast<mcp_transport_t*>(ctx);
}

static void SSE_Destroy(mcp_transport_t* transport) {
  if (!transport) return;
  auto* ctx = reinterpret_cast<mcp::transport::TransportContext*>(transport);

  // Clean up clients
  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    for (auto* client : ctx->clients) {
      if (ctx->callbacks.on_sse_disconnect && client->handle) {
        ctx->callbacks.on_sse_disconnect(ctx->user_data, client->handle);
      }
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
  if (!ctx->start_cv.wait_for(lock, std::chrono::seconds(MCP_STARTUP_TIMEOUT_SECONDS),
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

  for (auto* client : ctx->clients) {
    std::string_view sv(data, len);
    if (client->response->write(sv)) {
      if (ctx->callbacks.on_sse_send) {
        ctx->callbacks.on_sse_send(ctx->user_data, client->handle, sv.data(), sv.length());
      }
    } else {
      client->zombie.store(true, std::memory_order_release);
    }
  }

  ctx->clients.erase(std::remove_if(ctx->clients.begin(), ctx->clients.end(),
                                    [ctx](mcp::transport::Client* c) {
                                      if (!c->zombie.load(std::memory_order_acquire)) {
                                        return false;
                                      }

                                      if (ctx->callbacks.on_sse_disconnect && c->handle) {
                                        ctx->callbacks.on_sse_disconnect(ctx->user_data, c->handle);
                                      }
                                      delete c;
                                      return true;
                                    }),
                     ctx->clients.end());
}

static size_t SSE_GetClientCount(const mcp_transport_t* transport) {
  if (!transport) return 0;
  auto* ctx =
      reinterpret_cast<mcp::transport::TransportContext*>(const_cast<mcp_transport_t*>(transport));
  std::lock_guard<std::mutex> lock(ctx->clients_mutex);
  return ctx->clients.size();
}

// ============================================================================
// Transport Interface Export
// ============================================================================

const mcp_transport_interface_t mcp_sse_transport = {
    1,                   // version
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

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
