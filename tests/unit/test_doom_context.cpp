#include <chrono>
#include <cstring>
#include <thread>

#include "dmcp/adapter/entities.h"
#include "dmcp/doom/api.h"
#include "dmcp/doom/content.h"
#include "dmcp/doom/commands.h"
#include "dmcp/doom/constants.h"
#include "dmcp/doom/types.h"
#include "mcp/generic/constants.h"
#include "support/network.hpp"
#include "test_utils.hpp"

static int              snapshot_call_count = 0;
static dmcp_snapshot_t* last_snapshot       = nullptr;

static void test_snapshot_callback(void* user_data, dmcp_snapshot_t* snapshot) {
  snapshot_call_count++;

  if (user_data) {
    int* counter = static_cast<int*>(user_data);
    (*counter)++;
  }

  last_snapshot = snapshot;

  snapshot->player.hp         = 100;
  snapshot->player.armor      = 50;
  snapshot->player.position.x = 10.0f;
  snapshot->player.position.y = 20.0f;
  snapshot->player.position.z = 0.0f;
  snapshot->player.ammo[0]    = 100;

  snapshot->level.tic = 12345;
  mcp_strcpy_safe(snapshot->level.level_id, DMCP_MAX_LEVEL_ID, "MAP01");
  mcp_strcpy_safe(snapshot->level.level_name, DMCP_MAX_LEVEL_NAME, "Entryway");
  snapshot->level.kill_count   = 5;
  snapshot->level.item_count   = 10;
  snapshot->level.secret_count = 2;
}

static dmcp_enemy_t create_test_enemy(int id, int hp, const char* type) {
  dmcp_enemy_t enemy;
  enemy.id         = id;
  enemy.hp         = hp;
  enemy.max_hp     = hp;
  enemy.position.x = (float)id * 10.0f;
  enemy.position.y = (float)id * 20.0f;
  enemy.position.z = 0.0f;
  enemy.angle      = 0.0f;
  enemy.target_id  = -1;
  mcp_strcpy_safe(enemy.type, DMCP_MAX_ENEMY_TYPE, type);
  return enemy;
}

static dmcp_item_t create_test_item(const char* name, int amount) {
  dmcp_item_t item;
  mcp_strcpy_safe(item.name, DMCP_MAX_ITEM_NAME, name);
  item.amount = amount;
  return item;
}

static dmcp_config_t TestConfig() {
  dmcp_config_t config   = dmcp_config_default();
  config.port            = dmcp::test::allocate_loopback_port();
  config.start_transport = false;
  return config;
}

TEST_CASE("Doom MCP: Context lifecycle", "[doom][context][lifecycle]") {
  SECTION("Create and destroy context with default config") {
    dmcp_config_t   config = TestConfig();
    dmcp_context_t* ctx    = dmcp_context_create(&config);

    REQUIRE(ctx != nullptr);
    REQUIRE(dmcp_context_is_running(ctx) == false);

    dmcp_context_destroy(ctx);
  }

  SECTION("Default config helper provides sane defaults") {
    dmcp_config_t config = dmcp_config_default();
    REQUIRE(config.struct_size == sizeof(dmcp_config_t));
    REQUIRE(config.port == MCP_DEFAULT_PORT);
    REQUIRE(config.target_hz == DMCP_DEFAULT_TARGET_HZ);
    REQUIRE(config.start_transport == true);
    REQUIRE(config.permissions.allow_console_commands == false);
    REQUIRE(config.permissions.allow_cheats == false);
  }

  SECTION("Create context with custom port") {
    dmcp_config_t config = TestConfig();
    config.port          = dmcp::test::allocate_loopback_port();

    dmcp_context_t* ctx = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);

    dmcp_context_destroy(ctx);
  }

  SECTION("Create context with custom target Hz") {
    dmcp_config_t config = TestConfig();
    config.target_hz     = 30;

    dmcp_context_t* ctx = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);

    dmcp_context_destroy(ctx);
  }

  SECTION("Create context rejects zero target Hz") {
    dmcp_config_t config = TestConfig();
    config.target_hz     = 0;

    dmcp_context_t* ctx = dmcp_context_create(&config);
    REQUIRE(ctx == nullptr);
  }

  SECTION("Create context with screenshot disabled") {
    dmcp_config_t config     = TestConfig();
    config.screenshot.enable = false;

    dmcp_context_t* ctx = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);

    dmcp_context_destroy(ctx);
  }

  SECTION("Create context with custom screenshot dimensions") {
    dmcp_config_t config     = TestConfig();
    config.screenshot.width  = 1280;
    config.screenshot.height = 720;

    dmcp_context_t* ctx = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);

    dmcp_context_destroy(ctx);
  }

  SECTION("Create context with snapshot callback") {
    snapshot_call_count = 0;

    dmcp_config_t config = TestConfig();
    config.on_snapshot   = test_snapshot_callback;

    dmcp_context_t* ctx = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);

    dmcp_context_destroy(ctx);
  }

  SECTION("Destroy null context is safe") { dmcp_context_destroy(nullptr); }

  SECTION("Is running with null context returns false") {
    REQUIRE(dmcp_context_is_running(nullptr) == false);
  }
}

TEST_CASE("Doom MCP: Game loop integration", "[doom][tick]") {
  snapshot_call_count = 0;

  dmcp_config_t config = TestConfig();
  config.on_snapshot   = test_snapshot_callback;

  dmcp_context_t* ctx = dmcp_context_create(&config);
  REQUIRE(ctx != nullptr);

  SECTION("Tick without snapshot callback is safe") {
    dmcp_config_t no_callback = TestConfig();
    no_callback.on_snapshot   = nullptr;

    dmcp_context_t* ctx_no_callback = dmcp_context_create(&no_callback);
    REQUIRE(ctx_no_callback != nullptr);

    dmcp_context_tick(ctx_no_callback);

    dmcp_context_destroy(ctx_no_callback);
  }

  SECTION("Tick with snapshot callback invokes callback") {
    snapshot_call_count = 0;
    dmcp_context_tick(ctx);

    REQUIRE(snapshot_call_count > 0);
  }

  SECTION("Multiple ticks invoke callback") {
    snapshot_call_count = 0;

    for (int i = 0; i < 10; i++) {
      dmcp_context_tick(ctx);
    }

    REQUIRE(snapshot_call_count >= 1);
  }

  SECTION("Tick with null context is safe") { dmcp_context_tick(nullptr); }

  dmcp_context_destroy(ctx);
}

TEST_CASE("Doom MCP: Screenshot functionality", "[doom][screenshot]") {
  dmcp_config_t config     = TestConfig();
  config.screenshot.enable = true;

  dmcp_context_t* ctx = dmcp_context_create(&config);
  REQUIRE(ctx != nullptr);

  SECTION("Check screenshot requested with null context returns false") {
    REQUIRE(dmcp_screenshot_is_requested(nullptr) == false);
  }

  SECTION("Submit screenshot with null context returns error") {
    dmcp_screenshot_frame_t frame  = {};
    mcp_status_t            result = dmcp_screenshot_submit(nullptr, &frame);

    REQUIRE(result.code != 0);
  }

  SECTION("Submit screenshot with null frame returns error") {
    mcp_status_t result = dmcp_screenshot_submit(ctx, nullptr);

    REQUIRE(result.code != 0);
  }

  SECTION("Submit valid screenshot succeeds") {
    uint8_t pixels[640 * 480 * 4];
    for (size_t i = 0; i < sizeof(pixels); i++) {
      pixels[i] = (uint8_t)(i % 256);
    }

    dmcp_screenshot_frame_t frame = {};
    frame.pixels                  = pixels;
    frame.width                   = 640;
    frame.height                  = 480;
    frame.stride                  = 640 * 4;

    mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);

    REQUIRE(result.code == 0);
  }

  SECTION("Screenshot request is consumed by submitted frame") {
    REQUIRE(dmcp_screenshot_is_requested(ctx) == false);
    REQUIRE(dmcp_screenshot_request(ctx) == true);
    REQUIRE(dmcp_screenshot_is_requested(ctx) == true);

    uint8_t pixels[2 * 2 * 4] = {};

    dmcp_screenshot_frame_t frame = {};
    frame.pixels                  = pixels;
    frame.width                   = 2;
    frame.height                  = 2;
    frame.stride                  = 2 * 4;

    REQUIRE(dmcp_screenshot_submit(ctx, &frame).code == MCP_STATUS_CODE_OK);
    REQUIRE(dmcp_screenshot_is_requested(ctx) == false);
  }

  SECTION("Copy ASCII screenshot uses caller buffer") {
    uint8_t pixels[4 * 4 * 4] = {};
    for (size_t i = 0; i < sizeof(pixels); ++i) {
      pixels[i] = static_cast<uint8_t>(i % 255);
    }

    dmcp_screenshot_frame_t frame = {};
    frame.pixels                  = pixels;
    frame.width                   = 4;
    frame.height                  = 4;
    frame.stride                  = 4 * 4;

    mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);
    REQUIRE(result.code == 0);

    char ascii[128] = {};
    int  written    = dmcp_screenshot_copy_ascii(ctx, ascii, sizeof(ascii), 4);
    REQUIRE(written > 0);
    REQUIRE(ascii[written] == '\0');
    REQUIRE(std::strlen(ascii) == static_cast<size_t>(written));

    char short_ascii_buffer[2] = {};
    REQUIRE(dmcp_screenshot_copy_ascii(ctx, short_ascii_buffer, sizeof(short_ascii_buffer), 16) ==
            -1);
  }

  SECTION("JSON screenshot reads stable dimensions") {
    uint8_t pixels[2 * 2 * 4] = {};

    dmcp_screenshot_frame_t frame = {};
    frame.pixels                  = pixels;
    frame.width                   = 2;
    frame.height                  = 2;
    frame.stride                  = 2 * 4;

    mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);
    REQUIRE(result.code == 0);

    char json[512] = {};
    int  written   = dmcp_screenshot_to_json(ctx, json, sizeof(json), 2);
    REQUIRE(written > 0);
    REQUIRE(std::strstr(json, "\"width\":2") != nullptr);
    REQUIRE(std::strstr(json, "\"height\":2") != nullptr);
    REQUIRE(std::strstr(json, "\"ascii\":") != nullptr);
  }

  SECTION("Submit screenshot with custom dimensions succeeds") {
    uint8_t pixels[1280 * 720 * 4];

    dmcp_screenshot_frame_t frame = {};
    frame.pixels                  = pixels;
    frame.width                   = 1280;
    frame.height                  = 720;
    frame.stride                  = 1280 * 4;

    mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);

    REQUIRE(result.code == 0);
  }

  SECTION("Submit screenshot with null pixels") {
    dmcp_screenshot_frame_t frame = {};
    frame.pixels                  = nullptr;
    frame.width                   = 640;
    frame.height                  = 480;
    frame.stride                  = 1920;

    mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);

    REQUIRE(result.code == MCP_STATUS_CODE_INVALID_ARGS);
  }

  SECTION("Submit screenshot with zero dimensions") {
    uint8_t                 pixels[1];
    dmcp_screenshot_frame_t frame = {};
    frame.pixels                  = pixels;
    frame.width                   = 0;
    frame.height                  = 0;
    frame.stride                  = 1;

    mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);

    REQUIRE(result.code == MCP_STATUS_CODE_INVALID_ARGS);
  }

  SECTION("Submit screenshot with short stride returns error") {
    uint8_t pixels[2 * 2 * 4] = {};

    dmcp_screenshot_frame_t frame = {};
    frame.pixels                  = pixels;
    frame.width                   = 2;
    frame.height                  = 2;
    frame.stride                  = 2;

    mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);

    REQUIRE(result.code == MCP_STATUS_CODE_INVALID_ARGS);
  }

  SECTION("Submit screenshot with disabled screenshot feature") {
    dmcp_config_t no_screenshot     = TestConfig();
    no_screenshot.screenshot.enable = false;

    dmcp_context_t* ctx_no_ss = dmcp_context_create(&no_screenshot);
    REQUIRE(ctx_no_ss != nullptr);

    uint8_t                 pixels[1];
    dmcp_screenshot_frame_t frame = {};
    frame.pixels                  = pixels;
    frame.width                   = 1;
    frame.height                  = 1;
    frame.stride                  = 1;

    mcp_status_t result = dmcp_screenshot_submit(ctx_no_ss, &frame);

    REQUIRE(result.code != 0);

    dmcp_context_destroy(ctx_no_ss);
  }

  dmcp_context_destroy(ctx);
}

TEST_CASE("Doom MCP: Statistics", "[doom][stats]") {
  dmcp_config_t config = TestConfig();

  dmcp_context_t* ctx = dmcp_context_create(&config);
  REQUIRE(ctx != nullptr);

  SECTION("Get stats returns valid structure") {
    dmcp_stats_t stats = {};
    dmcp_stats_get(ctx, &stats);

    REQUIRE(stats.struct_size == sizeof(dmcp_stats_t));
    REQUIRE(stats.dropped_snapshots == 0);
    REQUIRE(stats.dropped_screenshots == 0);
    REQUIRE(stats.connected_clients == 0);
  }

  SECTION("Get stats with null context zeros structure") {
    dmcp_stats_t stats      = {};
    stats.dropped_snapshots = 999;

    dmcp_stats_get(nullptr, &stats);
  }

  SECTION("Get stats with null stats pointer is safe") { dmcp_stats_get(ctx, nullptr); }

  dmcp_context_destroy(ctx);
}

TEST_CASE("Doom MCP: Snapshot utilities", "[doom][snapshot]") {
  SECTION("Clear snapshot zeros all fields") {
    dmcp_snapshot_t snapshot = {};
    snapshot.player.hp       = 100;
    snapshot.enemy_count     = 5;

    dmcp_snapshot_clear(&snapshot);

    REQUIRE(snapshot.player.hp == 0);
    REQUIRE(snapshot.player.armor == 0);
    REQUIRE(snapshot.player.position.x == 0.0f);
    REQUIRE(snapshot.player.position.y == 0.0f);
    REQUIRE(snapshot.player.ammo[0] == 0);
    REQUIRE(snapshot.level.tic == 0);
    REQUIRE(snapshot.level.kill_count == 0);
    REQUIRE(snapshot.enemy_count == 0);
    REQUIRE(snapshot.inventory_count == 0);
  }

  SECTION("Clear snapshot with null pointer is safe") { dmcp_snapshot_clear(nullptr); }

  SECTION("Add enemy to empty snapshot succeeds") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    dmcp_enemy_t enemy = create_test_enemy(1, 60, "DoomImp");

    bool result = dmcp_snapshot_add_enemy(&snapshot, &enemy);

    REQUIRE(result == true);
    REQUIRE(snapshot.enemy_count == 1);
    REQUIRE(snapshot.enemies[0].id == 1);
    REQUIRE(snapshot.enemies[0].hp == 60);
    REQUIRE(std::strcmp(snapshot.enemies[0].type, "DoomImp") == 0);
  }

  SECTION("Add multiple enemies to snapshot") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    dmcp_enemy_t enemy1 = create_test_enemy(1, 60, "DoomImp");
    dmcp_enemy_t enemy2 = create_test_enemy(2, 150, "Demon");
    dmcp_enemy_t enemy3 = create_test_enemy(3, 500, "Baron");
    REQUIRE(dmcp_snapshot_add_enemy(&snapshot, &enemy1));
    REQUIRE(dmcp_snapshot_add_enemy(&snapshot, &enemy2));
    REQUIRE(dmcp_snapshot_add_enemy(&snapshot, &enemy3));

    REQUIRE(snapshot.enemy_count == 3);
  }

  SECTION("Add enemy beyond max limit fails") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    for (uint32_t i = 0; i < DMCP_MAX_ENEMIES; i++) {
      dmcp_enemy_t enemy = create_test_enemy((int)i, 60.0f, "DoomImp");
      dmcp_snapshot_add_enemy(&snapshot, &enemy);
    }

    dmcp_enemy_t extra_enemy = create_test_enemy(999, 60, "DoomImp");
    bool         result      = dmcp_snapshot_add_enemy(&snapshot, &extra_enemy);

    REQUIRE(result == false);
    REQUIRE(snapshot.enemy_count == DMCP_MAX_ENEMIES);
  }

  SECTION("Add enemy with null snapshot fails") {
    dmcp_enemy_t enemy  = create_test_enemy(1, 60, "DoomImp");
    bool         result = dmcp_snapshot_add_enemy(nullptr, &enemy);

    REQUIRE(result == false);
  }

  SECTION("Add enemy with null enemy fails") {
    dmcp_snapshot_t snapshot = {};
    bool            result   = dmcp_snapshot_add_enemy(&snapshot, nullptr);

    REQUIRE(result == false);
  }

  SECTION("Add item to empty inventory succeeds") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    dmcp_item_t item = create_test_item("Clip", 50);

    bool result = dmcp_snapshot_add_item(&snapshot, &item);

    REQUIRE(result == true);
    REQUIRE(snapshot.inventory_count == 1);
    REQUIRE(std::strcmp(snapshot.inventory[0].name, "Clip") == 0);
    REQUIRE(snapshot.inventory[0].amount == 50);
  }

  SECTION("Add multiple items to inventory") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    dmcp_item_t item1 = create_test_item("Clip", 50);
    dmcp_item_t item2 = create_test_item("Shell", 20);
    dmcp_item_t item3 = create_test_item("Rocket", 5);
    REQUIRE(dmcp_snapshot_add_item(&snapshot, &item1));
    REQUIRE(dmcp_snapshot_add_item(&snapshot, &item2));
    REQUIRE(dmcp_snapshot_add_item(&snapshot, &item3));

    REQUIRE(snapshot.inventory_count == 3);
  }

  SECTION("Add item beyond max limit fails") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    for (uint32_t i = 0; i < DMCP_MAX_INVENTORY; i++) {
      dmcp_item_t item = create_test_item("Item", 1);
      dmcp_snapshot_add_item(&snapshot, &item);
    }

    dmcp_item_t extra_item = create_test_item("Extra", 1);
    bool        result     = dmcp_snapshot_add_item(&snapshot, &extra_item);

    REQUIRE(result == false);
    REQUIRE(snapshot.inventory_count == DMCP_MAX_INVENTORY);
  }

  SECTION("Add item with null snapshot fails") {
    dmcp_item_t item   = create_test_item("Clip", 50);
    bool        result = dmcp_snapshot_add_item(nullptr, &item);

    REQUIRE(result == false);
  }

  SECTION("Add item with null item fails") {
    dmcp_snapshot_t snapshot = {};
    bool            result   = dmcp_snapshot_add_item(&snapshot, nullptr);

    REQUIRE(result == false);
  }
}

TEST_CASE("Doom MCP: String utilities", "[doom][string]") {
  SECTION("Copy string to buffer succeeds") {
    char dest[64];
    bool copied = mcp_strcpy_safe(dest, 64, "Hello, World!");

    REQUIRE(copied == true);
    REQUIRE(std::strcmp(dest, "Hello, World!") == 0);
  }

  SECTION("Copy string with exact buffer size null-terminates") {
    char dest[14];
    bool copied = mcp_strcpy_safe(dest, 14, "Hello, World!");

    REQUIRE(copied == true);
    REQUIRE(std::strlen(dest) == 13);
    REQUIRE(std::strcmp(dest, "Hello, World!") == 0);
  }

  SECTION("Copy string truncates if too long") {
    char dest[6];
    bool copied = mcp_strcpy_safe(dest, 6, "Hello, World!");

    REQUIRE(copied == false);
    REQUIRE(std::strcmp(dest, "Hello") == 0);
    REQUIRE(dest[5] == '\0');
  }

  SECTION("Copy string with null destination is safe") {
    REQUIRE(mcp_strcpy_safe(nullptr, 10, "test") == false);
  }

  SECTION("Copy string with null source is safe") {
    char dest[10] = "original";
    bool copied   = mcp_strcpy_safe(dest, 10, nullptr);

    REQUIRE(copied == false);
    REQUIRE(dest[0] == '\0');
  }

  SECTION("Copy string with zero buffer size is safe") {
    char dest[10];
    REQUIRE(mcp_strcpy_safe(dest, 0, "test") == false);
  }

  SECTION("Copy empty string") {
    char dest[10] = "original";
    bool copied   = mcp_strcpy_safe(dest, 10, "");

    REQUIRE(copied == true);
    REQUIRE(dest[0] == '\0');
  }
}

TEST_CASE("Doom MCP: Configuration", "[doom][config]") {
  SECTION("Default config has correct values") {
    dmcp_config_t config = dmcp_config_default();

    REQUIRE(config.struct_size == sizeof(dmcp_config_t));
    REQUIRE(config.port == MCP_DEFAULT_PORT);
    REQUIRE(config.target_hz == DMCP_DEFAULT_TARGET_HZ);
    REQUIRE(config.snapshot_pool_size == DMCP_DEFAULT_SNAPSHOT_POOL_SIZE);
    REQUIRE(config.command_queue_slots == DMCP_DEFAULT_QUEUE_SLOTS);
    REQUIRE(config.screenshot.enable == false);
    REQUIRE(config.screenshot.width == DMCP_DEFAULT_SCREENSHOT_WIDTH);
    REQUIRE(config.screenshot.height == DMCP_DEFAULT_SCREENSHOT_HEIGHT);
    REQUIRE(config.on_snapshot == nullptr);
    REQUIRE(config.on_log == nullptr);
    REQUIRE(config.user_data == nullptr);
    REQUIRE(config.start_transport == true);
  }

  SECTION("Custom config values") {
    dmcp_config_t config       = TestConfig();
    config.target_hz           = 30;
    config.snapshot_pool_size  = 32;
    config.command_queue_slots = 8;
    config.screenshot.width    = 1920;
    config.screenshot.height   = 1080;

    dmcp_context_t* ctx = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);

    dmcp_context_destroy(ctx);
  }
}

TEST_CASE("Doom MCP: Result codes", "[doom][result]") {
  SECTION("Result code constants") {
    REQUIRE(MCP_STATUS_CODE_OK == 0);
    REQUIRE(MCP_STATUS_CODE_INVALID_ARGS == -1);
    REQUIRE(MCP_STATUS_CODE_ENCODING_FAILED == -2);
    REQUIRE(MCP_STATUS_CODE_DISABLED == -3);
    REQUIRE(MCP_STATUS_CODE_QUEUE_FULL == -4);
  }

  SECTION("Convenience macros create valid results") {
    mcp_status_t ok = MCP_STATUS_OK("Success");
    REQUIRE(ok.code == 0);
    REQUIRE(ok.message != nullptr);

    mcp_status_t error = MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
    REQUIRE(error.code == -1);
    REQUIRE(error.message != nullptr);
  }

  SECTION("Result macros with custom message") {
    mcp_status_t result = MCP_STATUS_ERROR(-100, "Custom error");
    REQUIRE(result.code == -100);
    REQUIRE(std::strcmp(result.message, "Custom error") == 0);
  }

  SECTION("Result macro with null message") {
    mcp_status_t result = MCP_STATUS_ERROR(-200, nullptr);
    REQUIRE(result.code == -200);
    REQUIRE(result.message != nullptr);
  }
}

TEST_CASE("Doom MCP: Constants", "[doom][constants]") {
  SECTION("Max enemies limit") { REQUIRE(DMCP_MAX_ENEMIES == 1024); }

  SECTION("Max inventory limit") { REQUIRE(DMCP_MAX_INVENTORY == 64); }

  SECTION("Max item name length") { REQUIRE(DMCP_MAX_ITEM_NAME == 64); }

  SECTION("Max enemy type length") { REQUIRE(DMCP_MAX_ENEMY_TYPE == 128); }

  SECTION("Max level ID length") { REQUIRE(DMCP_MAX_LEVEL_ID == 32); }

  SECTION("Max level name length") { REQUIRE(DMCP_MAX_LEVEL_NAME == 96); }
}

TEST_CASE("Doom MCP: Content availability by mode", "[doom][content]") {
  SECTION("Shareware blocks entities not present in episode 1") {
    REQUIRE(dmcp_is_enemy_spawnable("DoomImp", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE(dmcp_is_enemy_spawnable("BaronOfHell", DMCP_GAMEMODE_SHAREWARE));

    REQUIRE_FALSE(dmcp_is_enemy_spawnable("Cacodemon", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("LostSoul", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("HellKnight", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("ChaingunGuy", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("BossBrain", DMCP_GAMEMODE_SHAREWARE));
  }

  SECTION("Registered and retail block Doom 2-only enemies") {
    REQUIRE(dmcp_is_enemy_spawnable("Cacodemon", DMCP_GAMEMODE_REGISTERED));
    REQUIRE(dmcp_is_enemy_spawnable("LostSoul", DMCP_GAMEMODE_RETAIL));

    REQUIRE_FALSE(dmcp_is_enemy_spawnable("HellKnight", DMCP_GAMEMODE_REGISTERED));
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("Arachnotron", DMCP_GAMEMODE_REGISTERED));
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("Archvile", DMCP_GAMEMODE_RETAIL));
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("BossBrain", DMCP_GAMEMODE_RETAIL));
  }

  SECTION("Commercial mode allows Doom 2-only enemies") {
    REQUIRE(dmcp_is_enemy_spawnable("HellKnight", DMCP_GAMEMODE_COMMERCIAL));
    REQUIRE(dmcp_is_enemy_spawnable("Arachnotron", DMCP_GAMEMODE_COMMERCIAL));
    REQUIRE(dmcp_is_enemy_spawnable("Archvile", DMCP_GAMEMODE_COMMERCIAL));
    REQUIRE(dmcp_is_enemy_spawnable("BossBrain", DMCP_GAMEMODE_COMMERCIAL));
  }

  SECTION("Shareware blocks restricted items and maps") {
    REQUIRE(dmcp_is_item_available("Shells", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE_FALSE(dmcp_is_item_available("Cell", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE_FALSE(dmcp_is_item_available("CellPack", DMCP_GAMEMODE_SHAREWARE));

    REQUIRE(dmcp_is_map_available("E1M8", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE_FALSE(dmcp_is_map_available("E2M1", DMCP_GAMEMODE_SHAREWARE));
  }

  SECTION("Doom 1 modes block Doom 2-only content") {
    REQUIRE_FALSE(dmcp_is_weapon_available("SuperShotgun", DMCP_GAMEMODE_REGISTERED));
    REQUIRE_FALSE(dmcp_is_item_available("SuperShotgun", DMCP_GAMEMODE_REGISTERED));
    REQUIRE_FALSE(dmcp_is_item_available("MegaSphere", DMCP_GAMEMODE_REGISTERED));
    REQUIRE_FALSE(dmcp_is_item_available("MegaSphere", DMCP_GAMEMODE_RETAIL));
    REQUIRE_FALSE(dmcp_is_map_available("MAP01", DMCP_GAMEMODE_RETAIL));

    REQUIRE(dmcp_is_weapon_available("Shotgun", DMCP_GAMEMODE_REGISTERED));
    REQUIRE(dmcp_is_item_available("Shells", DMCP_GAMEMODE_RETAIL));
    REQUIRE(dmcp_is_map_available("E1M1", DMCP_GAMEMODE_REGISTERED));
    REQUIRE(dmcp_is_map_available("E4M1", DMCP_GAMEMODE_RETAIL));
  }

  SECTION("Commercial mode blocks Doom 1 map names") {
    REQUIRE(dmcp_is_map_available("MAP01", DMCP_GAMEMODE_COMMERCIAL));
    REQUIRE(dmcp_is_item_available("MegaSphere", DMCP_GAMEMODE_COMMERCIAL));
    REQUIRE_FALSE(dmcp_is_map_available("E1M1", DMCP_GAMEMODE_COMMERCIAL));
  }

  SECTION("Doom keys are discoverable items in all Doom modes") {
    REQUIRE(dmcp_is_item_available("BlueKeycard", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE(dmcp_is_item_available("YellowKeycard", DMCP_GAMEMODE_REGISTERED));
    REQUIRE(dmcp_is_item_available("RedKeycard", DMCP_GAMEMODE_RETAIL));
    REQUIRE(dmcp_is_item_available("BlueSkullKey", DMCP_GAMEMODE_COMMERCIAL));
    REQUIRE(dmcp_is_item_available("YellowSkullKey", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE(dmcp_is_item_available("RedSkullKey", DMCP_GAMEMODE_REGISTERED));
  }

  SECTION("Adapter monster helper accepts canonical catalog names") {
    for (size_t i = 0; i < dmcp_all_enemies_count; ++i) {
      REQUIRE(dmcp_entity_is_monster(dmcp_all_enemies[i]));
    }
    REQUIRE_FALSE(dmcp_entity_is_monster("Fatso"));
  }

  SECTION("Unknown content is rejected until exposed by the adapter catalog") {
    REQUIRE_FALSE(dmcp_is_weapon_available("CustomLaser", DMCP_GAMEMODE_COMMERCIAL));
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("CustomBoss", DMCP_GAMEMODE_RETAIL));
    REQUIRE_FALSE(dmcp_is_item_available("ModOnlyArtifact", DMCP_GAMEMODE_REGISTERED));
    REQUIRE_FALSE(dmcp_is_map_available("MAP99", DMCP_GAMEMODE_COMMERCIAL));
  }

  SECTION("Non-canonical content names are not accepted") {
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("Imp", DMCP_GAMEMODE_SHAREWARE));
    REQUIRE_FALSE(dmcp_is_enemy_spawnable("Hell Knight", DMCP_GAMEMODE_REGISTERED));
    REQUIRE_FALSE(dmcp_is_weapon_available("BFG", DMCP_GAMEMODE_COMMERCIAL));
    REQUIRE_FALSE(dmcp_is_item_available("Cell Pack", DMCP_GAMEMODE_COMMERCIAL));
  }

  SECTION("Unavailable message can be copied into caller storage") {
    char buffer[128] = {};
    int written = dmcp_content_unavailable_message_copy("Item", "CellPack", DMCP_GAMEMODE_SHAREWARE,
                                                        buffer, sizeof(buffer));

    REQUIRE(written > 0);
    REQUIRE(std::strstr(buffer, "Item 'CellPack' not available in shareware mode") != nullptr);
    REQUIRE(buffer[written] == '\0');

    char short_message_buffer[4] = {};
    REQUIRE(dmcp_content_unavailable_message_copy("Item", "CellPack", DMCP_GAMEMODE_SHAREWARE,
                                                  short_message_buffer,
                                                  sizeof(short_message_buffer)) == -1);
  }
}

TEST_CASE("Doom MCP: Snapshot to JSON", "[doom][json]") {
  SECTION("Convert valid snapshot to JSON") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    snapshot.player.hp         = 100;
    snapshot.player.armor      = 50;
    snapshot.player.position.x = 10.0f;
    snapshot.player.position.y = 20.0f;
    snapshot.player.position.z = 0.0f;
    snapshot.player.ammo[0]    = 100;

    snapshot.level.tic = 12345;
    mcp_strcpy_safe(snapshot.level.level_id, DMCP_MAX_LEVEL_ID, "MAP01");
    mcp_strcpy_safe(snapshot.level.level_name, DMCP_MAX_LEVEL_NAME, "Entryway");
    snapshot.level.kill_count = 5;

    dmcp_enemy_t enemy = create_test_enemy(1, 60, "DoomImp");
    dmcp_snapshot_add_enemy(&snapshot, &enemy);

    char buffer[MCP_MAX_JSON_SIZE];
    int  result = dmcp_snapshot_to_json(&snapshot, buffer, sizeof(buffer));

    REQUIRE(result > 0);
    REQUIRE(std::string(buffer).find("\"hp\":") != std::string::npos);
    REQUIRE(std::string(buffer).find("\"armor\":") != std::string::npos);
    REQUIRE(std::string(buffer).find("\"position\":") != std::string::npos);
  }

  SECTION("Convert snapshot with null snapshot returns error") {
    char buffer[MCP_MAX_JSON_SIZE];
    int  result = dmcp_snapshot_to_json(nullptr, buffer, sizeof(buffer));

    REQUIRE(result == -1);
  }

  SECTION("Convert snapshot with null buffer returns error") {
    dmcp_snapshot_t snapshot = {};
    int             result   = dmcp_snapshot_to_json(&snapshot, nullptr, MCP_MAX_JSON_SIZE);

    REQUIRE(result == -1);
  }

  SECTION("Convert snapshot with small buffer truncates") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    char buffer[10];
    int  result = dmcp_snapshot_to_json(&snapshot, buffer, sizeof(buffer));

    REQUIRE((result == -1 || result < (int)sizeof(buffer)));
  }

  SECTION("Convert empty snapshot to JSON") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    char buffer[MCP_MAX_JSON_SIZE];
    int  result = dmcp_snapshot_to_json(&snapshot, buffer, sizeof(buffer));

    REQUIRE(result > 0);
  }

  SECTION("Convert snapshot with many enemies to JSON") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    for (int i = 0; i < 10; i++) {
      dmcp_enemy_t enemy = create_test_enemy(i, 60.0f, "DoomImp");
      dmcp_snapshot_add_enemy(&snapshot, &enemy);
    }

    char buffer[MCP_MAX_JSON_SIZE];
    int  result = dmcp_snapshot_to_json(&snapshot, buffer, sizeof(buffer));

    REQUIRE(result > 0);
  }

  SECTION("Convert snapshot with inventory to JSON") {
    dmcp_snapshot_t snapshot = {};
    dmcp_snapshot_clear(&snapshot);

    dmcp_item_t clip_item  = create_test_item("Clip", 50);
    dmcp_item_t shell_item = create_test_item("Shell", 20);
    dmcp_snapshot_add_item(&snapshot, &clip_item);
    dmcp_snapshot_add_item(&snapshot, &shell_item);

    char buffer[MCP_MAX_JSON_SIZE];
    int  result = dmcp_snapshot_to_json(&snapshot, buffer, sizeof(buffer));

    REQUIRE(result > 0);
    REQUIRE(std::string(buffer).find("\"inventory\":") != std::string::npos);
  }
}

TEST_CASE("Doom MCP: Error messages", "[doom][error]") {
  SECTION("Invalid args error message is descriptive") {
    mcp_status_t result = MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");

    REQUIRE(result.code == MCP_STATUS_CODE_INVALID_ARGS);
    REQUIRE(result.message != nullptr);
    REQUIRE(std::strlen(result.message) > 0);
  }

  SECTION("Encoding failed error message is descriptive") {
    mcp_status_t result = MCP_STATUS_ERROR(MCP_STATUS_CODE_ENCODING_FAILED, "Encoding failed");

    REQUIRE(result.code == MCP_STATUS_CODE_ENCODING_FAILED);
    REQUIRE(result.message != nullptr);
    REQUIRE(std::strlen(result.message) > 0);
  }

  SECTION("Success message is present") {
    mcp_status_t result = MCP_STATUS_OK("Success");

    REQUIRE(result.code == MCP_STATUS_CODE_OK);
    REQUIRE(result.message != nullptr);
  }
}

TEST_CASE("Doom MCP: Command queue lifecycle", "[doom][commands]") {
  dmcp_config_t   config = TestConfig();
  dmcp_context_t* ctx    = dmcp_context_create(&config);
  REQUIRE(ctx != nullptr);

  SECTION("Empty queue returns no commands") {
    REQUIRE(dmcp_has_commands(ctx) == false);
    REQUIRE(dmcp_command_count(ctx) == 0);
  }

  SECTION("Clear empty queue is safe") {
    dmcp_clear_commands(ctx);
    REQUIRE(dmcp_command_count(ctx) == 0);
  }

  dmcp_context_destroy(ctx);
}

TEST_CASE("Doom MCP: Command queue push/pop", "[doom][commands]") {
  dmcp_config_t   config = TestConfig();
  dmcp_context_t* ctx    = dmcp_context_create(&config);
  REQUIRE(ctx != nullptr);

  dmcp_command_t cmd = {};
  cmd.type           = DMCP_CMD_PAUSE_GAME;
  cmd.sequence       = 12345;

  SECTION("Push command succeeds") {
    mcp_status_t result = dmcp_push_command(ctx, &cmd);
    REQUIRE(result.code == MCP_STATUS_CODE_OK);
    REQUIRE(dmcp_has_commands(ctx) == true);
    REQUIRE(dmcp_command_count(ctx) == 1);
  }

  SECTION("Push and pop roundtrip") {
    dmcp_push_command(ctx, &cmd);

    dmcp_command_t popped = {};
    bool           result = dmcp_pop_command(ctx, &popped);
    REQUIRE(result == true);
    REQUIRE(popped.type == DMCP_CMD_PAUSE_GAME);
    REQUIRE(popped.sequence >= 1);
    REQUIRE(dmcp_has_commands(ctx) == false);
  }

  SECTION("Pop from empty queue returns error") {
    dmcp_command_t popped = {};
    bool           result = dmcp_pop_command(ctx, &popped);
    REQUIRE(result == false);
  }

  SECTION("Clear commands removes all") {
    cmd.type = DMCP_CMD_GIVE_ITEM;
    for (int i = 0; i < 5; i++) {
      cmd.sequence = i;
      dmcp_push_command(ctx, &cmd);
    }
    REQUIRE(dmcp_command_count(ctx) == 5);

    dmcp_clear_commands(ctx);
    REQUIRE(dmcp_command_count(ctx) == 0);
  }

  dmcp_context_destroy(ctx);
}

TEST_CASE("Doom MCP: Privileged command permissions", "[doom][commands][permissions]") {
  SECTION("Default config rejects raw console and cheat-style mutation") {
    dmcp_config_t   config = TestConfig();
    dmcp_context_t* ctx    = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);

    dmcp_command_t console{};
    console.type = DMCP_CMD_EXECUTE_CONSOLE;
    mcp_strcpy_safe(console.data.console.command, sizeof(console.data.console.command), "iddqd");
    REQUIRE(dmcp_push_command(ctx, &console).code == MCP_STATUS_CODE_DISABLED);

    dmcp_command_t health{};
    health.type                   = DMCP_CMD_SET_PLAYER_HEALTH;
    health.data.set_health.health = 200;
    REQUIRE(dmcp_push_command(ctx, &health).code == MCP_STATUS_CODE_DISABLED);

    dmcp_context_destroy(ctx);
  }

  SECTION("Console access alone does not allow known cheat commands") {
    dmcp_config_t config                      = TestConfig();
    config.permissions.allow_console_commands = true;

    dmcp_context_t* ctx = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);

    dmcp_command_t console{};
    console.type = DMCP_CMD_EXECUTE_CONSOLE;
    mcp_strcpy_safe(console.data.console.command, sizeof(console.data.console.command), "iddqd");
    REQUIRE(dmcp_push_command(ctx, &console).code == MCP_STATUS_CODE_DISABLED);

    dmcp_context_destroy(ctx);
  }

  SECTION("Explicit permission flags allow privileged commands") {
    dmcp_config_t config                      = TestConfig();
    config.permissions.allow_console_commands = true;
    config.permissions.allow_cheats           = true;

    dmcp_context_t* ctx = dmcp_context_create(&config);
    REQUIRE(ctx != nullptr);

    dmcp_command_t console{};
    console.type = DMCP_CMD_EXECUTE_CONSOLE;
    mcp_strcpy_safe(console.data.console.command, sizeof(console.data.console.command), "iddqd");
    REQUIRE(dmcp_push_command(ctx, &console).code == MCP_STATUS_CODE_OK);

    dmcp_command_t health{};
    health.type                   = DMCP_CMD_SET_PLAYER_HEALTH;
    health.data.set_health.health = 200;
    REQUIRE(dmcp_push_command(ctx, &health).code == MCP_STATUS_CODE_OK);

    dmcp_context_destroy(ctx);
  }
}

TEST_CASE("Doom MCP: Command queue overflow", "[doom][commands]") {
  dmcp_config_t config       = TestConfig();
  config.command_queue_slots = 64;
  dmcp_context_t* ctx        = dmcp_context_create(&config);
  REQUIRE(ctx != nullptr);

  dmcp_command_t cmd = {};
  cmd.type           = DMCP_CMD_GIVE_ITEM;

  SECTION("Queue respects max size") {
    int pushed = 0;
    for (int i = 0; i < 100; i++) {
      cmd.sequence        = i;
      mcp_status_t result = dmcp_push_command(ctx, &cmd);
      if (result.code == MCP_STATUS_CODE_OK) {
        pushed++;
      } else {
        break;
      }
    }
    REQUIRE(pushed == 100);
    REQUIRE(dmcp_command_count(ctx) == 64);
  }

  SECTION("Overflow drops oldest queued command") {
    dmcp_command_result_t result = {};
    for (int i = 0; i < 70; i++) {
      cmd.sequence = i;
      REQUIRE(dmcp_push_command(ctx, &cmd).code == MCP_STATUS_CODE_OK);
    }

    REQUIRE(dmcp_command_result_get(ctx, 1, &result).code == MCP_STATUS_CODE_OK);
    REQUIRE(result.completed == true);
    REQUIRE(result.success == false);
    REQUIRE(std::strcmp(result.message, "Dropped due to command queue overflow") == 0);

    dmcp_command_t popped = {};
    REQUIRE(dmcp_pop_command(ctx, &popped) == true);
    REQUIRE(popped.sequence == 7);
  }

  dmcp_context_destroy(ctx);
}

TEST_CASE("Doom MCP: Command queue capacity follows config", "[doom][commands]") {
  dmcp_config_t config       = TestConfig();
  config.command_queue_slots = 3;

  dmcp_context_t* ctx = dmcp_context_create(&config);
  REQUIRE(ctx != nullptr);

  dmcp_command_t cmd = {};
  cmd.type           = DMCP_CMD_GIVE_ITEM;

  for (int i = 0; i < 5; ++i) {
    REQUIRE(dmcp_push_command(ctx, &cmd).code == MCP_STATUS_CODE_OK);
  }

  REQUIRE(dmcp_command_count(ctx) == 3);

  dmcp_command_t popped = {};
  REQUIRE(dmcp_pop_command(ctx, &popped) == true);
  REQUIRE(popped.sequence == 3);

  dmcp_context_destroy(ctx);
}

TEST_CASE("Doom MCP: Command result tracking", "[doom][commands]") {
  dmcp_config_t   config = TestConfig();
  dmcp_context_t* ctx    = dmcp_context_create(&config);
  REQUIRE(ctx != nullptr);

  dmcp_command_t cmd    = {};
  cmd.type              = DMCP_CMD_PAUSE_GAME;
  cmd.data.pause.paused = true;

  SECTION("Track queued and completed command by sequence") {
    mcp_status_t push_result = dmcp_push_command(ctx, &cmd);
    REQUIRE(push_result.code == MCP_STATUS_CODE_OK);
    REQUIRE(cmd.sequence > 0);

    dmcp_command_result_t result = {};
    REQUIRE(dmcp_command_result_get(ctx, cmd.sequence, &result).code == MCP_STATUS_CODE_NOT_FOUND);

    REQUIRE(dmcp_command_result_mark_queued(ctx, &cmd).code == MCP_STATUS_CODE_OK);
    REQUIRE(dmcp_command_result_get(ctx, cmd.sequence, &result).code == MCP_STATUS_CODE_OK);
    REQUIRE(result.sequence == cmd.sequence);
    REQUIRE(result.command_type == DMCP_CMD_PAUSE_GAME);
    REQUIRE(result.completed == false);
    REQUIRE(result.success == false);

    REQUIRE(dmcp_command_result_complete(ctx, &cmd, true, "Command executed").code ==
            MCP_STATUS_CODE_OK);
    REQUIRE(dmcp_command_result_get(ctx, cmd.sequence, &result).code == MCP_STATUS_CODE_OK);
    REQUIRE(result.completed == true);
    REQUIRE(result.success == true);
    REQUIRE(std::strcmp(result.message, "Command executed") == 0);
  }

  SECTION("Reject invalid sequence") {
    dmcp_command_result_t result = {};
    REQUIRE(dmcp_command_result_get(ctx, 0, &result).code == MCP_STATUS_CODE_INVALID_ARGS);
  }

  dmcp_context_destroy(ctx);
}

TEST_CASE("Doom MCP: JSON command parsing", "[doom][commands]") {
  dmcp_config_t   config = TestConfig();
  dmcp_context_t* ctx    = dmcp_context_create(&config);
  REQUIRE(ctx != nullptr);

  auto parse_ok = [](const char* json, dmcp_command_t* cmd) {
    mcp_status_t result = dmcp_parse_command_json(json, cmd);
    REQUIRE(result.code == MCP_STATUS_CODE_OK);
  };

  auto parse_invalid = [](const char* json) {
    dmcp_command_t cmd    = {};
    mcp_status_t   result = dmcp_parse_command_json(json, &cmd);
    REQUIRE(result.code == MCP_STATUS_CODE_INVALID_ARGS);
  };

  SECTION("Parse spawn_entity canonical fields") {
    dmcp_command_t cmd = {};
    parse_ok(
        R"({"type":"spawn_entity","params":{"entity_class":"Zombieman","x":64,"y":128,"angle":90,"tid":7}})",
        &cmd);
    REQUIRE(cmd.type == DMCP_CMD_SPAWN_ENTITY);
    REQUIRE(std::strcmp(cmd.data.spawn.entity_class, "Zombieman") == 0);
    REQUIRE(cmd.data.spawn.position.x == 64.0f);
    REQUIRE(cmd.data.spawn.position.y == 128.0f);
    REQUIRE(cmd.data.spawn.angle == 90.0f);
    REQUIRE(cmd.data.spawn.tid == 7);
  }

  SECTION("Reject spawn_entity with missing required fields") {
    parse_invalid(R"({"type":"spawn_entity","x":0,"y":0})");
    parse_invalid(R"({"type":"spawn_entity","params":{"entity":"Zombieman","x":0,"y":0}})");
    parse_invalid(R"({"type":"spawn_entity","entity_class":"Imp","x":"bad","y":0})");
    parse_invalid(
        R"({"type":"spawn_entity","params":{"entity_class":"DoomImp","x":160,"y":96,"count":4}})");
    parse_invalid(
        R"({"type":"spawn_entity","params":{"entity_class":"DoomImp","x":160,"y":96,"radius":96}})");
  }

  SECTION("Parse change_level and validate fields") {
    dmcp_command_t cmd = {};
    parse_ok(
        R"({"type":"change_level","params":{"map_name":"E1M2","skill_level":4,"reset_inventory":true}})",
        &cmd);
    REQUIRE(cmd.type == DMCP_CMD_CHANGE_LEVEL);
    REQUIRE(std::strcmp(cmd.data.change_level.map_name, "E1M2") == 0);
    REQUIRE(cmd.data.change_level.skill_level == 4);
    REQUIRE(cmd.data.change_level.reset_inventory == true);
  }

  SECTION("Parse change_level with lowercase map name normalizes to uppercase") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"change_level","params":{"map_name":"e1m4"}})", &cmd);
    REQUIRE(cmd.type == DMCP_CMD_CHANGE_LEVEL);
    REQUIRE(std::strcmp(cmd.data.change_level.map_name, "E1M4") == 0);

    parse_ok(R"({"type":"change_level","params":{"map_name":"map01"}})", &cmd);
    REQUIRE(std::strcmp(cmd.data.change_level.map_name, "MAP01") == 0);
  }

  SECTION("Reject invalid change_level payload") {
    parse_invalid(R"({"type":"change_level","params":{"level":"E1M1"}})");
    parse_invalid(R"({"type":"change_level","params":{"map_name":"BAD"}})");
    parse_invalid(R"({"type":"change_level","params":{"map_name":"E1M1","skill_level":7}})");
    parse_invalid(R"({"type":"change_level","params":{"map_name":"E1M1","episode":1}})");
  }

  SECTION("Parse give_item canonical fields") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"give_item","params":{"item_class":"Shotgun","amount":2}})", &cmd);
    REQUIRE(cmd.type == DMCP_CMD_GIVE_ITEM);
    REQUIRE(std::strcmp(cmd.data.give_item.item_class, "Shotgun") == 0);
    REQUIRE(cmd.data.give_item.amount == 2);
  }

  SECTION("Reject give_item invalid amount") {
    parse_invalid(R"({"type":"give_item","params":{"item":"Shotgun","amount":1}})");
    parse_invalid(R"({"type":"give_item","params":{"item_class":"Shotgun","quantity":1}})");
    parse_invalid(R"({"type":"give_item","params":{"item_class":"Shotgun","amount":0}})");
    parse_invalid(R"({"type":"give_item","params":{"item_class":"Shotgun","amount":1.5}})");
    parse_invalid(R"({"type":"give_item","params":{"item_class":"Shotgun","amount":1,"ammo":4}})");
  }

  SECTION("Parse set_player_health values") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"set_player_health","params":{"health":50}})", &cmd);
    REQUIRE(cmd.type == DMCP_CMD_SET_PLAYER_HEALTH);
    REQUIRE(cmd.data.set_health.health == 50);

    parse_invalid(R"({"type":"set_player_health","params":{"value":150}})");
  }

  SECTION("Clamp set_player_health out-of-range values") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"set_player_health","params":{"health":250}})", &cmd);
    REQUIRE(cmd.data.set_health.health == 200);
  }

  SECTION("Reject set_player_health invalid values") {
    parse_invalid(R"({"type":"set_player_health","params":{"health":0}})");
    parse_invalid(R"({"type":"set_player_health","params":{"health":-5}})");
  }

  SECTION("Parse set_player_position") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"set_player_position","params":{"x":12.5,"y":64,"angle":180}})", &cmd);
    REQUIRE(cmd.type == DMCP_CMD_SET_PLAYER_POSITION);
    REQUIRE(cmd.data.set_position.position.x == 12.5f);
    REQUIRE(cmd.data.set_position.position.y == 64.0f);
    REQUIRE(cmd.data.set_position.angle == 180.0f);
  }

  SECTION("Reject set_player_position invalid payload") {
    parse_invalid(R"({"type":"teleport_player","x":1,"y":2})");
    parse_invalid(R"({"type":"teleport_player","params":{"x":1,"y":2}})");
    parse_invalid(R"({"type":"set_player_position","params":{"position":true}})");
    parse_invalid(R"({"type":"set_player_position","params":{"position":{"x":1,"y":2}}})");
    parse_invalid(R"({"type":"set_player_position","params":{"x":1}})");
    parse_invalid(R"({"type":"set_player_position","params":{"x":1,"y":2,"z":3}})");
  }

  SECTION("Parse execute_console") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"execute_console","params":{"command":"iddqd"}})", &cmd);
    REQUIRE(cmd.type == DMCP_CMD_EXECUTE_CONSOLE);
    REQUIRE(std::strcmp(cmd.data.console.command, "iddqd") == 0);
  }

  SECTION("Reject execute_console invalid payload") {
    parse_invalid(R"({"type":"execute_console","params":{"command":""}})");
    parse_invalid(R"({"type":"execute_console","params":{"command":123}})");
    parse_invalid(R"({"type":"execute_console","params":{"command":"iddqd","repeat":2}})");
  }

  SECTION("Parse pause_game canonical boolean") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"pause_game","params":{"paused":true}})", &cmd);
    REQUIRE(cmd.type == DMCP_CMD_PAUSE_GAME);
    REQUIRE(cmd.data.pause.paused == true);
  }

  SECTION("Reject pause_game invalid payload") {
    parse_invalid(R"({"type":"pause_game","params":{}})");
    parse_invalid(R"({"type":"pause_game","params":{"paused":2}})");
    parse_invalid(R"({"type":"pause_game","params":{"paused":"false"}})");
    parse_invalid(R"({"type":"pause_game","params":{"pause":false}})");
    parse_invalid(R"({"type":"pause_game","params":{"paused":true,"value":true}})");
  }

  SECTION("Parse damage_entity and default damage_type") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"damage_entity","params":{"target_tid":3,"damage":25}})", &cmd);
    REQUIRE(cmd.type == DMCP_CMD_DAMAGE_ENTITY);
    REQUIRE(cmd.data.damage.target_tid == 3);
    REQUIRE(cmd.data.damage.damage == 25.0f);
    REQUIRE(std::strcmp(cmd.data.damage.damage_type, "Normal") == 0);
  }

  SECTION("Reject damage_entity invalid payload") {
    parse_invalid(R"({"type":"damage_entity","params":{"target_tid":-1,"damage":10}})");
    parse_invalid(R"({"type":"damage_entity","params":{"target_tid":3,"damage":-5}})");
    parse_invalid(R"({"type":"damage_entity","params":{"target_tid":3,"damage":10,"radius":32}})");
  }

  SECTION("Parse kill_entity") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"kill_entity","params":{"target_tid":9}})", &cmd);
    REQUIRE(cmd.type == DMCP_CMD_KILL_ENTITY);
    REQUIRE(cmd.data.kill.target_tid == 9);
  }

  SECTION("Reject kill_entity invalid payload") {
    parse_invalid(R"({"type":"kill_entity","params":{"target_tid":1.2}})");
    parse_invalid(R"({"type":"kill_entity","params":{}})");
    parse_invalid(R"({"type":"kill_entity","params":{"target_tid":1,"damage":1}})");
  }

  SECTION("Parse player_input canonical actions") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"player_input","params":{"action":"forward"}})", &cmd);
    REQUIRE(cmd.type == DMCP_CMD_PLAYER_INPUT);
    REQUIRE(cmd.data.input.action == DMCP_INPUT_FORWARD);

    parse_ok(R"({"type":"player_input","params":{"action":"aim","value":90}})", &cmd);
    REQUIRE(cmd.data.input.action == DMCP_INPUT_AIM);
    REQUIRE(cmd.data.input.aim_angle == 90.0f);

    parse_ok(R"({"type":"player_input","params":{"action":"weapon","value":3}})", &cmd);
    REQUIRE(cmd.data.input.action == DMCP_INPUT_WEAPON);
    REQUIRE(cmd.data.input.weapon_slot == 3);
  }

  SECTION("Reject player_input aliases and invalid weapon slots") {
    parse_invalid(R"({"type":"player_input","params":{"a":"fwd"}})");
    parse_invalid(R"({"type":"player_input","params":{"action":"atk"}})");
    parse_invalid(R"({"type":"player_input","params":{"action":"aim","angle":90}})");
    parse_invalid(R"({"type":"player_input","params":{"action":"weapon"}})");
    parse_invalid(R"({"type":"player_input","params":{"action":"weapon","value":8}})");
    parse_invalid(R"({"type":"player_input","params":{"action":"forward","duration":10}})");
  }

  SECTION("Validate flags field") {
    dmcp_command_t cmd = {};
    parse_ok(R"({"type":"pause_game","params":{"paused":true},"flags":3})", &cmd);
    REQUIRE(cmd.flags == 3);

    parse_invalid(R"({"type":"pause_game","params":{"paused":true},"flags":-1})");
    parse_invalid(R"({"type":"pause_game","params":{"paused":true},"flags":1.5})");
  }

  SECTION("Parse with invalid JSON returns error") {
    dmcp_command_t cmd    = {};
    const char*    json   = "not valid json";
    mcp_status_t   result = dmcp_parse_command_json(json, &cmd);
    REQUIRE(result.code == MCP_STATUS_CODE_ENCODING_FAILED);
  }

  SECTION("Parse with missing type returns error") { parse_invalid(R"({"other":"field"})"); }

  SECTION("Parse with invalid params type returns error") {
    parse_invalid(R"({"type":"pause_game","params":true})");
  }

  SECTION("Parse unknown command type returns error") {
    parse_invalid(R"({"type":"totally_unknown","params":{}})");
  }

  dmcp_context_destroy(ctx);
}
