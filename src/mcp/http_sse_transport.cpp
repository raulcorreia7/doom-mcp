#include <httplib.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "mcp/generic/constants.h"
#include "mcp/generic/protocol.h"
#include "mcp/generic/transport.h"
#include "mcp/json_rpc.hpp"
#include "mcp/request_context.hpp"

// ============================================================================
// HTTP + SSE Transport Implementation using cpp-httplib
// ============================================================================

namespace mcp {
namespace transport {

static constexpr const char* kJsonErrorNotFound =
    "{\"error\":{\"code\":\"not_found\",\"message\":\"Endpoint not found\"}}";
static constexpr const char* kJsonErrorPayloadTooLarge =
    "{\"error\":{\"code\":\"payload_too_large\",\"message\":\"Payload too large\"}}";
static constexpr const char* kJsonErrorNotAcceptableSSE =
    "{\"error\":{\"code\":\"not_acceptable\",\"message\":\"Accept header must include "
    "text/event-stream\"}}";
static constexpr const char* kJsonErrorNotAcceptableJson =
    "{\"error\":{\"code\":\"not_acceptable\",\"message\":\"Accept header must include "
    "application/json\"}}";
static constexpr const char* kJsonErrorUnsupportedMediaType =
    "{\"error\":{\"code\":\"unsupported_media_type\",\"message\":\"Content-Type must be "
    "application/json\"}}";
static constexpr const char* kJsonErrorForbiddenOrigin =
    "{\"error\":{\"code\":\"forbidden_origin\",\"message\":\"Origin not allowed\"}}";
static constexpr size_t kMaxPendingEventsPerClient = 64;

struct Client {
  std::string                           id;
  void*                                 handle = nullptr;
  std::atomic<bool>                     closed{false};
  std::mutex                            mutex;
  std::condition_variable               cv;
  std::deque<std::string>               pending;
  std::chrono::steady_clock::time_point connected_at;
};

struct TransportContext {
  uint16_t                  port;
  mcp_transport_callbacks_t callbacks;
  void*                     user_data;

  std::string allowed_origin;
  size_t      max_payload_size = MCP_MAX_PAYLOAD_SIZE;

  std::unique_ptr<httplib::Server> server;

  std::vector<std::shared_ptr<Client>> clients;
  std::mutex                           clients_mutex;
  std::atomic<uint64_t>                client_counter{0};

  std::thread             thread;
  std::atomic<bool>       running{false};
  std::atomic<bool>       stopping{false};
  bool                    startup_done = false;
  bool                    startup_ok   = false;
  std::condition_variable start_cv;
  std::mutex              start_mutex;
};

static std::string normalize_http_method(std::string_view method) {
  std::string normalized(method);
  for (char& c : normalized) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return normalized;
}

static bool method_can_have_body(std::string_view method) {
  return method == "POST" || method == "PUT" || method == "PATCH";
}

static bool contains_case_insensitive(std::string_view haystack, std::string_view needle) {
  if (needle.empty() || haystack.size() < needle.size()) {
    return false;
  }

  for (size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
    bool matches = true;
    for (size_t j = 0; j < needle.size(); ++j) {
      const char a = static_cast<char>(std::tolower(static_cast<unsigned char>(haystack[i + j])));
      const char b = static_cast<char>(std::tolower(static_cast<unsigned char>(needle[j])));
      if (a != b) {
        matches = false;
        break;
      }
    }
    if (matches) {
      return true;
    }
  }

  return false;
}

static std::string get_header_value(const httplib::Request& req, const char* key) {
  const auto value = req.get_header_value(key);
  return value;
}

static void set_common_headers(httplib::Response& res) {
  res.set_header("Access-Control-Allow-Origin", "*");
  res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
  res.set_header("Access-Control-Allow-Headers",
                 "Content-Type, Accept, MCP-Session-Id, MCP-Protocol-Version");
  res.set_header("Cache-Control", "no-store");
}

static void set_mcp_headers(httplib::Response& res, std::string_view request_session_id,
                            const request_context::response_metadata& meta) {
  const std::string protocol =
      meta.protocol_version.empty() ? std::string(MCP_PROTOCOL_VERSION) : meta.protocol_version;
  res.set_header("MCP-Protocol-Version", protocol);

  if (!meta.session_id.empty()) {
    res.set_header("MCP-Session-Id", meta.session_id);
  } else if (!request_session_id.empty()) {
    res.set_header("MCP-Session-Id", std::string(request_session_id));
  }
}

static void write_json_error_response(httplib::Response& res, int status, std::string_view payload,
                                      bool             add_mcp_headers    = false,
                                      std::string_view request_session_id = {}) {
  res.status = status;
  set_common_headers(res);
  if (add_mcp_headers) {
    request_context::response_metadata meta;
    set_mcp_headers(res, request_session_id, meta);
  }
  res.set_content(std::string(payload), "application/json");
}

static bool is_origin_allowed(const TransportContext* ctx, const httplib::Request& req) {
  if (!ctx || ctx->allowed_origin.empty()) {
    return true;
  }

  const std::string origin = get_header_value(req, "Origin");
  if (origin.empty()) {
    return true;
  }

  return origin == ctx->allowed_origin;
}

static void mark_client_closed(TransportContext* ctx, const std::shared_ptr<Client>& client,
                               bool notify_disconnect) {
  if (!ctx || !client) return;

  bool expected = false;
  if (!client->closed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
    return;
  }

  if (notify_disconnect && ctx->callbacks.on_sse_disconnect && client->handle) {
    ctx->callbacks.on_sse_disconnect(ctx->user_data, client->handle);
  }

  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    ctx->clients.erase(std::remove_if(ctx->clients.begin(), ctx->clients.end(),
                                      [&client](const std::shared_ptr<Client>& current) {
                                        return current.get() == client.get();
                                      }),
                       ctx->clients.end());
  }

  client->cv.notify_all();
}

static void write_options_response(httplib::Response& res) {
  res.status = 204;
  set_common_headers(res);
  res.body.clear();
}

static void write_handled_response(httplib::Response& res, int http_status, const char* body,
                                   bool is_mcp_request, std::string_view request_session_id,
                                   const request_context::response_metadata& meta) {
  res.status = http_status;
  set_common_headers(res);
  if (is_mcp_request) {
    set_mcp_headers(res, request_session_id, meta);
  }

  if (!body || body[0] == '\0' || http_status == 204) {
    res.body.clear();
    return;
  }

  res.set_content(std::string(body), "application/json");
}

static void dispatch_http_request(TransportContext* ctx, const httplib::Request& req,
                                  httplib::Response& res, std::string_view method,
                                  std::string_view path, const char* body_or_null) {
  if (!ctx->callbacks.on_http_request) {
    write_json_error_response(res, 404, kJsonErrorNotFound);
    return;
  }

  request_context::request_metadata req_meta;
  req_meta.session_id       = get_header_value(req, "MCP-Session-Id");
  req_meta.protocol_version = get_header_value(req, "MCP-Protocol-Version");
  req_meta.origin           = get_header_value(req, "Origin");
  req_meta.accept           = get_header_value(req, "Accept");
  req_meta.content_type     = get_header_value(req, "Content-Type");

  request_context::set_current(req_meta);
  request_context::clear_response();

  char response[MCP_BUFFER_SIZE_DEFAULT] = {};
  int  http_status                       = 200;

  std::string method_str(method);
  std::string path_str(path);
  bool        handled =
      ctx->callbacks.on_http_request(ctx->user_data, method_str.c_str(), path_str.c_str(),
                                     body_or_null, response, sizeof(response), &http_status);

  const request_context::response_metadata response_meta = request_context::response();

  request_context::clear_current();
  request_context::clear_response();

  if (!handled) {
    write_json_error_response(res, 404, kJsonErrorNotFound, path == MCP_ENDPOINT_MCP,
                              req_meta.session_id);
    return;
  }

  write_handled_response(res, http_status, response, path == MCP_ENDPOINT_MCP, req_meta.session_id,
                         response_meta);
}

static void handle_get_sse(TransportContext* ctx, const httplib::Request& req,
                           httplib::Response& res) {
  if (!is_origin_allowed(ctx, req)) {
    write_json_error_response(res, 403, kJsonErrorForbiddenOrigin);
    return;
  }

  const std::string accept_header = get_header_value(req, "Accept");
  if (!contains_case_insensitive(accept_header, "text/event-stream")) {
    write_json_error_response(res, 406, kJsonErrorNotAcceptableSSE);
    return;
  }

  auto client          = std::make_shared<Client>();
  client->id           = "client_" + std::to_string(ctx->client_counter.fetch_add(1));
  client->connected_at = std::chrono::steady_clock::now();

  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    ctx->clients.push_back(client);
  }

  if (ctx->callbacks.on_sse_connect) {
    void* client_handle = client.get();
    ctx->callbacks.on_sse_connect(ctx->user_data, &client_handle);
    client->handle = client_handle;
  }

  set_common_headers(res);
  res.set_header("Cache-Control", "no-cache");
  res.set_header("Connection", "keep-alive");
  res.set_header("MCP-Protocol-Version", MCP_PROTOCOL_VERSION);

  res.set_chunked_content_provider(
      "text/event-stream",
      [ctx, client](size_t /*offset*/, httplib::DataSink& sink) {
        std::unique_lock<std::mutex> lock(client->mutex);

        while (client->pending.empty() && !ctx->stopping.load(std::memory_order_acquire) &&
               !client->closed.load(std::memory_order_acquire)) {
          client->cv.wait_for(lock, std::chrono::seconds(1));

          if (!client->pending.empty() || client->closed.load(std::memory_order_acquire) ||
              ctx->stopping.load(std::memory_order_acquire)) {
            break;
          }

          const char* heartbeat = ": keepalive\n\n";
          lock.unlock();
          if (!sink.write(heartbeat, std::strlen(heartbeat))) {
            client->closed.store(true, std::memory_order_release);
            sink.done();
            return false;
          }
          lock.lock();
        }

        if (ctx->stopping.load(std::memory_order_acquire) ||
            client->closed.load(std::memory_order_acquire)) {
          sink.done();
          return true;
        }

        while (!client->pending.empty()) {
          std::string message = std::move(client->pending.front());
          client->pending.pop_front();

          lock.unlock();
          const bool write_ok = sink.write(message.data(), message.size());
          if (write_ok && ctx->callbacks.on_sse_send) {
            ctx->callbacks.on_sse_send(ctx->user_data, client->handle, message.data(),
                                       message.size());
          }
          lock.lock();

          if (!write_ok) {
            client->closed.store(true, std::memory_order_release);
            sink.done();
            return false;
          }
        }

        return true;
      },
      [ctx, client](bool /*success*/) { mark_client_closed(ctx, client, true); });
}

static void handle_http_request(TransportContext* ctx, const httplib::Request& req,
                                httplib::Response& res) {
  const std::string method = normalize_http_method(req.method);
  const std::string path   = req.path;

  if (!is_origin_allowed(ctx, req)) {
    write_json_error_response(res, 403, kJsonErrorForbiddenOrigin);
    return;
  }

  if (method == "OPTIONS") {
    write_options_response(res);
    return;
  }

  if (method == "GET" && path == MCP_ENDPOINT_MCP) {
    handle_get_sse(ctx, req, res);
    return;
  }

  if (method == "POST" && path == MCP_ENDPOINT_MCP) {
    const std::string content_type = get_header_value(req, "Content-Type");
    if (!content_type.empty() && !contains_case_insensitive(content_type, "application/json")) {
      write_json_error_response(res, 415, kJsonErrorUnsupportedMediaType, true,
                                get_header_value(req, "MCP-Session-Id"));
      return;
    }

    const std::string accept = get_header_value(req, "Accept");
    if (!accept.empty() && !contains_case_insensitive(accept, "application/json") &&
        !contains_case_insensitive(accept, "*/*")) {
      write_json_error_response(res, 406, kJsonErrorNotAcceptableJson, true,
                                get_header_value(req, "MCP-Session-Id"));
      return;
    }
  }

  if (method_can_have_body(method) && req.body.size() > ctx->max_payload_size) {
    write_json_error_response(res, 413, kJsonErrorPayloadTooLarge, path == MCP_ENDPOINT_MCP,
                              get_header_value(req, "MCP-Session-Id"));
    return;
  }

  const char* body_or_null = method_can_have_body(method) ? req.body.c_str() : nullptr;
  dispatch_http_request(ctx, req, res, method, path, body_or_null);
}

static void transport_thread_main(TransportContext* ctx) {
  ctx->server = std::make_unique<httplib::Server>();
  ctx->server->set_payload_max_length(ctx->max_payload_size);

  auto handler = [ctx](const httplib::Request& req, httplib::Response& res) {
    handle_http_request(ctx, req, res);
  };

  ctx->server->Get(R"(.*)", handler);
  ctx->server->Post(R"(.*)", handler);
  ctx->server->Put(R"(.*)", handler);
  ctx->server->Patch(R"(.*)", handler);
  ctx->server->Delete(R"(.*)", handler);
  ctx->server->Options(R"(.*)", handler);

  bool bind_ok = ctx->server->bind_to_port("0.0.0.0", static_cast<int>(ctx->port));

  {
    std::lock_guard<std::mutex> lock(ctx->start_mutex);
    ctx->startup_done = true;
    ctx->startup_ok   = bind_ok;
    ctx->running.store(bind_ok, std::memory_order_release);
  }
  ctx->start_cv.notify_one();

  if (!bind_ok) {
    ctx->stopping.store(true, std::memory_order_release);
    ctx->server.reset();
    return;
  }

  ctx->server->listen_after_bind();

  ctx->running.store(false, std::memory_order_release);

  std::vector<std::shared_ptr<Client>> clients_snapshot;
  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    clients_snapshot = ctx->clients;
  }

  for (const auto& client : clients_snapshot) {
    mark_client_closed(ctx, client, true);
  }

  ctx->server.reset();
}

}  // namespace transport
}  // namespace mcp

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

static void sse_stop(mcp_transport_t* transport);

static mcp_transport_t* sse_create(uint16_t port, const mcp_transport_callbacks_t* callbacks,
                                   void* user_data) {
  auto ctx  = std::make_unique<mcp::transport::TransportContext>();
  ctx->port = port;
  if (callbacks) {
    ctx->callbacks = *callbacks;
  }
  if (ctx->callbacks.max_payload_size > 0) {
    ctx->max_payload_size = ctx->callbacks.max_payload_size;
  }
  ctx->user_data = user_data;

  if (const char* env_origin = std::getenv("DMCP_ALLOWED_ORIGIN"); env_origin && env_origin[0]) {
    ctx->allowed_origin = env_origin;
  }

  return reinterpret_cast<mcp_transport_t*>(ctx.release());
}

static void sse_destroy(mcp_transport_t* transport) {
  if (!transport) return;
  std::unique_ptr<mcp::transport::TransportContext> ctx(
      reinterpret_cast<mcp::transport::TransportContext*>(transport));

  sse_stop(transport);

  std::vector<std::shared_ptr<mcp::transport::Client>> clients_snapshot;
  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    clients_snapshot = ctx->clients;
    ctx->clients.clear();
  }

  for (const auto& client : clients_snapshot) {
    mcp::transport::mark_client_closed(ctx.get(), client, true);
  }
}

static bool sse_start(mcp_transport_t* transport) {
  if (!transport) return false;
  auto* ctx = reinterpret_cast<mcp::transport::TransportContext*>(transport);

  if (ctx->running.load(std::memory_order_acquire)) {
    return false;
  }

  {
    std::lock_guard<std::mutex> lock(ctx->start_mutex);
    ctx->startup_done = false;
    ctx->startup_ok   = false;
  }

  ctx->thread = std::thread(mcp::transport::transport_thread_main, ctx);

  std::unique_lock<std::mutex> lock(ctx->start_mutex);
  if (!ctx->start_cv.wait_for(lock, std::chrono::seconds(MCP_STARTUP_TIMEOUT_SECONDS),
                              [ctx] { return ctx->startup_done; })) {
    ctx->stopping.store(true, std::memory_order_release);
    if (ctx->server) {
      ctx->server->stop();
    }
    lock.unlock();
    if (ctx->thread.joinable()) {
      ctx->thread.join();
    }
    return false;
  }

  if (!ctx->startup_ok) {
    lock.unlock();
    if (ctx->thread.joinable()) {
      ctx->thread.join();
    }
    return false;
  }

  return true;
}

static void sse_stop(mcp_transport_t* transport) {
  if (!transport) return;
  auto* ctx = reinterpret_cast<mcp::transport::TransportContext*>(transport);

  ctx->stopping.store(true, std::memory_order_release);

  if (ctx->server) {
    ctx->server->stop();
  }

  std::vector<std::shared_ptr<mcp::transport::Client>> clients_snapshot;
  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    clients_snapshot = ctx->clients;
  }

  for (const auto& client : clients_snapshot) {
    mcp::transport::mark_client_closed(ctx, client, true);
  }

  if (ctx->thread.joinable()) {
    ctx->thread.join();
  }
}

static bool sse_is_running(const mcp_transport_t* transport) {
  if (!transport) return false;
  auto* ctx = reinterpret_cast<const mcp::transport::TransportContext*>(transport);
  return ctx->running.load(std::memory_order_acquire);
}

static void sse_broadcast(mcp_transport_t* transport, const char* data, size_t len) {
  if (!transport || !data || len == 0) return;
  auto*             ctx    = reinterpret_cast<mcp::transport::TransportContext*>(transport);
  const std::string framed = mcp::format_sse_message(std::string_view(data, len));

  std::vector<std::shared_ptr<mcp::transport::Client>> clients_snapshot;
  {
    std::lock_guard<std::mutex> lock(ctx->clients_mutex);
    clients_snapshot = ctx->clients;
  }

  for (const auto& client : clients_snapshot) {
    if (!client || client->closed.load(std::memory_order_acquire)) {
      continue;
    }

    {
      std::lock_guard<std::mutex> lock(client->mutex);
      if (!client->closed.load(std::memory_order_acquire)) {
        if (client->pending.size() >= mcp::transport::kMaxPendingEventsPerClient) {
          client->pending.pop_front();
        }
        client->pending.push_back(framed);
      }
    }
    client->cv.notify_one();
  }
}

static size_t sse_get_client_count(const mcp_transport_t* transport) {
  if (!transport) return 0;
  auto* ctx = reinterpret_cast<const mcp::transport::TransportContext*>(transport);
  std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(ctx->clients_mutex));
  return ctx->clients.size();
}

const mcp_transport_interface_t mcp_sse_transport = {
    1,                     // version
    "sse-httplib",         // name
    sse_create,            // create
    sse_destroy,           // destroy
    sse_start,             // start
    sse_stop,              // stop
    sse_is_running,        // is_running
    sse_broadcast,         // broadcast
    sse_get_client_count,  // get_client_count
};

}  // extern "C"
