#pragma once

#include <chrono>
#include <cstdint>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "mcp/generic/server.h"

namespace dmcp::test {

#ifdef _WIN32
struct winsock_guard {
  winsock_guard() {
    WSADATA data{};
    WSAStartup(MAKEWORD(2, 2), &data);
  }

  ~winsock_guard() { WSACleanup(); }
};
#endif

inline void close_socket_platform(
#ifdef _WIN32
    SOCKET sock
#else
    int sock
#endif
) {
#ifdef _WIN32
  closesocket(sock);
#else
  close(sock);
#endif
}

inline uint16_t allocate_loopback_port() {
#ifdef _WIN32
  static winsock_guard guard;
  SOCKET               sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    return 0;
  }
#else
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    return 0;
  }
#endif

  sockaddr_in addr{};
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(0);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    close_socket_platform(sock);
    return 0;
  }

#ifdef _WIN32
  int len = sizeof(addr);
#else
  socklen_t len = sizeof(addr);
#endif
  if (getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
    close_socket_platform(sock);
    return 0;
  }

  close_socket_platform(sock);
  return ntohs(addr.sin_port);
}

inline bool wait_for_server_running(mcp_server_t* server, std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (mcp_server_is_running(server)) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return false;
}

}  // namespace dmcp::test
