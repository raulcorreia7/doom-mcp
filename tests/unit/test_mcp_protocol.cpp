#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "mcp/generic/constants.h"
#include "mcp/generic/protocol.h"
#include "mcp/generic/server.h"
#include "mcp/json/json.hpp"
#include "test_utils.hpp"

namespace {
std::atomic<uint16_t> g_base_port{9000};
}

static uint16_t GetUniquePort() { return g_base_port.fetch_add(1); }

static bool WaitForServer(mcp_server_t* server, uint16_t, int timeout_ms = 2000) {
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                               start)
             .count() < timeout_ms) {
    if (mcp_server_is_running(server)) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return false;
}

struct HttpResponse {
  int         status = 0;
  std::string body;
};

static HttpResponse HttpPost(uint16_t port, const char* path, const char* body) {
  HttpResponse resp;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) return resp;

  sockaddr_in addr{};
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    close(sock);
    return resp;
  }

  std::string request = "POST ";
  request += path;
  request += " HTTP/1.1\r\n";
  request += "Host: 127.0.0.1:";
  request += std::to_string(port);
  request += "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Content-Length: ";
  request += std::to_string(std::strlen(body));
  request += "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";
  request += body;

  send(sock, request.c_str(), request.size(), 0);

  char        buffer[4096] = {};
  std::string response;
  ssize_t     n;
  while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
    buffer[n] = '\0';
    response += buffer;
  }
  close(sock);

  size_t status_pos = response.find("HTTP/1.1 ");
  if (status_pos != std::string::npos) {
    resp.status = std::stoi(response.substr(status_pos + 9, 3));
  }

  size_t body_start = response.find("\r\n\r\n");
  if (body_start != std::string::npos) {
    resp.body = response.substr(body_start + 4);
  }

  return resp;
}

static std::string ExtractSessionId(const std::string& json_body) {
  mcp::json::Document doc;
  if (!doc.parse(json_body)) return "";

  auto root = doc.root();
  if (!root.has_member("result")) return "";

  auto result = root["result"];
  if (!result.has_member("sessionId")) return "";

  return std::string(result["sessionId"].get_string(""));
}

struct SessionContext {
  uint16_t    port;
  std::string session_id;
};

static SessionContext PerformFullLifecycle(uint16_t port) {
  SessionContext ctx;
  ctx.port = port;

  HttpResponse init = HttpPost(
      port, MCP_ENDPOINT_MCP,
      R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}})");

  ctx.session_id = ExtractSessionId(init.body);

  std::string init_done =
      R"({"jsonrpc":"2.0","method":"notifications/initialized","params":{"_sessionId":")" +
      ctx.session_id + R"("}})";
  HttpPost(port, MCP_ENDPOINT_MCP, init_done.c_str());

  return ctx;
}

TEST_CASE("MCP Protocol: JSON-RPC envelope validation", "[protocol][jsonrpc]") {
  uint16_t            port   = GetUniquePort();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(WaitForServer(server, port));

  SECTION("Missing jsonrpc field returns Invalid Request") {
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, R"({"id":1,"method":"ping","params":{}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32600);
  }

  SECTION("Invalid jsonrpc value returns Invalid Request") {
    HttpResponse resp =
        HttpPost(port, MCP_ENDPOINT_MCP, R"({"jsonrpc":"1.0","id":1,"method":"ping","params":{}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32600);
  }

  SECTION("Missing method field returns Invalid Request") {
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, R"({"jsonrpc":"2.0","id":1,"params":{}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32600);
  }

  SECTION("Empty method returns Invalid Request") {
    HttpResponse resp =
        HttpPost(port, MCP_ENDPOINT_MCP, R"({"jsonrpc":"2.0","id":1,"method":"","params":{}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32600);
  }

  SECTION("Invalid JSON returns Parse error") {
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, "{not valid json}");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32700);
  }

  SECTION("Non-object root returns Invalid Request") {
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, R"("just a string")");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32600);
  }

  mcp_server_destroy(server);
}

TEST_CASE("MCP Protocol: Initialize parameter validation", "[protocol][initialize]") {
  uint16_t            port   = GetUniquePort();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(WaitForServer(server, port));

  SECTION("Initialize without params returns Invalid Params") {
    HttpResponse resp =
        HttpPost(port, MCP_ENDPOINT_MCP, R"({"jsonrpc":"2.0","id":1,"method":"initialize"})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32602);
  }

  SECTION("Initialize without protocolVersion returns Invalid Params") {
    HttpResponse resp = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32602);
  }

  SECTION("Initialize without capabilities returns Invalid Params") {
    HttpResponse resp = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","clientInfo":{"name":"test","version":"1.0"}}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32602);
  }

  SECTION("Initialize without clientInfo returns Invalid Params") {
    HttpResponse resp = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{}}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32602);
  }

  SECTION("Initialize with unsupported protocol returns error with supported versions") {
    HttpResponse resp = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-01-01","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32602);
    REQUIRE(root["error"].has_member("data"));
    REQUIRE(root["error"]["data"].has_member("supported"));
  }

  SECTION("Valid initialize returns result with required fields including sessionId") {
    HttpResponse resp = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("result"));
    REQUIRE(root["result"].has_member("protocolVersion"));
    REQUIRE(root["result"].has_member("capabilities"));
    REQUIRE(root["result"].has_member("serverInfo"));
    REQUIRE(root["result"].has_member("sessionId"));
    REQUIRE(std::string(root["result"]["protocolVersion"].get_string("")) == MCP_PROTOCOL_VERSION);
    REQUIRE_FALSE(std::string(root["result"]["sessionId"].get_string("")).empty());
  }

  SECTION("Each initialize creates a new session") {
    HttpResponse resp1 = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}})");

    HttpResponse resp2 = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","id":2,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test2","version":"1.0"}}})");

    std::string sid1 = ExtractSessionId(resp1.body);
    std::string sid2 = ExtractSessionId(resp2.body);

    REQUIRE_FALSE(sid1.empty());
    REQUIRE_FALSE(sid2.empty());
    REQUIRE(sid1 != sid2);
  }

  mcp_server_destroy(server);
}

TEST_CASE("MCP Protocol: Lifecycle gating", "[protocol][lifecycle]") {
  uint16_t            port   = GetUniquePort();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(WaitForServer(server, port));

  SECTION("Request without session returns ServerNotInitialized") {
    HttpResponse resp =
        HttpPost(port, MCP_ENDPOINT_MCP, R"({"jsonrpc":"2.0","id":1,"method":"tools/list"})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32002);
  }

  SECTION("Request with invalid session returns ServerNotInitialized") {
    HttpResponse resp = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","id":1,"method":"tools/list","params":{"_sessionId":"invalid123"}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32002);
  }

  SECTION("Request after initialize but before initialized returns ServerNotInitialized") {
    HttpResponse init = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}})");

    std::string sid = ExtractSessionId(init.body);
    REQUIRE_FALSE(sid.empty());

    std::string req =
        R"({"jsonrpc":"2.0","id":2,"method":"ping","params":{"_sessionId":")" + sid + R"("}})";
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, req.c_str());
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32002);
  }

  SECTION("Full lifecycle allows requests") {
    SessionContext ctx = PerformFullLifecycle(port);

    std::string req = R"({"jsonrpc":"2.0","id":3,"method":"ping","params":{"_sessionId":")" +
                      ctx.session_id + R"("}})";
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, req.c_str());
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("result"));
  }

  SECTION("Initialize as notification returns error") {
    HttpResponse resp = HttpPost(
        port, MCP_ENDPOINT_MCP,
        R"({"jsonrpc":"2.0","method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
  }

  mcp_server_destroy(server);
}

TEST_CASE("MCP Protocol: Method not found", "[protocol][method]") {
  uint16_t            port   = GetUniquePort();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(WaitForServer(server, port));

  SessionContext ctx = PerformFullLifecycle(port);

  SECTION("Unknown method returns Method not found") {
    std::string req =
        R"({"jsonrpc":"2.0","id":2,"method":"unknown_method","params":{"_sessionId":")" +
        ctx.session_id + R"("}})";
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, req.c_str());
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32601);
  }

  mcp_server_destroy(server);
}

TEST_CASE("MCP Protocol: Ping method", "[protocol][ping]") {
  uint16_t            port   = GetUniquePort();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(WaitForServer(server, port));

  SessionContext ctx = PerformFullLifecycle(port);

  SECTION("Ping returns empty result") {
    std::string req = R"({"jsonrpc":"2.0","id":2,"method":"ping","params":{"_sessionId":")" +
                      ctx.session_id + R"("}})";
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, req.c_str());
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("result"));
  }

  SECTION("Ping notification returns empty response") {
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, R"({"jsonrpc":"2.0","method":"ping"})");
    REQUIRE(resp.status == 202);
    REQUIRE(resp.body.empty());
  }

  mcp_server_destroy(server);
}

TEST_CASE("MCP Protocol: Multiple sessions", "[protocol][session]") {
  uint16_t            port   = GetUniquePort();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(WaitForServer(server, port));

  SECTION("Two clients can initialize independently") {
    SessionContext ctx1 = PerformFullLifecycle(port);
    SessionContext ctx2 = PerformFullLifecycle(port);

    REQUIRE_FALSE(ctx1.session_id.empty());
    REQUIRE_FALSE(ctx2.session_id.empty());
    REQUIRE(ctx1.session_id != ctx2.session_id);

    std::string req1 = R"({"jsonrpc":"2.0","id":10,"method":"ping","params":{"_sessionId":")" +
                       ctx1.session_id + R"("}})";
    std::string req2 = R"({"jsonrpc":"2.0","id":11,"method":"ping","params":{"_sessionId":")" +
                       ctx2.session_id + R"("}})";

    HttpResponse resp1 = HttpPost(port, MCP_ENDPOINT_MCP, req1.c_str());
    HttpResponse resp2 = HttpPost(port, MCP_ENDPOINT_MCP, req2.c_str());

    mcp::json::Document doc1, doc2;
    REQUIRE(doc1.parse(resp1.body));
    REQUIRE(doc2.parse(resp2.body));

    REQUIRE(doc1.root().has_member("result"));
    REQUIRE(doc2.root().has_member("result"));
  }

  mcp_server_destroy(server);
}
