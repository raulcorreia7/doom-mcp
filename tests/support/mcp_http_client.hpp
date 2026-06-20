#pragma once

#include <cstdint>
#include <string>

#include "mcp/generic/constants.h"
#include "mcp/generic/protocol.h"
#include "mcp/json_rpc.hpp"
#include "mcp/json/json.hpp"
#include "support/http_client.hpp"

namespace dmcp::test {

struct SessionContext {
  uint16_t    port = 0;
  std::string session_id;
};

inline HttpHeaders initialize_headers() {
  return {{"Accept", "application/json, text/event-stream"}};
}

inline HttpHeaders session_headers(const std::string& session_id) {
  return {{"MCP-Session-Id", session_id},
          {"MCP-Protocol-Version", MCP_PROTOCOL_VERSION},
          {"Accept", "application/json, text/event-stream"}};
}

inline HttpResponse http_post(uint16_t port, const char* path, const char* body,
                              const HttpHeaders& headers = initialize_headers()) {
  return post_json(port, path, body, headers);
}

inline std::string jsonrpc_request(int id, const char* method, const char* params) {
  return mcp::build_json_rpc_request(id, method ? method : "", params ? params : "");
}

inline std::string extract_session_id(const std::string& json_body) {
  mcp::json::Document doc;
  if (!doc.parse(json_body)) {
    return {};
  }

  auto root = doc.root();
  if (!root.has_member("result")) {
    return {};
  }

  auto result = root["result"];
  if (!result.has_member("sessionId")) {
    return {};
  }

  return std::string(result["sessionId"].get_string(""));
}

inline SessionContext perform_full_lifecycle(uint16_t port) {
  SessionContext ctx;
  ctx.port = port;

  const std::string initialize = mcp::build_json_rpc_request(
      1, "initialize",
      R"({"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"dmcp-test","version":"1.0"}})");
  const HttpResponse init = http_post(port, MCP_ENDPOINT_MCP, initialize.c_str());

  ctx.session_id = extract_session_id(init.body);

  const std::string initialized = mcp::build_json_rpc_notification("notifications/initialized");
  (void)http_post(port, MCP_ENDPOINT_MCP, initialized.c_str(), session_headers(ctx.session_id));

  return ctx;
}

inline std::string initialize_session(uint16_t port) {
  return perform_full_lifecycle(port).session_id;
}

inline bool jsonrpc_result(const std::string& body, mcp::json::Document* out_doc) {
  if (!out_doc || !out_doc->parse(body)) {
    return false;
  }
  auto root = out_doc->root();
  return root.has_member("result") && !root.has_member("error");
}

inline std::string tool_text_payload(const std::string& body) {
  mcp::json::Document doc;
  if (!jsonrpc_result(body, &doc)) {
    return {};
  }

  auto result  = doc.root()["result"];
  auto content = result["content"];
  if (!content.is_array() || content.size() == 0) {
    return {};
  }

  auto first = content[0];
  return std::string(first["text"].get_string(""));
}

}  // namespace dmcp::test
