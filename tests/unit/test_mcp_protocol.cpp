#include <chrono>
#include <string>

#include "mcp/generic/constants.h"
#include "mcp/generic/protocol.h"
#include "mcp/generic/server.h"
#include "mcp/json/json.hpp"
#include "support/mcp_http_client.hpp"
#include "support/network.hpp"
#include "test_utils.hpp"

using dmcp::test::ExtractSessionId;
using dmcp::test::HttpPost;
using dmcp::test::HttpResponse;
using dmcp::test::PerformFullLifecycle;
using dmcp::test::SessionContext;
using dmcp::test::SessionHeaders;

TEST_CASE("MCP Protocol: JSON-RPC envelope validation", "[protocol][jsonrpc]") {
  uint16_t            port   = dmcp::test::allocate_loopback_port();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(dmcp::test::wait_for_server_running(server, std::chrono::milliseconds(2000)));

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

  SECTION("Fractional numeric id returns Invalid Request") {
    HttpResponse resp =
        HttpPost(port, MCP_ENDPOINT_MCP, R"({"jsonrpc":"2.0","id":1.5,"method":"ping"})");
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32600);
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
  uint16_t            port   = dmcp::test::allocate_loopback_port();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(dmcp::test::wait_for_server_running(server, std::chrono::milliseconds(2000)));

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
  uint16_t            port   = dmcp::test::allocate_loopback_port();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(dmcp::test::wait_for_server_running(server, std::chrono::milliseconds(2000)));

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
    HttpResponse resp =
        HttpPost(port, MCP_ENDPOINT_MCP, R"({"jsonrpc":"2.0","id":1,"method":"tools/list"})",
                 SessionHeaders("invalid123"));
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

    std::string  req  = R"({"jsonrpc":"2.0","id":2,"method":"ping"})";
    HttpResponse resp = HttpPost(port, MCP_ENDPOINT_MCP, req.c_str(), SessionHeaders(sid));
    REQUIRE(resp.status == 200);

    mcp::json::Document doc;
    REQUIRE(doc.parse(resp.body));
    auto root = doc.root();
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32002);
  }

  SECTION("Full lifecycle allows requests") {
    SessionContext ctx = PerformFullLifecycle(port);

    std::string  req = R"({"jsonrpc":"2.0","id":3,"method":"ping"})";
    HttpResponse resp =
        HttpPost(port, MCP_ENDPOINT_MCP, req.c_str(), SessionHeaders(ctx.session_id));
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
  uint16_t            port   = dmcp::test::allocate_loopback_port();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(dmcp::test::wait_for_server_running(server, std::chrono::milliseconds(2000)));

  SessionContext ctx = PerformFullLifecycle(port);

  SECTION("Unknown method returns Method not found") {
    std::string  req = R"({"jsonrpc":"2.0","id":2,"method":"unknown_method"})";
    HttpResponse resp =
        HttpPost(port, MCP_ENDPOINT_MCP, req.c_str(), SessionHeaders(ctx.session_id));
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
  uint16_t            port   = dmcp::test::allocate_loopback_port();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(dmcp::test::wait_for_server_running(server, std::chrono::milliseconds(2000)));

  SessionContext ctx = PerformFullLifecycle(port);

  SECTION("Ping returns empty result") {
    std::string  req = R"({"jsonrpc":"2.0","id":2,"method":"ping"})";
    HttpResponse resp =
        HttpPost(port, MCP_ENDPOINT_MCP, req.c_str(), SessionHeaders(ctx.session_id));
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
  uint16_t            port   = dmcp::test::allocate_loopback_port();
  mcp_server_config_t config = mcp_default_config();
  config.port                = port;
  mcp_server_t* server       = mcp_server_create(&config);
  REQUIRE(server != nullptr);
  REQUIRE(dmcp::test::wait_for_server_running(server, std::chrono::milliseconds(2000)));

  SECTION("Two clients can initialize independently") {
    SessionContext ctx1 = PerformFullLifecycle(port);
    SessionContext ctx2 = PerformFullLifecycle(port);

    REQUIRE_FALSE(ctx1.session_id.empty());
    REQUIRE_FALSE(ctx2.session_id.empty());
    REQUIRE(ctx1.session_id != ctx2.session_id);

    std::string req1 = R"({"jsonrpc":"2.0","id":10,"method":"ping"})";
    std::string req2 = R"({"jsonrpc":"2.0","id":11,"method":"ping"})";

    HttpResponse resp1 =
        HttpPost(port, MCP_ENDPOINT_MCP, req1.c_str(), SessionHeaders(ctx1.session_id));
    HttpResponse resp2 =
        HttpPost(port, MCP_ENDPOINT_MCP, req2.c_str(), SessionHeaders(ctx2.session_id));

    mcp::json::Document doc1, doc2;
    REQUIRE(doc1.parse(resp1.body));
    REQUIRE(doc2.parse(resp2.body));

    REQUIRE(doc1.root().has_member("result"));
    REQUIRE(doc2.root().has_member("result"));
  }

  mcp_server_destroy(server);
}
