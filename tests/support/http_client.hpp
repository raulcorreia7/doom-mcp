#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <httplib.h>

#include "mcp/generic/constants.h"

namespace dmcp::test {

struct HttpResponse {
  int         status = 0;
  std::string body;
};

using HttpHeaders = std::vector<std::pair<std::string, std::string>>;

inline httplib::Headers to_httplib_headers(const HttpHeaders& headers) {
  httplib::Headers converted;
  for (const auto& header : headers) {
    converted.emplace(header.first, header.second);
  }
  return converted;
}

inline httplib::Client make_client(uint16_t port) {
  httplib::Client client("127.0.0.1", port);
  client.set_connection_timeout(2, 0);
  client.set_read_timeout(2, 0);
  client.set_write_timeout(2, 0);
  return client;
}

inline HttpResponse post_json(uint16_t port, const char* path, const char* body,
                              const HttpHeaders& headers) {
  HttpResponse response;
  auto         client = make_client(port);
  auto res = client.Post(path, to_httplib_headers(headers), body ? body : "", "application/json");
  if (!res) {
    return response;
  }

  response.status = res->status;
  response.body   = res->body;
  return response;
}

inline bool wait_for_health(uint16_t port, std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    auto client = make_client(port);
    auto res    = client.Get(MCP_ENDPOINT_HEALTH);
    if (res && res->status == 200) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
  return false;
}

}  // namespace dmcp::test
