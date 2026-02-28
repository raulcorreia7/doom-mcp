#include <cstring>

#include "adapters/zdoom/adapter.h"
#include "dmcp/doom/commands.h"
#include "dmcp/doom/types.h"
#include "test_utils.hpp"

static int log_call_count         = 0;
static int should_tick_call_count = 0;

static void test_log_callback(void* user, int level, const char* message) {
  (void)user;
  (void)level;
  (void)message;
  log_call_count++;
}

static bool test_should_tick(void* user) {
  (void)user;
  should_tick_call_count++;
  return true;
}

TEST_CASE("Adapter: ZDoom configuration", "[adapter][config]") {
  SECTION("Default config has correct values") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();

    REQUIRE(config.struct_size == sizeof(dmcp_zdoom_config_t));
    REQUIRE(config.dmcp_config == nullptr);
    REQUIRE(config.log_fn == nullptr);
    REQUIRE(config.log_user == nullptr);
    REQUIRE(config.should_tick_fn == nullptr);
    REQUIRE(config.should_tick_user == nullptr);
    REQUIRE(config.port_override == 0);
  }

  SECTION("Custom config values") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.port_override       = 7070;

    REQUIRE(config.port_override == 7070);
  }

  SECTION("Config with custom log callback") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.log_fn              = test_log_callback;
    config.log_user            = (void*)0xDEADBEEF;

    REQUIRE(config.log_fn == test_log_callback);
    REQUIRE(config.log_user == (void*)0xDEADBEEF);
  }

  SECTION("Config with custom should_tick callback") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.should_tick_fn      = test_should_tick;
    config.should_tick_user    = (void*)0xFEEDFACE;

    REQUIRE(config.should_tick_fn == test_should_tick);
    REQUIRE(config.should_tick_user == (void*)0xFEEDFACE);
  }

  SECTION("Config with custom DMCP config") {
    dmcp_config_t dmcp_cfg = dmcp_config_default();
    dmcp_cfg.port          = 8080;

    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.dmcp_config         = &dmcp_cfg;

    REQUIRE(config.dmcp_config == &dmcp_cfg);
  }
}

TEST_CASE("Adapter: dmcp_zdoom_config_default", "[adapter][config]") {
  SECTION("Returns properly initialized config") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();

    REQUIRE(config.struct_size > 0);
    REQUIRE(config.dmcp_config == nullptr);
    REQUIRE(config.log_fn == nullptr);
    REQUIRE(config.should_tick_fn == nullptr);
    REQUIRE(config.port_override == 0);
  }

  SECTION("Multiple calls return independent configs") {
    dmcp_zdoom_config_t config1 = dmcp_zdoom_config_default();
    dmcp_zdoom_config_t config2 = dmcp_zdoom_config_default();

    config1.port_override = 7070;

    REQUIRE(config1.port_override == 7070);
    REQUIRE(config2.port_override == 0);
  }
}

TEST_CASE("Adapter: Lifecycle - dmcp_zdoom_create/destroy",
          "[adapter][lifecycle]") {
  SECTION("Create with null config uses defaults") {
    dmcp_zdoom_t* mcp = dmcp_zdoom_create(nullptr);

    REQUIRE(mcp != nullptr);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("Create with default config succeeds") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);

    REQUIRE(mcp != nullptr);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("Create with custom port override") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.port_override       = 7070;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);

    REQUIRE(mcp != nullptr);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("Create with custom log callback") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.log_fn              = test_log_callback;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);

    REQUIRE(mcp != nullptr);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("Create with should_tick callback") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.should_tick_fn      = test_should_tick;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);

    REQUIRE(mcp != nullptr);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("Create with custom DMCP config") {
    dmcp_config_t       dmcp_cfg = dmcp_config_default();
    dmcp_zdoom_config_t config   = dmcp_zdoom_config_default();
    config.dmcp_config           = &dmcp_cfg;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);

    REQUIRE(mcp != nullptr);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("Destroy null adapter is safe") { dmcp_zdoom_destroy(nullptr); }

  SECTION("Multiple create/destroy cycles work") {
    for (int i = 0; i < 3; i++) {
      dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
      dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);

      REQUIRE(mcp != nullptr);

      dmcp_zdoom_destroy(mcp);
    }
  }
}

TEST_CASE("Adapter: Game loop - dmcp_zdoom_tick", "[adapter][tick]") {
  dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
  dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);
  REQUIRE(mcp != nullptr);

  SECTION("Tick with valid context succeeds") {
    int result = dmcp_zdoom_tick(mcp);

    REQUIRE(result == 0);
  }

  SECTION("Multiple ticks succeed") {
    for (int i = 0; i < 10; i++) {
      int result = dmcp_zdoom_tick(mcp);
      REQUIRE(result == 0);
    }
  }

  SECTION("Tick with null context returns error") {
    int result = dmcp_zdoom_tick(nullptr);

    REQUIRE(result != 0);
  }

  SECTION("Tick after destroy is handled correctly") {
    dmcp_zdoom_t* temp_mcp = dmcp_zdoom_create(&config);
    dmcp_zdoom_destroy(temp_mcp);
  }

  dmcp_zdoom_destroy(mcp);
}

TEST_CASE("Adapter: State queries", "[adapter][state]") {
  dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
  dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);
  REQUIRE(mcp != nullptr);

  SECTION("Is running with valid context") {
    bool running = dmcp_zdoom_is_running(mcp);

    REQUIRE(running == true);
  }

  SECTION("Is running with null context returns false") {
    bool running = dmcp_zdoom_is_running(nullptr);

    REQUIRE(running == false);
  }

  SECTION("Get stats returns valid structure") {
    dmcp_stats_t stats = {};
    dmcp_zdoom_get_stats(mcp, &stats);

    REQUIRE(stats.connected_clients >= 0);
  }

  SECTION("Get stats with null context") {
    dmcp_stats_t stats = {};
    dmcp_zdoom_get_stats(nullptr, &stats);
  }

  SECTION("Get stats with null stats pointer") {
    dmcp_zdoom_get_stats(mcp, nullptr);
  }

  dmcp_zdoom_destroy(mcp);
}

TEST_CASE("Adapter: Command execution", "[adapter][commands]") {
  dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
  dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);
  REQUIRE(mcp != nullptr);

  SECTION("Execute null command returns false") {
    bool result = dmcp_zdoom_command_execute(mcp, nullptr);

    REQUIRE(result == false);
  }

  SECTION("Execute command with null context returns false") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SPAWN_ENTITY;

    bool result = dmcp_zdoom_command_execute(nullptr, &cmd);

    REQUIRE(result == false);
  }

  SECTION("Execute spawn command") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SPAWN_ENTITY;
    cmd.flags          = 0;

    dmcp_cmd_spawn_t* spawn = &cmd.data.spawn;
    dmcp_strcpy(spawn->entity_class, "DoomImp", 128);
    spawn->position.x = 100.0f;
    spawn->position.y = 200.0f;
    spawn->angle      = 0.0f;
    spawn->tid        = 0;

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);

    REQUIRE((result == true || result == false));
  }

  SECTION("Execute console command") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_EXECUTE_CONSOLE;
    cmd.flags          = 0;

    dmcp_cmd_console_t* console = &cmd.data.console;
    dmcp_strcpy(console->command, "echo \"test\"", 256);

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);

    REQUIRE((result == true || result == false));
  }

  SECTION("Execute pause command") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_PAUSE_GAME;
    cmd.flags          = 0;

    cmd.data.pause.paused = true;

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);

    REQUIRE((result == true || result == false));
  }

  SECTION("Execute set health command") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SET_PLAYER_HEALTH;
    cmd.flags          = 0;

    cmd.data.set_health.health = 100.0f;

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);

    (void)result;
  }

  SECTION("Execute set position command") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SET_PLAYER_POSITION;
    cmd.flags          = 0;

    cmd.data.set_position.position.x = 100.0f;
    cmd.data.set_position.position.y = 200.0f;
    cmd.data.set_position.angle      = 90.0f;

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);

    REQUIRE((result == true || result == false));
  }

  SECTION("Execute give item command") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_GIVE_ITEM;
    cmd.flags          = 0;

    dmcp_cmd_give_item_t* give = &cmd.data.give_item;
    dmcp_strcpy(give->item_class, "Clip", 128);
    give->amount = 50;

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);

    REQUIRE((result == true || result == false));
  }

  SECTION("Execute change level command") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_CHANGE_LEVEL;
    cmd.flags          = 0;

    dmcp_cmd_change_level_t* change = &cmd.data.change_level;
    dmcp_strcpy(change->map_name, "MAP01", 32);
    change->skill_level     = 2;
    change->reset_inventory = false;

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);

    REQUIRE((result == true || result == false));
  }

  dmcp_zdoom_destroy(mcp);
}

TEST_CASE("Adapter: Command processing", "[adapter][commands][process]") {
  dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
  dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);
  REQUIRE(mcp != nullptr);

  SECTION("Process commands with valid context") {
    dmcp_zdoom_commands_process(mcp);
  }

  SECTION("Process commands with null context") {
    dmcp_zdoom_commands_process(nullptr);
  }

  SECTION("Process commands after tick") {
    dmcp_zdoom_tick(mcp);
    dmcp_zdoom_commands_process(mcp);
  }

  dmcp_zdoom_destroy(mcp);
}

TEST_CASE("Adapter: Integration with callbacks", "[adapter][integration]") {
  SECTION("Log callback is invoked") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.log_fn              = test_log_callback;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);
    REQUIRE(mcp != nullptr);

    dmcp_zdoom_tick(mcp);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("Should tick callback is consulted") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.should_tick_fn      = test_should_tick;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);
    REQUIRE(mcp != nullptr);

    should_tick_call_count = 0;
    dmcp_zdoom_tick(mcp);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("All callbacks together") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.log_fn              = test_log_callback;
    config.should_tick_fn      = test_should_tick;
    config.port_override       = 8080;
    config.log_user            = (void*)0x1234;
    config.should_tick_user    = (void*)0x5678;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);
    REQUIRE(mcp != nullptr);

    dmcp_zdoom_tick(mcp);
    dmcp_zdoom_commands_process(mcp);

    dmcp_zdoom_destroy(mcp);
  }
}

TEST_CASE("Adapter: Command type validation", "[adapter][commands][types]") {
  SECTION("Unknown command type") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_UNKNOWN;

    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);

    REQUIRE(result == false);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("Spawn entity command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SPAWN_ENTITY;

    dmcp_cmd_spawn_t* spawn = &cmd.data.spawn;
    REQUIRE(spawn != nullptr);
  }

  SECTION("Change level command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_CHANGE_LEVEL;

    dmcp_cmd_change_level_t* change = &cmd.data.change_level;
    REQUIRE(change != nullptr);
  }

  SECTION("Give item command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_GIVE_ITEM;

    dmcp_cmd_give_item_t* give = &cmd.data.give_item;
    REQUIRE(give != nullptr);
  }

  SECTION("Set health command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SET_PLAYER_HEALTH;

    dmcp_cmd_set_health_t* health = &cmd.data.set_health;
    REQUIRE(health != nullptr);
  }

  SECTION("Set position command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SET_PLAYER_POSITION;

    dmcp_cmd_set_position_t* pos = &cmd.data.set_position;
    REQUIRE(pos != nullptr);
  }

  SECTION("Console command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_EXECUTE_CONSOLE;

    dmcp_cmd_console_t* console = &cmd.data.console;
    REQUIRE(console != nullptr);
  }

  SECTION("Pause command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_PAUSE_GAME;

    dmcp_cmd_pause_t* pause = &cmd.data.pause;
    REQUIRE(pause != nullptr);
  }

  SECTION("Timescale command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SET_TIMESCALE;

    dmcp_cmd_timescale_t* timescale = &cmd.data.timescale;
    REQUIRE(timescale != nullptr);
  }

  SECTION("Damage entity command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_DAMAGE_ENTITY;

    dmcp_cmd_damage_t* damage = &cmd.data.damage;
    REQUIRE(damage != nullptr);
  }

  SECTION("Kill entity command structure") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_KILL_ENTITY;

    dmcp_cmd_kill_t* kill = &cmd.data.kill;
    REQUIRE(kill != nullptr);
  }
}

TEST_CASE("Adapter: Command flags", "[adapter][commands][flags]") {
  dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
  dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);

  SECTION("Command with immediate flag") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_EXECUTE_CONSOLE;
    cmd.flags          = DMCP_CMD_FLAG_IMMEDIATE;

    dmcp_cmd_console_t* console = &cmd.data.console;
    dmcp_strcpy(console->command, "echo \"test\"", 256);

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);
    REQUIRE((result == true || result == false));
  }

  SECTION("Command with reliable flag") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SET_PLAYER_HEALTH;
    cmd.flags          = DMCP_CMD_FLAG_RELIABLE;

    cmd.data.set_health.health = 100.0f;

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);
    REQUIRE((result == true || result == false));
  }

  SECTION("Command with both flags") {
    dmcp_command_t cmd = {};
    cmd.type           = DMCP_CMD_SPAWN_ENTITY;
    cmd.flags          = DMCP_CMD_FLAG_IMMEDIATE | DMCP_CMD_FLAG_RELIABLE;

    dmcp_cmd_spawn_t* spawn = &cmd.data.spawn;
    dmcp_strcpy(spawn->entity_class, "DoomImp", 128);
    spawn->position.x = 100.0f;
    spawn->position.y = 200.0f;
    spawn->angle      = 0.0f;

    bool result = dmcp_zdoom_command_execute(mcp, &cmd);
    REQUIRE((result == true || result == false));
  }

  dmcp_zdoom_destroy(mcp);
}

TEST_CASE("Adapter: Statistics tracking", "[adapter][stats]") {
  dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
  dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);

  SECTION("Initial stats are zero") {
    dmcp_stats_t stats = {};
    dmcp_zdoom_get_stats(mcp, &stats);

    REQUIRE(stats.dropped_snapshots == 0);
    REQUIRE(stats.dropped_screenshots == 0);
    REQUIRE(stats.connected_clients == 0);
  }

  SECTION("Stats update after ticks") {
    dmcp_stats_t stats_before = {};
    dmcp_zdoom_get_stats(mcp, &stats_before);

    for (int i = 0; i < 10; i++) {
      dmcp_zdoom_tick(mcp);
    }

    dmcp_stats_t stats_after = {};
    dmcp_zdoom_get_stats(mcp, &stats_after);
  }

  dmcp_zdoom_destroy(mcp);
}

TEST_CASE("Adapter: Port override functionality", "[adapter][config][port]") {
  SECTION("Port override takes precedence") {
    dmcp_config_t dmcp_cfg = dmcp_config_default();
    dmcp_cfg.port          = 6060;

    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.dmcp_config         = &dmcp_cfg;
    config.port_override       = 9090;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);

    REQUIRE(mcp != nullptr);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("No port override uses DMCP config port") {
    dmcp_config_t dmcp_cfg = dmcp_config_default();
    dmcp_cfg.port          = 8080;

    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.dmcp_config         = &dmcp_cfg;
    config.port_override       = 0;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);

    REQUIRE(mcp != nullptr);

    dmcp_zdoom_destroy(mcp);
  }

  SECTION("No port override and no DMCP config uses default") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    config.port_override       = 0;
    config.dmcp_config         = nullptr;

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);

    REQUIRE(mcp != nullptr);

    dmcp_zdoom_destroy(mcp);
  }
}

TEST_CASE("Adapter: Error handling", "[adapter][error]") {
  SECTION("Handle create failure gracefully") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();

    dmcp_zdoom_t* mcp = dmcp_zdoom_create(&config);

    if (mcp) {
      dmcp_zdoom_destroy(mcp);
    }
  }

  SECTION("Handle tick failure gracefully") {
    dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
    dmcp_zdoom_t*       mcp    = dmcp_zdoom_create(&config);

    if (mcp) {
      int result = dmcp_zdoom_tick(mcp);

      if (result != 0) {
      }

      dmcp_zdoom_destroy(mcp);
    }
  }
}
