#include "mcp/generic/server.h"
#include "mcp/generic/transport.h"
#include "json/json.hpp"

#include <atomic>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mcp {

// ============================================================================
// Internal Types
// ============================================================================

using json::Builder;
using json::Document;
using json::Value;

struct MethodHandler {
  mcp_method_handler_t fn;
  void* user_data;
};

struct Server {
  mcp_server_config_t config;
  
  // Transport
  mcp_transport_t* transport;
  
  // Method handlers
  std::unordered_map<std::string, MethodHandler> methods;
  std::mutex methods_mutex;
  
  // Statistics
  std::atomic<uint64_t> connected_clients{0};
  std::atomic<uint64_t> requests_handled{0};
  std::atomic<uint64_t> requests_failed{0};
  std::atomic<uint64_t> events_broadcast{0};
  std::atomic<uint64_t> bytes_sent{0};
  
  // State
  std::atomic<bool> running{false};
};

// ============================================================================
// Protocol Helpers
// ============================================================================

static void BuildCapabilities(Builder& b, bool screenshot_enabled) {
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
  info.add("name", MCP_SERVER_NAME);
  info.add("version", MCP_SERVER_VERSION);
  b.add("serverInfo", info);
}

// ============================================================================
// HTTP Handlers
// ============================================================================

static bool HandleMCPRequest(Server* server, 
                             const char* body,
                             char* response_buffer,
                             size_t response_size,
                             int* http_status) {
  *http_status = 200;
  
  Document doc;
  if (!doc.parse(body)) {
    // Invalid JSON - return capabilities
    Builder b;
    b.start_object();
    b.add("jsonrpc", "2.0");
    Builder result;
    BuildCapabilities(result, true);
    b.add("result", result);
    
    std::string resp = b.finish();
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    std::strcpy(response_buffer, resp.c_str());
    return true;
  }
  
  Value req = doc.root();
  std::string method = std::string(req["method"].get_string(""));
  
  if (method.empty()) {
    *http_status = 400;
    return false;
  }
  
  std::string id = std::string(req["id"].get_string("null"));
  
  // Handle built-in methods
  if (method == "initialize") {
    Builder b;
    b.start_object();
    b.add("jsonrpc", "2.0");
    b.add("id", id);
    Builder result;
    BuildCapabilities(result, true);
    b.add("result", result);
    
    std::string resp = b.finish();
    if (resp.size() >= response_size) {
      *http_status = 500;
      return false;
    }
    std::strcpy(response_buffer, resp.c_str());
    server->requests_handled++;
    return true;
  }
  
  if (method == "notifications/initialized") {
    *http_status = 202;
    response_buffer[0] = '\0';
    return true;
  }
  
  // Try registered handlers
  bool handled = false;
  {
    std::lock_guard<std::mutex> lock(server->methods_mutex);
    auto it = server->methods.find(method);
    if (it != server->methods.end()) {
      Value params = req["params"];
      char handler_response[8192];
      
      std::string params_str;
      if (params) {
        // Need to serialize params - create a temp doc
        Builder pb;
        pb.start_object();
        // Copy params to builder... simplified for now
        params_str = "{}";
      } else {
        params_str = "{}";
      }
      
      if (it->second.fn(it->second.user_data, method.c_str(),
                        params_str.c_str(),
                        handler_response, sizeof(handler_response))) {
        std::strcpy(response_buffer, handler_response);
        handled = true;
      }
    }
  }
  
  if (!handled) {
    // Method not found
    std::string resp = json::make_jsonrpc_error(id, -32601, "Method not found");
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

// ============================================================================
// Transport Callbacks
// ============================================================================

static bool OnHttpRequest(void* user_data,
                          const char* method,
                          const char* path,
                          const char* body,
                          char* response_buffer,
                          size_t response_size,
                          int* http_status) {
  auto* server = static_cast<Server*>(user_data);
  
  response_buffer[0] = '\0';
  *http_status = 404;
  
  if (std::strcmp(method, "POST") == 0 && std::strcmp(path, "/mcp") == 0) {
    return HandleMCPRequest(server, body ? body : "", 
                            response_buffer, response_size, http_status);
  }
  
  if (std::strcmp(method, "GET") == 0 && std::strcmp(path, "/health") == 0) {
    Builder b;
    b.start_object();
    b.add("status", "ok");
    b.add("clients", static_cast<int64_t>(server->connected_clients.load()));
    
    std::string resp = b.finish();
    if (resp.size() < response_size) {
      std::strcpy(response_buffer, resp.c_str());
      *http_status = 200;
    }
    return true;
  }
  
  return false;
}

// ============================================================================
// C API Implementation
// ============================================================================

}  // namespace mcp

extern "C" {

mcp_server_t* mcp_server_create(const mcp_server_config_t* config) {
  auto* server = new mcp::Server();
  
  server->config = config ? *config : mcp_default_config();
  
  // Initialize transport callbacks
  mcp_transport_callbacks_t callbacks = {};
  callbacks.on_http_request = mcp::OnHttpRequest;
  
  // Create SSE transport
  server->transport = mcp_sse_transport.create(
    server->config.port,
    &callbacks,
    server
  );
  
  if (!server->transport) {
    delete server;
    return nullptr;
  }
  
  if (!mcp_sse_transport.start(server->transport)) {
    mcp_sse_transport.destroy(server->transport);
    delete server;
    return nullptr;
  }
  
  server->running = true;
  return reinterpret_cast<mcp_server_t*>(server);
}

void mcp_server_destroy(mcp_server_t* server_handle) {
  if (!server_handle) return;
  auto* server = reinterpret_cast<mcp::Server*>(server_handle);
  
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

mcp_result_t mcp_server_register_method(mcp_server_t* server_handle,
                                        const char* method,
                                        mcp_method_handler_t handler,
                                        void* user_data) {
  if (!server_handle || !method || !handler) {
    return MCP_ERROR_INVALID_ARGS;
  }
  
  auto* server = reinterpret_cast<mcp::Server*>(server_handle);
  std::lock_guard<std::mutex> lock(server->methods_mutex);
  
  server->methods[method] = {handler, user_data};
  return MCP_OK;
}

void mcp_server_unregister_method(mcp_server_t* server_handle,
                                  const char* method) {
  if (!server_handle || !method) return;
  
  auto* server = reinterpret_cast<mcp::Server*>(server_handle);
  std::lock_guard<std::mutex> lock(server->methods_mutex);
  server->methods.erase(method);
}

void mcp_server_broadcast(mcp_server_t* server_handle,
                          const char* event_type,
                          const char* json_payload) {
  if (!server_handle || !event_type || !json_payload) return;
  
  auto* server = reinterpret_cast<mcp::Server*>(server_handle);
  
  // Format as SSE
  std::string msg = "event: " + std::string(event_type) + "\ndata: " 
                    + json_payload + "\n\n";
  
  if (server->transport) {
    mcp_sse_transport.broadcast(server->transport, msg.data(), msg.size());
  }
  
  server->events_broadcast++;
  server->bytes_sent += msg.size();
}

bool mcp_server_has_clients(const mcp_server_t* server_handle) {
  if (!server_handle) return false;
  auto* server = reinterpret_cast<const mcp::Server*>(server_handle);
  return server->connected_clients.load() > 0;
}

void mcp_server_get_stats(const mcp_server_t* server_handle,
                          mcp_server_stats_t* stats) {
  if (!server_handle || !stats) return;
  
  auto* server = reinterpret_cast<const mcp::Server*>(server_handle);
  stats->connected_clients = server->connected_clients.load();
  stats->requests_handled = server->requests_handled.load();
  stats->requests_failed = server->requests_failed.load();
  stats->events_broadcast = server->events_broadcast.load();
  stats->bytes_sent = server->bytes_sent.load();
}

size_t mcp_format_success_response(char* buffer, size_t buffer_size,
                                   const char* id,
                                   const char* result_json) {
  if (!buffer || buffer_size == 0) return 0;
  
  // Build simple JSON-RPC response
  mcp::json::Builder b;
  b.start_object();
  b.add("jsonrpc", "2.0");
  b.add("id", id ? id : "null");
  b.add("result", result_json ? result_json : "{}");
  std::string response = b.finish();
  
  if (response.size() >= buffer_size) return 0;
  std::strcpy(buffer, response.c_str());
  return response.size();
}

size_t mcp_format_error_response(char* buffer, size_t buffer_size,
                                 const char* id,
                                 int error_code,
                                 const char* error_message) {
  if (!buffer || buffer_size == 0) return 0;
  
  std::string response = mcp::json::make_jsonrpc_error(id ? id : "null", 
                                                   error_code, 
                                                   error_message ? error_message : "Unknown error");
  
  if (response.size() >= buffer_size) return 0;
  std::strcpy(buffer, response.c_str());
  return response.size();
}

}  // extern "C"
