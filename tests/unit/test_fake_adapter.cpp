#include <array>
#include <cstring>
#include <memory>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "dmcp/adapters/fake.h"
#include "dmcp/doom/constants.h"
#include "dmcp_hooks.h"

namespace {

constexpr uint16_t kFakeAdapterPort         = 17070;
constexpr uint32_t kFakeTickIterations      = 3;
constexpr uint32_t kScenarioTicks           = 5;
constexpr int32_t  kRequestedHealth         = DMCP_PLAYER_MAX_HEALTH;
constexpr int32_t  kSpawnedEnemyTid         = 777;
constexpr int32_t  kDamageAmount            = 25;
constexpr int32_t  kExpectedInventoryAmount = 3;
constexpr float    kSpawnOffset             = 16.0f;
constexpr float    kScenarioSpawnX          = 160.0f;
constexpr float    kScenarioSpawnY          = 96.0f;
constexpr float    kScenarioSpawnAngle      = 0.0f;

constexpr const char* kSpawnClass             = "DoomImp";
constexpr const char* kScenarioMap            = "MAP02";
constexpr const char* kScenarioItem           = "Backpack";
constexpr const char* kScenarioConsoleCommand = "give all";
constexpr const char* kDamageTypeNormal       = "Normal";

constexpr std::array<int32_t, DMCP_MAX_AMMO_TYPES> kExpectedDefaultMaxAmmo = {200, 50, 300, 50};
constexpr std::array<int32_t, DMCP_MAX_AMMO_TYPES> kExpectedInitialAmmo    = {50, 8, 0, 0};

dmcp_fake_config_t make_fake_config(void) {
  dmcp_fake_config_t cfg   = dmcp_fake_config_default();
  cfg.base.port            = kFakeAdapterPort;
  cfg.base.target_hz       = DMCP_DEFAULT_TARGET_HZ;
  cfg.base.start_transport = false;
  return cfg;
}

dmcp_fake_config_t make_privileged_fake_config(void) {
  dmcp_fake_config_t cfg                      = make_fake_config();
  cfg.base.permissions.allow_console_commands = true;
  cfg.base.permissions.allow_cheats           = true;
  return cfg;
}

std::unique_ptr<dmcp_snapshot_t> require_snapshot(dmcp_fake_t* fake) {
  auto snapshot = std::make_unique<dmcp_snapshot_t>();
  REQUIRE(dmcp_fake_snapshot_get(fake, snapshot.get()));
  return snapshot;
}

dmcp_command_result_t require_completed_result(dmcp_context_t* ctx, uint64_t sequence) {
  dmcp_command_result_t result{};
  REQUIRE(dmcp_command_result_get(ctx, sequence, &result).code == MCP_STATUS_CODE_OK);
  REQUIRE(result.sequence == sequence);
  REQUIRE(result.completed == true);
  return result;
}

bool inventory_contains(const dmcp_snapshot_t& snapshot, const char* item_name, int32_t amount) {
  for (uint32_t i = 0; i < snapshot.inventory_count; ++i) {
    if (std::strcmp(snapshot.inventory[i].name, item_name) == 0 &&
        snapshot.inventory[i].amount == amount) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST_CASE("Fake adapter simple: default config", "[adapter][fake][simple]") {
  const dmcp_fake_config_t cfg = dmcp_fake_config_default();

  REQUIRE(cfg.struct_size == sizeof(dmcp_fake_config_t));
  REQUIRE(cfg.base.struct_size == sizeof(dmcp_config_t));
  REQUIRE(cfg.base.target_hz == DMCP_FAKE_DEFAULT_TARGET_HZ);
  REQUIRE(cfg.seed == DMCP_FAKE_DEFAULT_SEED);
  REQUIRE(cfg.max_enemies == DMCP_MAX_ENEMIES);
  REQUIRE(cfg.max_entities == DMCP_MAX_ENTITIES);
}

TEST_CASE("Adapter common simple: screenshots are opt-in from argv", "[adapter][common][simple]") {
  dmcp_engine_config_t default_config = dmcp_engine_config_default();
  REQUIRE(default_config.screenshot_enabled == false);
  REQUIRE(default_config.allow_cheats == true);
  REQUIRE(default_config.allow_console_commands == true);

  char                 program[]      = "engine";
  char*                no_args[]      = {program};
  dmcp_engine_config_t parsed_default = dmcp_engine_config_from_argv(1, no_args);
  REQUIRE(parsed_default.screenshot_enabled == false);
  REQUIRE(parsed_default.allow_cheats == true);
  REQUIRE(parsed_default.allow_console_commands == true);

  char                 screenshot_flag[] = "-dmcp_enable_screenshots";
  char*                screenshot_args[] = {program, screenshot_flag};
  dmcp_engine_config_t parsed_screenshot = dmcp_engine_config_from_argv(2, screenshot_args);
  REQUIRE(parsed_screenshot.screenshot_enabled == true);
}

TEST_CASE("Fake adapter simple: lifecycle", "[adapter][fake][simple]") {
  dmcp_fake_config_t cfg = make_fake_config();

  dmcp_fake_t* fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);
  REQUIRE(dmcp_fake_get_context(fake) != nullptr);
  REQUIRE(dmcp_fake_is_running(fake));

  dmcp_stats_t stats{};
  dmcp_fake_get_stats(fake, &stats);
  REQUIRE(stats.connected_clients == 0);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter simple: Doom ammo model", "[adapter][fake][simple]") {
  dmcp_fake_config_t cfg  = make_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  const auto snapshot = require_snapshot(fake);
  for (size_t i = 0; i < kExpectedDefaultMaxAmmo.size(); ++i) {
    REQUIRE(snapshot->player.maxammo[i] == kExpectedDefaultMaxAmmo[i]);
    REQUIRE(snapshot->player.ammo[i] == kExpectedInitialAmmo[i]);
  }

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter medium: tick advances simulation", "[adapter][fake][medium]") {
  dmcp_fake_config_t cfg  = make_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  const auto before = require_snapshot(fake);

  for (uint32_t i = 0; i < kFakeTickIterations; ++i) {
    REQUIRE(dmcp_fake_tick(fake).code == MCP_STATUS_CODE_OK);
  }

  const auto after = require_snapshot(fake);

  REQUIRE(after->level.tic > before->level.tic);
  REQUIRE(after->level.leveltime > before->level.leveltime);
  REQUIRE(after->enemy_count == before->enemy_count);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter medium: input queue processing", "[adapter][fake][medium]") {
  dmcp_fake_config_t cfg  = make_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  dmcp_context_t* ctx = dmcp_fake_get_context(fake);
  REQUIRE(ctx != nullptr);

  const auto before = require_snapshot(fake);

  dmcp_command_t input_cmd{};
  input_cmd.type              = DMCP_CMD_PLAYER_INPUT;
  input_cmd.data.input.action = DMCP_INPUT_FORWARD;

  REQUIRE(dmcp_push_input(ctx, &input_cmd).code == MCP_STATUS_CODE_OK);
  REQUIRE(input_cmd.sequence > 0);

  dmcp_fake_inputs_process(fake);

  const dmcp_command_result_t result = require_completed_result(ctx, input_cmd.sequence);
  REQUIRE(result.success == true);

  const auto after = require_snapshot(fake);
  REQUIRE(after->player.position.y > before->player.position.y);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter medium: weapon slot validation matches Doom keys",
          "[adapter][fake][medium]") {
  dmcp_fake_config_t cfg  = make_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  dmcp_context_t* ctx = dmcp_fake_get_context(fake);
  REQUIRE(ctx != nullptr);

  dmcp_command_t valid_input{};
  valid_input.type                   = DMCP_CMD_PLAYER_INPUT;
  valid_input.data.input.action      = DMCP_INPUT_WEAPON;
  valid_input.data.input.weapon_slot = 7;
  REQUIRE(dmcp_push_input(ctx, &valid_input).code == MCP_STATUS_CODE_OK);

  dmcp_fake_inputs_process(fake);
  dmcp_command_result_t valid_result = require_completed_result(ctx, valid_input.sequence);
  REQUIRE(valid_result.success == true);

  dmcp_command_t invalid_input{};
  invalid_input.type                   = DMCP_CMD_PLAYER_INPUT;
  invalid_input.data.input.action      = DMCP_INPUT_WEAPON;
  invalid_input.data.input.weapon_slot = 8;
  REQUIRE(dmcp_push_input(ctx, &invalid_input).code == MCP_STATUS_CODE_OK);

  dmcp_fake_inputs_process(fake);
  dmcp_command_result_t invalid_result = require_completed_result(ctx, invalid_input.sequence);
  REQUIRE(invalid_result.success == false);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter medium: command queue processing", "[adapter][fake][medium]") {
  dmcp_fake_config_t cfg  = make_privileged_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  dmcp_context_t* ctx = dmcp_fake_get_context(fake);
  REQUIRE(ctx != nullptr);

  const auto before = require_snapshot(fake);

  dmcp_command_t set_health{};
  set_health.type                   = DMCP_CMD_SET_PLAYER_HEALTH;
  set_health.data.set_health.health = kRequestedHealth;
  REQUIRE(dmcp_push_command(ctx, &set_health).code == MCP_STATUS_CODE_OK);

  dmcp_command_t spawn{};
  spawn.type = DMCP_CMD_SPAWN_ENTITY;
  mcp_strcpy_safe(spawn.data.spawn.entity_class, sizeof(spawn.data.spawn.entity_class),
                  kSpawnClass);
  spawn.data.spawn.tid      = kSpawnedEnemyTid;
  spawn.data.spawn.position = {before->player.position.x + kSpawnOffset,
                               before->player.position.y + kSpawnOffset};
  spawn.data.spawn.angle    = before->player.angle;
  REQUIRE(dmcp_push_command(ctx, &spawn).code == MCP_STATUS_CODE_OK);

  dmcp_fake_commands_process(fake);

  const dmcp_command_result_t health_result = require_completed_result(ctx, set_health.sequence);
  const dmcp_command_result_t spawn_result  = require_completed_result(ctx, spawn.sequence);
  REQUIRE(health_result.success == true);
  REQUIRE(spawn_result.success == true);

  const auto after = require_snapshot(fake);
  REQUIRE(after->player.hp == kRequestedHealth);
  REQUIRE(after->enemy_count == before->enemy_count + 1);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter hard: end-to-end smoke scenario", "[adapter][fake][hard][smoke]") {
  dmcp_fake_config_t cfg  = make_privileged_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  dmcp_context_t* ctx = dmcp_fake_get_context(fake);
  REQUIRE(ctx != nullptr);

  dmcp_command_t change_level{};
  change_level.type = DMCP_CMD_CHANGE_LEVEL;
  mcp_strcpy_safe(change_level.data.change_level.map_name,
                  sizeof(change_level.data.change_level.map_name), kScenarioMap);
  change_level.data.change_level.skill_level     = DMCP_SKILL_MIN;
  change_level.data.change_level.reset_inventory = false;
  REQUIRE(dmcp_push_command(ctx, &change_level).code == MCP_STATUS_CODE_OK);

  dmcp_command_t spawn{};
  spawn.type = DMCP_CMD_SPAWN_ENTITY;
  mcp_strcpy_safe(spawn.data.spawn.entity_class, sizeof(spawn.data.spawn.entity_class),
                  kSpawnClass);
  spawn.data.spawn.tid      = kSpawnedEnemyTid;
  spawn.data.spawn.position = {kScenarioSpawnX, kScenarioSpawnY};
  spawn.data.spawn.angle    = kScenarioSpawnAngle;
  REQUIRE(dmcp_push_command(ctx, &spawn).code == MCP_STATUS_CODE_OK);

  dmcp_command_t damage{};
  damage.type                   = DMCP_CMD_DAMAGE_ENTITY;
  damage.data.damage.target_tid = kSpawnedEnemyTid;
  damage.data.damage.damage     = kDamageAmount;
  mcp_strcpy_safe(damage.data.damage.damage_type, sizeof(damage.data.damage.damage_type),
                  kDamageTypeNormal);
  REQUIRE(dmcp_push_command(ctx, &damage).code == MCP_STATUS_CODE_OK);

  dmcp_command_t kill{};
  kill.type                 = DMCP_CMD_KILL_ENTITY;
  kill.data.kill.target_tid = kSpawnedEnemyTid;
  REQUIRE(dmcp_push_command(ctx, &kill).code == MCP_STATUS_CODE_OK);

  dmcp_command_t give_item{};
  give_item.type = DMCP_CMD_GIVE_ITEM;
  mcp_strcpy_safe(give_item.data.give_item.item_class, sizeof(give_item.data.give_item.item_class),
                  kScenarioItem);
  give_item.data.give_item.amount = kExpectedInventoryAmount;
  REQUIRE(dmcp_push_command(ctx, &give_item).code == MCP_STATUS_CODE_OK);

  dmcp_command_t console{};
  console.type = DMCP_CMD_EXECUTE_CONSOLE;
  mcp_strcpy_safe(console.data.console.command, sizeof(console.data.console.command),
                  kScenarioConsoleCommand);
  REQUIRE(dmcp_push_command(ctx, &console).code == MCP_STATUS_CODE_OK);

  for (uint32_t i = 0; i < kScenarioTicks; ++i) {
    REQUIRE(dmcp_fake_tick(fake).code == MCP_STATUS_CODE_OK);
  }

  const std::array<uint64_t, 6> sequences = {change_level.sequence, spawn.sequence,
                                             damage.sequence,       kill.sequence,
                                             give_item.sequence,    console.sequence};
  for (uint64_t sequence : sequences) {
    const dmcp_command_result_t result = require_completed_result(ctx, sequence);
    REQUIRE(result.success == true);
  }

  const auto snapshot = require_snapshot(fake);
  REQUIRE(std::strcmp(snapshot->level.level_id, kScenarioMap) == 0);
  REQUIRE(snapshot->level.kill_count >= 1);
  REQUIRE(inventory_contains(*snapshot, kScenarioItem, kExpectedInventoryAmount));

  char snapshot_json[MCP_MAX_JSON_SIZE] = {0};
  REQUIRE(dmcp_snapshot_to_json(snapshot.get(), snapshot_json, sizeof(snapshot_json)) > 0);
  REQUIRE(std::string(snapshot_json).find(kScenarioMap) != std::string::npos);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter medium: give all fills every Doom ammo pool", "[adapter][fake][medium]") {
  dmcp_fake_config_t cfg  = make_privileged_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  dmcp_context_t* ctx = dmcp_fake_get_context(fake);
  REQUIRE(ctx != nullptr);

  dmcp_command_t console{};
  console.type = DMCP_CMD_EXECUTE_CONSOLE;
  mcp_strcpy_safe(console.data.console.command, sizeof(console.data.console.command),
                  kScenarioConsoleCommand);
  REQUIRE(dmcp_push_command(ctx, &console).code == MCP_STATUS_CODE_OK);

  dmcp_fake_commands_process(fake);
  const dmcp_command_result_t result = require_completed_result(ctx, console.sequence);
  REQUIRE(result.success == true);

  const auto snapshot = require_snapshot(fake);
  for (size_t i = 0; i < kExpectedDefaultMaxAmmo.size(); ++i) {
    REQUIRE(snapshot->player.ammo[i] == snapshot->player.maxammo[i]);
    REQUIRE(snapshot->player.maxammo[i] == kExpectedDefaultMaxAmmo[i]);
  }

  dmcp_fake_destroy(fake);
}
