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
