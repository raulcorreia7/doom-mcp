#include "mcp/generic/server.h"

#include <atomic>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "json/json.hpp"
#include "mcp/generic/constants.h"
#include "mcp/generic/transport.h"

namespace mcp {

// ============================================================================
// Internal Types
// ============================================================================

using json::Builder;
using json::Document;
using json::Value;

struct MethodHandler {
  mcp_method_handler_t fn;
  void*                user_data;
};

struct RouteHandler {
  mcp_route_handler_t fn;
  void*               user_data;
};

struct Server {
  mcp_server_config_t config;

  // Transport
  mcp_transport_t* transport;

  // Method handlers
  std::unordered_map<std::string, MethodHandler> methods;
  std::mutex                                     methods_mutex;

  // HTTP routes
  std::unordered_map<std::string, RouteHandler> routes;
  std::mutex                                    routes_mutex;

  // Statistics
  std::atomic<uint64_t> connected_clients{0};
  std::atomic<uint64_t> requests_handled{0};
  std::atomic<uint64_t> requests_failed{0};
  std::atomic<uint64_t> events_broadcast{0};
  std::atomic<uint64_t> bytes_sent{0};

  // State
  std::atomic<bool> running{false};
};

static void ServerLog(const Server* server, int level, const char* fmt, ...) {
  if (!server || !server->config.on_log || !fmt) {
    return;
  }

  char    buffer[512];
  va_list args;
  va_start(args, fmt);
  int result = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  if (result >= static_cast<int>(sizeof(buffer))) {
    std::strcpy(buffer + sizeof(buffer) - 4, "...");
  }

  server->config.on_log(server->config.log_user_data, level, buffer);
}

static bool ParseJsonObject(std::string_view json, Document* out_doc) {
  if (!out_doc) {
    return false;
  }

  if (!out_doc->parse(json)) {
    return false;
  }

  return out_doc->root().is_object();
}

static std::string NormalizeHttpMethod(std::string_view method) {
  std::string out(method);
  for (char& c : out) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return out;
}

static std::string NormalizeHttpPath(std::string_view path) {
  if (path.empty()) {
    return "/";
  }
  std::string out(path);
  if (out.front() != '/') {
    out.insert(out.begin(), '/');
  }
  return out;
}

static std::string BuildRouteKey(std::string_view method, std::string_view path) {
  std::string key = NormalizeHttpMethod(method);
  key.push_back(' ');
  key += NormalizeHttpPath(path);
  return key;
}

static bool IsReservedRouteKey(std::string_view route_key) {
  return route_key == BuildRouteKey("POST", MCP_ENDPOINT_MCP) ||
         route_key == BuildRouteKey("GET", MCP_ENDPOINT_HEALTH);
}

static std::string BuildJsonRpcResult(std::string_view id_json, std::string_view result_json) {
  Document response_doc;
  response_doc.create_object();
  Value response_root = response_doc.root();
  response_root.set_member("jsonrpc", MCP_JSONRPC_VERSION);

  Document    id_doc;
  std::string id_envelope = "{\"id\":";
  id_envelope += id_json.empty() ? "null" : std::string(id_json);
  id_envelope += "}";

  if (ParseJsonObject(id_envelope, &id_doc)) {
    response_root.set_member("id", id_doc.root()["id"]);
  } else {
    Document null_id_doc;
    null_id_doc.parse("{\"id\":null}");
    response_root.set_member("id", null_id_doc.root()["id"]);
  }

  Document    result_doc;
  std::string result_part = result_json.empty() ? "{}" : std::string(result_json);
  if (result_doc.parse(result_part)) {
    response_root.set_member("result", result_doc.root());
  } else {
    Document empty;
    empty.create_object();
    response_root.set_member("result", empty.root());
  }

  return response_doc.dump(false);
}

static std::string BuildJsonRpcError(std::string_view id_json, int code, std::string_view message) {
  Document response_doc;
  response_doc.create_object();
  Value response_root = response_doc.root();
  response_root.set_member("jsonrpc", MCP_JSONRPC_VERSION);

  Document    id_doc;
  std::string id_envelope = "{\"id\":";
  id_envelope += id_json.empty() ? "null" : std::string(id_json);
  id_envelope += "}";

  if (ParseJsonObject(id_envelope, &id_doc)) {
    response_root.set_member("id", id_doc.root()["id"]);
  } else {
    Document null_id_doc;
    null_id_doc.parse("{\"id\":null}");
    response_root.set_member("id", null_id_doc.root()["id"]);
  }

  Document error_doc;
  error_doc.create_object();
  Value error_root = error_doc.root();
  error_root.set_member("code", static_cast<int64_t>(code));
  error_root.set_member("message", message);
  response_root.set_member("error", error_root);

  return response_doc.dump(false);
}

static bool IsValidJson(const std::string& json) {
  Document doc;
  return doc.parse(json);
}

static bool IsJsonRpcEnvelope(const std::string& json) {
  Document doc;
  if (!doc.parse(json)) {
    return false;
  }

  Value root = doc.root();
  if (!root.is_object()) {
    return false;
  }

  if (std::string(root["jsonrpc"].get_string()) != MCP_JSONRPC_VERSION) {
    return false;
  }

  return root.has_member("result") || root.has_member("error");
}

static bool IsValidJsonRpcId(const Value& id_value) {
  if (!id_value) return false;
  return id_value.is_string() || id_value.is_number() || id_value.is_null();
}

static std::string FormatSseEvent(const char* event_type, const char* json_payload) {
  std::string message = "event: ";
  message += event_type;
  message += "\n";

  if (!json_payload || json_payload[0] == '\0') {
    message += "data: {}\n\n";
    return message;
  }

  std::string payload(json_payload);
  {
    Document payload_doc;
    if (payload_doc.parse(payload)) {
      payload = payload_doc.dump(true);
    }
  }

  size_t start = 0;

  while (start <= payload.size()) {
    size_t end = payload.find('\n', start);
    if (end == std::string::npos) {
      end = payload.size();
    }

    message += "data: ";
    message.append(payload, start, end - start);
    message += "\n";

    if (end == payload.size()) {
      break;
    }
    start = end + 1;
  }

  message += "\n";
  return message;
}

// ============================================================================
// Protocol Helpers
// ============================================================================

static void BuildCapabilities(Builder& b, bool screenshot_enabled, const char* server_name) {
  b.start_object();
  b.add("protocolVersion", MCP_PROTOCOL_VERSION);

  // capabilities object
  Builder caps;
  caps.start_object();
  caps.add("notifications", true);

  Builder tools;
  tools.start_object();
  tools.add("listChanged", true);
  caps.add("tools", tools);

  b.add("capabilities", caps);

  if (screenshot_enabled) {
    // Add screenshot capability
  }

  // serverInfo object
  Builder info;
  info.start_object();
  info.add("name", server_name ? server_name : "doom-mcp");
  info.add("version", MCP_SERVER_VERSION);
  b.add("serverInfo", info);
}

// ============================================================================
// HTTP Handlers
// ============================================================================

static bool HandleMCPRequest(Server* server, const char* body, char* response_buffer,
                             size_t response_size, int* http_status) {
  *http_status = 200;
  ServerLog(server, MCP_LOG_DEBUG, "HTTP POST %s", MCP_ENDPOINT_MCP);

  Document doc;
  if (!doc.parse(body)) {
    ServerLog(server, MCP_LOG_WARN, "Parse error on %s", MCP_ENDPOINT_MCP);
    std::string resp = BuildJsonRpcError("null", -32700, "Parse error");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    std::strcpy(response_buffer, resp.c_str());
    server->requests_failed++;
    return true;
  }

  Value req = doc.root();
  if (!req.is_object()) {
    ServerLog(server, MCP_LOG_WARN, "Invalid JSON-RPC request: root is not object");
    std::string resp = BuildJsonRpcError("null", -32600, "Invalid Request");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    std::strcpy(response_buffer, resp.c_str());
    server->requests_failed++;
    return true;
  }

  Value method_value = req["method"];
  if (!method_value.is_string()) {
    ServerLog(server, MCP_LOG_WARN, "Invalid JSON-RPC request: method missing or not string");
    std::string resp = BuildJsonRpcError("null", -32600, "Invalid Request");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    std::strcpy(response_buffer, resp.c_str());
    server->requests_failed++;
    return true;
  }

  std::string method          = std::string(method_value.get_string(""));
  bool        is_notification = !req.has_member("id");
  std::string id_json         = "null";
  ServerLog(server, MCP_LOG_DEBUG, "JSON-RPC method=%s notification=%s", method.c_str(),
            is_notification ? "true" : "false");

  if (!is_notification) {
    Value id_value = req["id"];
    if (!IsValidJsonRpcId(id_value)) {
      ServerLog(server, MCP_LOG_WARN, "Invalid JSON-RPC id for method=%s", method.c_str());
      std::string resp = BuildJsonRpcError("null", -32600, "Invalid Request");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      std::strcpy(response_buffer, resp.c_str());
      server->requests_failed++;
      return true;
    }
    id_json = id_value.dump();
  }

  if (method.empty()) {
    ServerLog(server, MCP_LOG_WARN, "Invalid JSON-RPC request: empty method");
    std::string resp = BuildJsonRpcError(id_json, -32600, "Invalid Request");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    std::strcpy(response_buffer, resp.c_str());
    server->requests_failed++;
    return true;
  }

  // Handle built-in methods
  if (method == "initialize") {
    ServerLog(server, MCP_LOG_INFO, "MCP initialize request received");
    if (is_notification) {
      *http_status       = 202;
      response_buffer[0] = '\0';
      server->requests_handled++;
      return true;
    }

    Builder b;
    BuildCapabilities(b, true, server->config.server_name);

    std::string resp = BuildJsonRpcResult(id_json, b.finish());

    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    std::strcpy(response_buffer, resp.c_str());
    server->requests_handled++;
    return true;
  }

  if (method == "notifications/initialized") {
    ServerLog(server, MCP_LOG_INFO, "MCP notifications/initialized received");
    if (is_notification) {
      *http_status       = 202;
      response_buffer[0] = '\0';
    } else {
      const std::string resp = BuildJsonRpcResult(id_json, "{}");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      std::strcpy(response_buffer, resp.c_str());
    }
    server->requests_handled++;
    return true;
  }

  if (method == "ping") {
    ServerLog(server, MCP_LOG_DEBUG, "MCP ping request received");
    if (is_notification) {
      *http_status       = 202;
      response_buffer[0] = '\0';
    } else {
      const std::string resp = BuildJsonRpcResult(id_json, "{}");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      std::strcpy(response_buffer, resp.c_str());
    }
    server->requests_handled++;
    return true;
  }

  // Try registered handlers
  bool handled = false;
  {
    std::lock_guard<std::mutex> lock(server->methods_mutex);
    auto                        it = server->methods.find(method);
    if (it != server->methods.end()) {
      ServerLog(server, MCP_LOG_DEBUG, "Method found in registry: %s", method.c_str());
      Value params = req["params"];
      char  handler_response[MCP_BUFFER_SIZE_DEFAULT];

      std::string params_str = params ? params.dump() : "{}";

      if (it->second.fn(it->second.user_data, method.c_str(), params_str.c_str(), handler_response,
                        sizeof(handler_response))) {
        if (is_notification) {
          *http_status       = 202;
          response_buffer[0] = '\0';
          server->requests_handled++;
          return true;
        }

        std::string handler_json(handler_response);
        std::string resp;

        if (IsJsonRpcEnvelope(handler_json)) {
          resp = std::move(handler_json);
        } else if (IsValidJson(handler_json)) {
          resp = BuildJsonRpcResult(id_json, handler_json);
        } else {
          ServerLog(server, MCP_LOG_ERROR, "Handler returned invalid JSON for method=%s",
                    method.c_str());
          resp = BuildJsonRpcError(id_json, -32603, "Internal error");
        }

        if (resp.size() >= response_size) {
          *http_status = 500;
          return false;
        }

        std::strcpy(response_buffer, resp.c_str());
        handled = true;
      }
    }
  }

  if (!handled) {
    if (is_notification) {
      *http_status       = 202;
      response_buffer[0] = '\0';
      server->requests_handled++;
      return true;
    }

    // Method not found
    ServerLog(server, MCP_LOG_WARN, "Method not found: %s", method.c_str());
    std::string resp = BuildJsonRpcError(id_json, -32601, "Method not found");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    std::strcpy(response_buffer, resp.c_str());
    server->requests_failed++;
    return true;
  }

  server->requests_handled++;
  return true;
}

static bool HandleHealthRequest(Server* server, char* response_buffer, size_t response_size,
                                int* http_status) {
  if (!server || !response_buffer || !http_status) {
    return false;
  }

  ServerLog(server, MCP_LOG_DEBUG, "HTTP GET %s", MCP_ENDPOINT_HEALTH);
  Builder b;
  b.start_object();
  b.add("status", "ok");
  b.add("clients", static_cast<int64_t>(server->connected_clients.load()));

  std::string resp = b.finish();
  if (resp.size() >= response_size) {
    *http_status = 500;
    return false;
  }

  std::strcpy(response_buffer, resp.c_str());
  *http_status = 200;
  return true;
}

static bool HandleRouteMcp(void* user_data, const char* method, const char* path, const char* body,
                           char* response_buffer, size_t response_size, int* http_status) {
  (void)path;
  auto* server = static_cast<Server*>(user_data);
  if (!server || !method || std::strcmp(method, "POST") != 0) {
    return false;
  }
  return HandleMCPRequest(server, body ? body : "", response_buffer, response_size, http_status);
}

static bool HandleRouteHealth(void* user_data, const char* method, const char* path,
                              const char* body, char* response_buffer, size_t response_size,
                              int* http_status) {
  (void)path;
  (void)body;
  auto* server = static_cast<Server*>(user_data);
  if (!server || !method || std::strcmp(method, "GET") != 0) {
    return false;
  }
  return HandleHealthRequest(server, response_buffer, response_size, http_status);
}

static bool DispatchHttpRoute(Server* server, const char* method, const char* path,
                              const char* body, char* response_buffer, size_t response_size,
                              int* http_status) {
  if (!server || !method || !path || !response_buffer || !http_status) {
    return false;
  }

  RouteHandler handler{};
  {
    std::lock_guard<std::mutex> lock(server->routes_mutex);
    auto                        it = server->routes.find(BuildRouteKey(method, path));
    if (it == server->routes.end()) {
      ServerLog(server, MCP_LOG_WARN, "Route not found: %s %s", method, path);
      return false;
    }
    handler = it->second;
  }

  if (!handler.fn) {
    return false;
  }

  return handler.fn(handler.user_data, method, path, body, response_buffer, response_size,
                    http_status);
}

// ============================================================================
// Transport Callbacks
// ============================================================================

static bool OnHttpRequest(void* user_data, const char* method, const char* path, const char* body,
                          char* response_buffer, size_t response_size, int* http_status) {
  auto* server = static_cast<Server*>(user_data);

  if (!server || !method || !path || !response_buffer || !http_status) {
    return false;
  }

  ServerLog(server, MCP_LOG_DEBUG, "HTTP %s %s", method, path);

  response_buffer[0] = '\0';
  *http_status       = 404;
  return DispatchHttpRoute(server, method, path, body, response_buffer, response_size, http_status);
}

static void* OnSseConnect(void* user_data, void** client_handle) {
  (void)client_handle;
  auto* server = static_cast<Server*>(user_data);
  if (server) {
    uint64_t clients = server->connected_clients.fetch_add(1, std::memory_order_relaxed) + 1;
    ServerLog(server, MCP_LOG_INFO, "SSE client connected (clients=%llu)",
              static_cast<unsigned long long>(clients));
  }
  return nullptr;
}

static void OnSseDisconnect(void* user_data, void* client_handle) {
  (void)client_handle;
  auto* server = static_cast<Server*>(user_data);
  if (!server) return;

  uint64_t current = server->connected_clients.load(std::memory_order_relaxed);
  while (current > 0) {
    if (server->connected_clients.compare_exchange_weak(
            current, current - 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
      ServerLog(server, MCP_LOG_INFO, "SSE client disconnected (clients=%llu)",
                static_cast<unsigned long long>(current - 1));
      return;
    }
  }
}

// ============================================================================
// C API Implementation
// ============================================================================

}  // namespace mcp

extern "C" {

mcp_server_t* mcp_server_create(const mcp_server_config_t* config) {
  auto* server = new mcp::Server();

  server->config = config ? *config : mcp_default_config();

  {
    std::lock_guard<std::mutex> lock(server->routes_mutex);
    server->routes[mcp::BuildRouteKey("POST", MCP_ENDPOINT_MCP)]   = {mcp::HandleRouteMcp, server};
    server->routes[mcp::BuildRouteKey("GET", MCP_ENDPOINT_HEALTH)] = {mcp::HandleRouteHealth,
                                                                      server};
  }

  // Initialize transport callbacks
  mcp_transport_callbacks_t callbacks = {};
  callbacks.struct_size               = sizeof(mcp_transport_callbacks_t);
  callbacks.on_http_request           = mcp::OnHttpRequest;
  callbacks.on_sse_connect            = mcp::OnSseConnect;
  callbacks.on_sse_disconnect         = mcp::OnSseDisconnect;

  // Create SSE transport
  server->transport = mcp_sse_transport.create(server->config.port, &callbacks, server);

  if (!server->transport) {
    mcp::ServerLog(server, MCP_LOG_ERROR, "Failed to create transport=%s port=%u",
                   mcp_sse_transport.name, server->config.port);
    delete server;
    return nullptr;
  }

  if (!mcp_sse_transport.start(server->transport)) {
    mcp::ServerLog(server, MCP_LOG_ERROR, "Failed to start transport=%s port=%u",
                   mcp_sse_transport.name, server->config.port);
    mcp_sse_transport.destroy(server->transport);
    delete server;
    return nullptr;
  }

  server->running = true;
  mcp::ServerLog(server, MCP_LOG_INFO, "MCP server started on port=%u transport=%s",
                 server->config.port, mcp_sse_transport.name);
  return reinterpret_cast<mcp_server_t*>(server);
}

void mcp_server_destroy(mcp_server_t* server_handle) {
  if (!server_handle) return;
  auto* server = reinterpret_cast<mcp::Server*>(server_handle);

  mcp::ServerLog(server, MCP_LOG_INFO, "MCP server shutting down");

  if (server->transport) {
    mcp_sse_transport.stop(server->transport);
    mcp_sse_transport.destroy(server->transport);
  }

  delete server;
}

bool mcp_server_is_running(const mcp_server_t* server_handle) {
  if (!server_handle) return false;
  auto* server = reinterpret_cast<const mcp::Server*>(server_handle);
  return server->running.load();
}

mcp_result_t mcp_server_route_register(mcp_server_t* server_handle, const char* method,
                                       const char* path, mcp_route_handler_t handler,
                                       void* user_data) {
  if (!server_handle || !method || !path || !handler) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto*       server = reinterpret_cast<mcp::Server*>(server_handle);
  std::string key    = mcp::BuildRouteKey(method, path);

  if (mcp::IsReservedRouteKey(key)) {
    return MCP_RESULT_ERROR(MCP_RESULT_CODE_INVALID_ARGS, "Reserved route");
  }

  std::lock_guard<std::mutex> lock(server->routes_mutex);
  server->routes[key] = {handler, user_data};
  mcp::ServerLog(server, MCP_LOG_DEBUG, "Registered route=%s %s", method, path);
  return MCP_OK;
}

void mcp_server_route_unregister(mcp_server_t* server_handle, const char* method,
                                 const char* path) {
  if (!server_handle || !method || !path) return;

  auto*       server = reinterpret_cast<mcp::Server*>(server_handle);
  std::string key    = mcp::BuildRouteKey(method, path);
  if (mcp::IsReservedRouteKey(key)) {
    return;
  }

  std::lock_guard<std::mutex> lock(server->routes_mutex);
  server->routes.erase(key);
  mcp::ServerLog(server, MCP_LOG_DEBUG, "Unregistered route=%s %s", method, path);
}

mcp_result_t mcp_server_method_register(mcp_server_t* server_handle, const char* method,
                                        mcp_method_handler_t handler, void* user_data) {
  if (!server_handle || !method || !handler) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto*                       server = reinterpret_cast<mcp::Server*>(server_handle);
  std::lock_guard<std::mutex> lock(server->methods_mutex);

  server->methods[method] = {handler, user_data};
  mcp::ServerLog(server, MCP_LOG_DEBUG, "Registered method=%s", method);
  return MCP_OK;
}

mcp_result_t mcp_server_methods_register(mcp_server_t*                    server_handle,
                                         const mcp_method_registration_t* methods, size_t count) {
  if (!server_handle || (count > 0 && !methods)) {
    return MCP_ERROR_INVALID_ARGS;
  }

  auto* server = reinterpret_cast<mcp::Server*>(server_handle);

  std::lock_guard<std::mutex> lock(server->methods_mutex);

  server->methods.reserve(server->methods.size() + count);

  for (size_t i = 0; i < count; ++i) {
    if (!methods[i].method || !methods[i].handler) {
      return MCP_ERROR_INVALID_ARGS;
    }
    server->methods[methods[i].method] = {methods[i].handler, methods[i].user_data};
    mcp::ServerLog(server, MCP_LOG_DEBUG, "Registered method (batch)=%s", methods[i].method);
  }

  return MCP_OK;
}

void mcp_server_method_unregister(mcp_server_t* server_handle, const char* method) {
  if (!server_handle || !method) return;

  auto*                       server = reinterpret_cast<mcp::Server*>(server_handle);
  std::lock_guard<std::mutex> lock(server->methods_mutex);
  server->methods.erase(method);
  mcp::ServerLog(server, MCP_LOG_DEBUG, "Unregistered method=%s", method);
}

void mcp_server_event_broadcast(mcp_server_t* server_handle, const char* event_type,
                                const char* json_payload) {
  if (!server_handle || !event_type || !json_payload) return;

  auto* server = reinterpret_cast<mcp::Server*>(server_handle);

  std::string msg = mcp::FormatSseEvent(event_type, json_payload);

  if (server->transport) {
    mcp_sse_transport.broadcast(server->transport, msg.data(), msg.size());
  }

  server->events_broadcast++;
  server->bytes_sent += msg.size();
}

uint64_t mcp_server_clients_count(const mcp_server_t* server_handle) {
  if (!server_handle) return 0;
  auto* server = reinterpret_cast<const mcp::Server*>(server_handle);
  return server->connected_clients.load();
}

void mcp_server_stats_get(const mcp_server_t* server_handle, mcp_server_stats_t* stats) {
  if (!server_handle || !stats) return;

  auto* server             = reinterpret_cast<const mcp::Server*>(server_handle);
  stats->struct_size       = sizeof(mcp_server_stats_t);
  stats->connected_clients = server->connected_clients.load();
  stats->requests_handled  = server->requests_handled.load();
  stats->requests_failed   = server->requests_failed.load();
  stats->events_broadcast  = server->events_broadcast.load();
  stats->bytes_sent        = server->bytes_sent.load();
}

size_t mcp_format_success_response(char* buffer, size_t buffer_size, const char* id,
                                   const char* result_json) {
  if (!buffer || buffer_size == 0) return 0;

  mcp::json::Builder id_builder;
  id_builder.start_object();
  id_builder.add("id", id ? id : "null");

  mcp::json::Document id_doc;
  std::string         id_json = "\"null\"";
  if (id_doc.parse(id_builder.finish())) {
    id_json = id_doc.root()["id"].dump();
  }

  std::string result_part = result_json ? result_json : "{}";
  if (!mcp::IsValidJson(result_part)) {
    result_part = "{}";
  }

  std::string response = mcp::BuildJsonRpcResult(id_json, result_part);

  if (response.size() >= buffer_size) return 0;
  std::strcpy(buffer, response.c_str());
  return response.size();
}

size_t mcp_format_error_response(char* buffer, size_t buffer_size, const char* id, int error_code,
                                 const char* error_message) {
  if (!buffer || buffer_size == 0) return 0;

  mcp::json::Builder id_builder;
  id_builder.start_object();
  id_builder.add("id", id ? id : "null");

  mcp::json::Document id_doc;
  std::string         id_json = "\"null\"";
  if (id_doc.parse(id_builder.finish())) {
    id_json = id_doc.root()["id"].dump();
  }

  std::string response =
      mcp::BuildJsonRpcError(id_json, error_code, error_message ? error_message : "Unknown error");

  if (response.size() >= buffer_size) return 0;
  std::strcpy(buffer, response.c_str());
  return response.size();
}

}  // extern "C"
