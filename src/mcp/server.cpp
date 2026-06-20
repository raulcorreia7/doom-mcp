#include "mcp/generic/server.h"

#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "json/json.hpp"
#include "mcp/core/memory.h"
#include "mcp/core/string.h"
#include "mcp/generic/constants.h"
#include "mcp/generic/transport.h"
#include "mcp/json_rpc.hpp"
#include "mcp/request_context.hpp"

namespace mcp {

namespace {
constexpr int kSessionIdHexLength  = 16;
constexpr int kSessionIdBufferSize = kSessionIdHexLength + 1;  // +1 for null terminator
}  // namespace

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

struct Session {
  std::string                           id;
  std::string                           protocol_version;
  std::chrono::steady_clock::time_point created_at;
  std::atomic<bool>                     initialize_completed{false};
  std::atomic<bool>                     requires_initialized_notification{true};
  std::atomic<bool>                     initialized_notification_received{false};
};

struct Server {
  mcp_server_config_t config;

  // Transport
  const mcp_transport_interface_t* transport_api = nullptr;
  mcp_transport_t*                 transport     = nullptr;

  // Method handlers
  std::unordered_map<std::string, MethodHandler> methods;
  std::mutex                                     methods_mutex;

  // HTTP routes
  std::unordered_map<std::string, RouteHandler> routes;
  std::mutex                                    routes_mutex;

  // Sessions (per-client lifecycle state)
  std::unordered_map<std::string, std::shared_ptr<Session>> sessions;
  std::mutex                                                sessions_mutex;
  std::mt19937                                              rng;
  std::mutex                                                rng_mutex;

  // Statistics
  std::atomic<uint64_t> connected_clients{0};
  std::atomic<uint64_t> requests_handled{0};
  std::atomic<uint64_t> requests_failed{0};
  std::atomic<uint64_t> events_broadcast{0};
  std::atomic<uint64_t> bytes_sent{0};

  // State
  std::atomic<bool> running{false};
};

static constexpr int kJsonRpcServerNotInitialized = -32002;

static std::string GenerateSessionId(Server* server) {
  std::lock_guard<std::mutex>             lock(server->rng_mutex);
  std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

  char buf[kSessionIdBufferSize];
  std::snprintf(buf, sizeof(buf), "%016" PRIx64, dist(server->rng));
  return std::string(buf);
}

static std::shared_ptr<Session> FindSessionById(Server* server, std::string_view session_id) {
  if (!server || session_id.empty()) {
    return nullptr;
  }

  std::lock_guard<std::mutex> lock(server->sessions_mutex);
  auto                        it = server->sessions.find(std::string(session_id));
  if (it != server->sessions.end()) {
    return it->second;
  }
  return nullptr;
}

static std::shared_ptr<Session> CreateSession(Server* server, std::string& out_session_id) {
  if (!server) {
    return nullptr;
  }

  for (int attempt = 0; attempt < 8; ++attempt) {
    const std::string candidate = GenerateSessionId(server);

    std::lock_guard<std::mutex> lock(server->sessions_mutex);
    if (server->sessions.find(candidate) != server->sessions.end()) {
      continue;
    }

    auto session        = std::make_shared<Session>();
    session->id         = candidate;
    session->created_at = std::chrono::steady_clock::now();

    out_session_id              = candidate;
    server->sessions[candidate] = session;
    return session;
  }

  out_session_id.clear();
  return nullptr;
}

static bool DeleteSessionById(Server* server, std::string_view session_id) {
  if (!server || session_id.empty()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(server->sessions_mutex);
  return server->sessions.erase(std::string(session_id)) > 0;
}

static std::string ExtractSessionId(const Value& params) {
  const auto& request_meta = request_context::current();
  if (!request_meta.session_id.empty()) {
    return request_meta.session_id;
  }

  if (params && params.is_object() && params.has_member("sessionId")) {
    Value sid = params["sessionId"];
    if (sid.is_string()) {
      return std::string(sid.get_string(""));
    }
  }
  return "";
}

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
    mcp_strcpy_safe(buffer + sizeof(buffer) - 4, 4, "...");
  }

  server->config.on_log(server->config.log_user_data, level, buffer);
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
         route_key == BuildRouteKey("DELETE", MCP_ENDPOINT_MCP) ||
         route_key == BuildRouteKey("GET", MCP_ENDPOINT_HEALTH);
}

static constexpr std::array<std::string_view, 4> kSupportedProtocolVersions = {
    MCP_PROTOCOL_VERSION, "2025-06-18", "2025-03-26", "2024-11-05"};

static bool IsSupportedProtocolVersion(std::string_view requested_version) {
  for (std::string_view version : kSupportedProtocolVersions) {
    if (requested_version == version) {
      return true;
    }
  }
  return false;
}

static bool RequiresInitializedNotification(std::string_view protocol_version) {
  return protocol_version == MCP_PROTOCOL_VERSION;
}

static std::string BuildUnsupportedProtocolData(std::string_view requested_version) {
  Builder data;
  data.start_object();

  Builder supported;
  supported.start_array();
  for (std::string_view version : kSupportedProtocolVersions) {
    supported.push(version);
  }
  data.add("supported", std::move(supported));
  data.add("requested", requested_version);
  return data.finish();
}

// ============================================================================
// HTTP Handlers
// ============================================================================

static bool HandleMCPRequest(Server* server, const char* body, char* response_buffer,
                             size_t response_size, int* http_status) {
  *http_status = 200;
  ServerLog(server, MCP_LOG_DEBUG, "HTTP POST %s", MCP_ENDPOINT_MCP);
  request_context::set_response_protocol_version(MCP_PROTOCOL_VERSION);

  Document doc;
  if (!doc.parse(body ? body : "")) {
    ServerLog(server, MCP_LOG_WARN, "Parse error on %s", MCP_ENDPOINT_MCP);
    std::string resp = BuildJsonRpcError("null", -32700, "Parse error");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
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
    mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
    server->requests_failed++;
    return true;
  }

  bool        is_notification = !req.has_member("id");
  std::string id_json         = "null";
  if (!is_notification) {
    Value id_value = req["id"];
    if (!IsValidJsonRpcId(id_value)) {
      ServerLog(server, MCP_LOG_WARN, "Invalid JSON-RPC id");
      std::string resp = BuildJsonRpcError("null", -32600, "Invalid Request");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      server->requests_failed++;
      return true;
    }
    id_json = id_value.dump();
  }

  Value jsonrpc_value = req["jsonrpc"];
  if (!jsonrpc_value.is_string() ||
      std::string(jsonrpc_value.get_string("")) != MCP_JSONRPC_VERSION) {
    ServerLog(server, MCP_LOG_WARN, "Invalid JSON-RPC request: jsonrpc must be %s",
              MCP_JSONRPC_VERSION);
    std::string resp = BuildJsonRpcError(id_json, -32600, "Invalid Request");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
    server->requests_failed++;
    return true;
  }

  Value method_value = req["method"];
  if (!method_value.is_string()) {
    ServerLog(server, MCP_LOG_WARN, "Invalid JSON-RPC request: method missing or not string");
    std::string resp = BuildJsonRpcError(id_json, -32600, "Invalid Request");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
    server->requests_failed++;
    return true;
  }

  std::string method = std::string(method_value.get_string(""));
  ServerLog(server, MCP_LOG_DEBUG, "JSON-RPC method=%s notification=%s", method.c_str(),
            is_notification ? "true" : "false");

  if (method.empty()) {
    ServerLog(server, MCP_LOG_WARN, "Invalid JSON-RPC request: empty method");
    std::string resp = BuildJsonRpcError(id_json, -32600, "Invalid Request");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
    server->requests_failed++;
    return true;
  }

  // Handle built-in methods
  if (method == "initialize") {
    ServerLog(server, MCP_LOG_DEBUG, "MCP initialize request received");

    if (is_notification) {
      std::string resp = BuildJsonRpcError("null", -32600, "initialize must be a request");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      server->requests_failed++;
      return true;
    }

    Value params = req["params"];
    if (!params.is_object()) {
      std::string resp = BuildJsonRpcError(id_json, -32602, "initialize params must be an object");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      server->requests_failed++;
      return true;
    }

    Value       protocol_version = params["protocolVersion"];
    std::string requested_version;
    if (protocol_version.is_string()) {
      requested_version = std::string(protocol_version.get_string(""));
    } else {
      requested_version = request_context::current().protocol_version;
    }

    if (requested_version.empty()) {
      std::string resp =
          BuildJsonRpcError(id_json, -32602, "initialize.params.protocolVersion is required");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      server->requests_failed++;
      return true;
    }

    if (!IsSupportedProtocolVersion(requested_version)) {
      std::string resp = BuildJsonRpcError(id_json, -32602, "Unsupported protocol version",
                                           BuildUnsupportedProtocolData(requested_version));
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      server->requests_failed++;
      return true;
    }

    if (!params["capabilities"].is_object()) {
      std::string resp =
          BuildJsonRpcError(id_json, -32602, "initialize.params.capabilities is required");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      server->requests_failed++;
      return true;
    }

    if (!params["clientInfo"].is_object()) {
      std::string resp =
          BuildJsonRpcError(id_json, -32602, "initialize.params.clientInfo is required");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      server->requests_failed++;
      return true;
    }

    std::string              session_id;
    std::shared_ptr<Session> session = CreateSession(server, session_id);
    if (!session) {
      const std::string resp = BuildJsonRpcError(id_json, -32603, "Failed to create session");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      server->requests_failed++;
      return true;
    }

    session->initialize_completed.store(true, std::memory_order_release);
    session->protocol_version = requested_version;
    session->requires_initialized_notification.store(
        RequiresInitializedNotification(session->protocol_version), std::memory_order_release);
    session->initialized_notification_received.store(
        !session->requires_initialized_notification.load(std::memory_order_acquire),
        std::memory_order_release);

    request_context::set_response_protocol_version(session->protocol_version);
    request_context::set_response_session_id(session_id);

    Builder b;
    b.start_object();
    b.add("protocolVersion", session->protocol_version);

    Builder caps;
    caps.start_object();
    Builder tools;
    tools.start_object();
    tools.add("listChanged", true);
    caps.add("tools", tools);
    b.add("capabilities", caps);

    Builder info;
    info.start_object();
    info.add("name", server->config.server_name[0] ? server->config.server_name : "mcp-server");
    info.add("version", MCP_SERVER_VERSION);
    b.add("serverInfo", info);

    b.add("sessionId", session_id);

    std::string resp = BuildJsonRpcResult(id_json, b.finish());

    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
    server->requests_handled++;
    return true;
  }

  if (method == "notifications/initialized") {
    ServerLog(server, MCP_LOG_DEBUG, "MCP notifications/initialized received");

    std::string                    session_id = ExtractSessionId(req["params"]);
    const std::shared_ptr<Session> session    = FindSessionById(server, session_id);

    if (!session || !session->initialize_completed.load(std::memory_order_acquire)) {
      if (is_notification) {
        *http_status       = 202;
        response_buffer[0] = '\0';
        server->requests_failed++;
        return true;
      }

      const std::string resp = BuildJsonRpcError(id_json, kJsonRpcServerNotInitialized,
                                                 "Server not initialized: call initialize first");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      server->requests_failed++;
      return true;
    }

    request_context::set_response_protocol_version(session->protocol_version);
    request_context::set_response_session_id(session->id);
    session->initialized_notification_received.store(true, std::memory_order_release);

    if (is_notification) {
      *http_status       = 202;
      response_buffer[0] = '\0';
    } else {
      const std::string resp = BuildJsonRpcResult(id_json, "{}");
      if (resp.size() >= response_size) {
        *http_status = 500;
        return false;
      }
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
    }
    server->requests_handled++;
    return true;
  }

  std::string                    session_id = ExtractSessionId(req["params"]);
  const std::shared_ptr<Session> session    = FindSessionById(server, session_id);

  if (!session || !session->initialize_completed.load(std::memory_order_acquire) ||
      (session->requires_initialized_notification.load(std::memory_order_acquire) &&
       !session->initialized_notification_received.load(std::memory_order_acquire))) {
    ServerLog(server, MCP_LOG_WARN, "Request before initialization complete: method=%s session=%s",
              method.c_str(), session_id.c_str());

    if (is_notification) {
      *http_status       = 202;
      response_buffer[0] = '\0';
      server->requests_failed++;
      return true;
    }

    const std::string resp = BuildJsonRpcError(
        id_json, kJsonRpcServerNotInitialized,
        "Client not initialized: send notifications/initialized after initialize");
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }

    mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
    server->requests_failed++;
    return true;
  }

  request_context::set_response_protocol_version(session->protocol_version);
  request_context::set_response_session_id(session->id);

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
      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
    }
    server->requests_handled++;
    return true;
  }

  // Try registered handlers
  bool          handled = false;
  MethodHandler handler{};
  bool          found_handler = false;
  {
    std::lock_guard<std::mutex> lock(server->methods_mutex);
    auto                        it = server->methods.find(method);
    if (it != server->methods.end()) {
      handler       = it->second;
      found_handler = true;
    }
  }

  if (found_handler) {
    ServerLog(server, MCP_LOG_DEBUG, "Method found in registry: %s", method.c_str());
    Value params = req["params"];
    char  handler_response[MCP_BUFFER_SIZE_DEFAULT];

    std::string params_str = params ? params.dump() : "{}";

    if (handler.fn(handler.user_data, method.c_str(), params_str.c_str(), handler_response,
                   sizeof(handler_response))) {
      if (is_notification) {
        *http_status       = 202;
        response_buffer[0] = '\0';
        server->requests_handled++;
        return true;
      }

      std::string handler_json(handler_response);
      std::string resp;

      if (IsJsonRpcResponseEnvelope(handler_json)) {
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

      mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
      handled = true;
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
    mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
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

  mcp_strcpy_safe(response_buffer, response_size, resp.c_str());
  *http_status = 200;
  return true;
}

static bool HandleRouteMcp(void* user_data, const char* method, const char* path, const char* body,
                           char* response_buffer, size_t response_size, int* http_status) {
  (void)path;
  auto* server = static_cast<Server*>(user_data);
  if (!server || !method) {
    return false;
  }

  if (std::strcmp(method, "DELETE") == 0) {
    const std::string session_id = request_context::current().session_id;
    (void)DeleteSessionById(server, session_id);
    response_buffer[0] = '\0';
    *http_status       = 204;
    server->requests_handled++;
    return true;
  }

  if (std::strcmp(method, "POST") != 0) {
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

  const bool handled = handler.fn(handler.user_data, method, path, body, response_buffer,
                                  response_size, http_status);
  if (!handled && *http_status >= 500) {
    if (response_buffer[0] == '\0' && response_size > 0) {
      const char* payload =
          "{\"error\":{\"code\":\"internal_error\",\"message\":\"Internal "
          "server error\"}}";
      if (std::strlen(payload) < response_size) {
        mcp_strcpy_safe(response_buffer, response_size, payload);
      }
    }
    return true;
  }

  return handled;
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
    ServerLog(server, MCP_LOG_DEBUG, "SSE client connected (clients=%llu)",
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
      ServerLog(server, MCP_LOG_DEBUG, "SSE client disconnected (clients=%llu)",
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
  auto server = std::make_unique<mcp::Server>();

  server->config = mcp_default_config();
  if (config) {
    size_t copy_size = config->struct_size;
    if (copy_size == 0 || copy_size > sizeof(mcp_server_config_t)) {
      copy_size = sizeof(mcp_server_config_t);
    }
    mcp_memcpy_safe(&server->config, sizeof(server->config), config, copy_size);
  }
  // Seed RNG for session ID generation
  {
    std::random_device          rd;
    std::lock_guard<std::mutex> lock(server->rng_mutex);
    server->rng.seed(rd());
  }

  {
    std::lock_guard<std::mutex> lock(server->routes_mutex);
    server->routes[mcp::BuildRouteKey("POST", MCP_ENDPOINT_MCP)]   = {mcp::HandleRouteMcp,
                                                                      server.get()};
    server->routes[mcp::BuildRouteKey("DELETE", MCP_ENDPOINT_MCP)] = {mcp::HandleRouteMcp,
                                                                      server.get()};
    server->routes[mcp::BuildRouteKey("GET", MCP_ENDPOINT_HEALTH)] = {mcp::HandleRouteHealth,
                                                                      server.get()};
  }

  if (!server->config.start_transport) {
    mcp::ServerLog(server.get(), MCP_LOG_INFO, "MCP server created without HTTP/SSE transport");
    return reinterpret_cast<mcp_server_t*>(server.release());
  }

  // Initialize transport callbacks
  mcp_transport_callbacks_t callbacks = {};
  callbacks.struct_size               = sizeof(mcp_transport_callbacks_t);
  callbacks.max_payload_size          = server->config.max_payload_size;
  callbacks.on_http_request           = mcp::OnHttpRequest;
  callbacks.on_sse_connect            = mcp::OnSseConnect;
  callbacks.on_sse_disconnect         = mcp::OnSseDisconnect;

  const mcp_transport_interface_t* transport_api =
      server->config.transport ? server->config.transport : &mcp_sse_transport;

  if (!transport_api->create || !transport_api->destroy || !transport_api->start ||
      !transport_api->stop || !transport_api->is_running || !transport_api->broadcast ||
      !transport_api->get_client_count) {
    mcp::ServerLog(server.get(), MCP_LOG_ERROR, "Invalid MCP transport implementation");
    return nullptr;
  }

  server->transport_api = transport_api;

  auto transport_deleter = [transport_api](mcp_transport_t* transport) {
    if (transport) {
      transport_api->destroy(transport);
    }
  };
  std::unique_ptr<mcp_transport_t, decltype(transport_deleter)> transport(
      transport_api->create(server->config.port, &callbacks, server.get()), transport_deleter);

  if (!transport) {
    mcp::ServerLog(server.get(), MCP_LOG_ERROR, "Failed to create transport=%s port=%u",
                   transport_api->name ? transport_api->name : "unknown", server->config.port);
    return nullptr;
  }

  server->transport = transport.get();
  if (!transport_api->start(server->transport)) {
    server->transport = nullptr;
    mcp::ServerLog(server.get(), MCP_LOG_ERROR, "Failed to start transport=%s port=%u",
                   transport_api->name ? transport_api->name : "unknown", server->config.port);
    return nullptr;
  }

  server->transport = transport.release();
  server->running   = true;
  mcp::ServerLog(server.get(), MCP_LOG_INFO, "MCP server started on port=%u transport=%s",
                 server->config.port, transport_api->name ? transport_api->name : "unknown");
  return reinterpret_cast<mcp_server_t*>(server.release());
}

void mcp_server_destroy(mcp_server_t* server_handle) {
  if (!server_handle) return;
  std::unique_ptr<mcp::Server> server(reinterpret_cast<mcp::Server*>(server_handle));

  mcp::ServerLog(server.get(), MCP_LOG_INFO, "MCP server shutting down");

  if (server->transport) {
    server->transport_api->stop(server->transport);
    server->transport_api->destroy(server->transport);
    server->transport = nullptr;
  }

  server->running.store(false, std::memory_order_release);
}

bool mcp_server_is_running(const mcp_server_t* server_handle) {
  if (!server_handle) return false;
  auto* server = reinterpret_cast<const mcp::Server*>(server_handle);

  if (!server->transport) {
    return false;
  }

  return server->transport_api && server->transport_api->is_running(server->transport);
}

mcp_status_t mcp_server_route_register(mcp_server_t* server_handle, const char* method,
                                       const char* path, mcp_route_handler_t handler,
                                       void* user_data) {
  if (!server_handle || !method || !path || !handler) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  auto*       server = reinterpret_cast<mcp::Server*>(server_handle);
  std::string key    = mcp::BuildRouteKey(method, path);

  if (mcp::IsReservedRouteKey(key)) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Reserved route");
  }

  std::lock_guard<std::mutex> lock(server->routes_mutex);
  server->routes[key] = {handler, user_data};
  mcp::ServerLog(server, MCP_LOG_DEBUG, "Registered route=%s %s", method, path);
  return MCP_STATUS_OK("Success");
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

mcp_status_t mcp_server_method_register(mcp_server_t* server_handle, const char* method,
                                        mcp_method_handler_t handler, void* user_data) {
  if (!server_handle || !method || !handler) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  auto*                       server = reinterpret_cast<mcp::Server*>(server_handle);
  std::lock_guard<std::mutex> lock(server->methods_mutex);

  server->methods[method] = {handler, user_data};
  mcp::ServerLog(server, MCP_LOG_DEBUG, "Registered method=%s", method);
  return MCP_STATUS_OK("Success");
}

mcp_status_t mcp_server_methods_register(mcp_server_t*                    server_handle,
                                         const mcp_method_registration_t* methods, size_t count) {
  if (!server_handle || (count > 0 && !methods)) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  auto* server = reinterpret_cast<mcp::Server*>(server_handle);

  std::lock_guard<std::mutex> lock(server->methods_mutex);

  for (size_t i = 0; i < count; ++i) {
    if (!methods[i].method || !methods[i].handler) {
      return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
    }
  }

  server->methods.reserve(server->methods.size() + count);

  for (size_t i = 0; i < count; ++i) {
    server->methods[methods[i].method] = {methods[i].handler, methods[i].user_data};
    mcp::ServerLog(server, MCP_LOG_DEBUG, "Registered method (batch)=%s", methods[i].method);
  }

  return MCP_STATUS_OK("Success");
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

  std::string message_json;
  if (mcp::IsJsonRpcMessage(json_payload)) {
    message_json = json_payload;
  } else {
    std::string method = std::string("notifications/") + event_type;
    message_json       = mcp::BuildJsonRpcNotification(method, json_payload);
  }

  if (server->transport) {
    if (server->transport_api) {
      server->transport_api->broadcast(server->transport, message_json.data(), message_json.size());
    }
  }

  server->events_broadcast++;
  server->bytes_sent += message_json.size();
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
  mcp_strcpy_safe(buffer, buffer_size, response.c_str());
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
  mcp_strcpy_safe(buffer, buffer_size, response.c_str());
  return response.size();
}

}  // extern "C"
