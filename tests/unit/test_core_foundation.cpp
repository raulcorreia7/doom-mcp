#include <catch2/catch_test_macros.hpp>

#include "mcp/core/core.h"

TEST_CASE("MCP core string helpers compare ASCII case-insensitively", "[core][string]") {
  REQUIRE(mcp_strcmp_ci("debug", "DEBUG") == 0);
  REQUIRE(mcp_strcmp_ci("warn", "warning") < 0);
  REQUIRE(mcp_strcmp_ci("warning", "warn") > 0);
  REQUIRE(mcp_strcmp_ci("DoomImp", "doomimp") == 0);
  REQUIRE(mcp_strcmp_ci("DoomImp", "DoomImp") == 0);
}

TEST_CASE("MCP core string helpers define null ordering", "[core][string]") {
  REQUIRE(mcp_strcmp_ci(nullptr, nullptr) == 0);
  REQUIRE(mcp_strcmp_ci(nullptr, "value") < 0);
  REQUIRE(mcp_strcmp_ci("value", nullptr) > 0);
}

TEST_CASE("MCP core memory helper checks destination capacity", "[core][memory]") {
  char dest[4] = {};

  REQUIRE(mcp_memcpy_safe(dest, sizeof(dest), "abc", 4) == true);
  REQUIRE(dest[0] == 'a');
  REQUIRE(dest[3] == '\0');

  REQUIRE(mcp_memcpy_safe(dest, sizeof(dest), "overflow", 9) == false);
  REQUIRE(mcp_memcpy_safe(nullptr, sizeof(dest), "abc", 4) == false);
  REQUIRE(mcp_memcpy_safe(dest, sizeof(dest), nullptr, 4) == false);
  REQUIRE(mcp_memcpy_safe(nullptr, 0, nullptr, 0) == true);
}
