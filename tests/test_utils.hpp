#pragma once
#include <catch2/catch_test_macros.hpp>

#include "dmcp/doom/api.h"
#include "dmcp/doom/commands.h"
#include "mcp/generic/server.h"

namespace dmcp::test {

constexpr int      TEST_PORT         = 16060;
constexpr int      TEST_SCREENSHOT_W = 640;
constexpr int      TEST_SCREENSHOT_H = 480;
constexpr uint32_t MAX_COMMAND_QUEUE = 64;

class ServerFixture {
 protected:
  mcp_server_t*       server = nullptr;
  mcp_server_config_t config{};

  ServerFixture() {
    config                 = mcp_server_config_default();
    config.port            = TEST_PORT;
    config.start_transport = false;
    server                 = mcp_server_create(&config);
    REQUIRE(server != nullptr);
  }

  ~ServerFixture() {
    if (server) mcp_server_destroy(server);
  }
};

class DoomContextFixture {
 protected:
  dmcp_context_t* ctx = nullptr;
  dmcp_config_t   config{};

  DoomContextFixture() {
    config                 = dmcp_config_default();
    config.port            = TEST_PORT;
    config.start_transport = false;
    ctx                    = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);
  }

  ~DoomContextFixture() {
    if (ctx) dmcp_context_destroy(ctx);
  }
};

inline dmcp_enemy_t make_test_enemy(int id = 1, float hp = 60.0f, const char* type = "DoomImp") {
  dmcp_enemy_t enemy{};
  enemy.id        = id;
  enemy.hp        = hp;
  enemy.max_hp    = hp * 2;
  enemy.position  = {100.0f * id, 200.0f, 0.0f};
  enemy.angle     = 0.0f;
  enemy.target_id = -1;
  mcp_strcpy_safe(enemy.type, sizeof(enemy.type), type);
  return enemy;
}

inline dmcp_item_t make_test_item(const char* name = "Clip", int amount = 50) {
  dmcp_item_t item{};
  mcp_strcpy_safe(item.name, sizeof(item.name), name);
  item.amount = amount;
  return item;
}

inline std::vector<uint8_t> make_test_screenshot_pixels(int w = TEST_SCREENSHOT_W,
                                                        int h = TEST_SCREENSHOT_H) {
  std::vector<uint8_t> pixels(w * h * 3, 128);
  return pixels;
}

}  // namespace dmcp::test
