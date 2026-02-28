#include <array>
#include <cstring>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "dmcp/adapters/fake.h"
#include "dmcp/doom/constants.h"

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

dmcp_fake_config_t make_fake_config(void) {
  dmcp_fake_config_t cfg = dmcp_fake_config_default();
  cfg.base.port          = kFakeAdapterPort;
  cfg.base.target_hz     = MCP_DEFAULT_TARGET_HZ;
  return cfg;
}

dmcp_snapshot_t require_snapshot(dmcp_fake_t* fake) {
  dmcp_snapshot_t snapshot{};
  REQUIRE(dmcp_fake_snapshot_get(fake, &snapshot));
  return snapshot;
}

dmcp_command_result_t require_completed_result(dmcp_context_t* ctx, uint64_t sequence) {
  dmcp_command_result_t result{};
  REQUIRE(dmcp_command_result_get(ctx, sequence, &result).code == MCP_RESULT_CODE_OK);
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

TEST_CASE("Fake adapter simple: lifecycle", "[adapter][fake][simple]") {
  dmcp_fake_config_t cfg = make_fake_config();

  dmcp_fake_t* fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);
  REQUIRE(dmcp_fake_context(fake) != nullptr);
  REQUIRE(dmcp_fake_is_running(fake));

  dmcp_stats_t stats{};
  dmcp_fake_get_stats(fake, &stats);
  REQUIRE(stats.connected_clients == 0);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter medium: tick advances simulation", "[adapter][fake][medium]") {
  dmcp_fake_config_t cfg  = make_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  const dmcp_snapshot_t before = require_snapshot(fake);

  for (uint32_t i = 0; i < kFakeTickIterations; ++i) {
    REQUIRE(dmcp_fake_tick(fake).code == MCP_RESULT_CODE_OK);
  }

  const dmcp_snapshot_t after = require_snapshot(fake);

  REQUIRE(after.level.tic > before.level.tic);
  REQUIRE(after.level.leveltime > before.level.leveltime);
  REQUIRE(after.enemy_count == before.enemy_count);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter medium: input queue processing", "[adapter][fake][medium]") {
  dmcp_fake_config_t cfg  = make_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  dmcp_context_t* ctx = dmcp_fake_context(fake);
  REQUIRE(ctx != nullptr);

  const dmcp_snapshot_t before = require_snapshot(fake);

  dmcp_command_t input_cmd{};
  input_cmd.type              = DMCP_CMD_PLAYER_INPUT;
  input_cmd.data.input.action = DMCP_INPUT_FORWARD;

  REQUIRE(dmcp_push_input(ctx, &input_cmd).code == MCP_RESULT_CODE_OK);
  REQUIRE(input_cmd.sequence > 0);

  dmcp_fake_inputs_process(fake);

  const dmcp_command_result_t result = require_completed_result(ctx, input_cmd.sequence);
  REQUIRE(result.success == true);

  const dmcp_snapshot_t after = require_snapshot(fake);
  REQUIRE(after.player.position.y > before.player.position.y);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter medium: command queue processing", "[adapter][fake][medium]") {
  dmcp_fake_config_t cfg  = make_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  dmcp_context_t* ctx = dmcp_fake_context(fake);
  REQUIRE(ctx != nullptr);

  dmcp_snapshot_t before = require_snapshot(fake);

  dmcp_command_t set_health{};
  set_health.type                   = DMCP_CMD_SET_PLAYER_HEALTH;
  set_health.data.set_health.health = kRequestedHealth;
  REQUIRE(dmcp_push_command(ctx, &set_health).code == MCP_RESULT_CODE_OK);

  dmcp_command_t spawn{};
  spawn.type = DMCP_CMD_SPAWN_ENTITY;
  dmcp_strcpy(spawn.data.spawn.entity_class, kSpawnClass, sizeof(spawn.data.spawn.entity_class));
  spawn.data.spawn.tid      = kSpawnedEnemyTid;
  spawn.data.spawn.position = {before.player.position.x + kSpawnOffset,
                               before.player.position.y + kSpawnOffset};
  spawn.data.spawn.angle    = before.player.angle;
  REQUIRE(dmcp_push_command(ctx, &spawn).code == MCP_RESULT_CODE_OK);

  dmcp_fake_commands_process(fake);

  const dmcp_command_result_t health_result = require_completed_result(ctx, set_health.sequence);
  const dmcp_command_result_t spawn_result  = require_completed_result(ctx, spawn.sequence);
  REQUIRE(health_result.success == true);
  REQUIRE(spawn_result.success == true);

  const dmcp_snapshot_t after = require_snapshot(fake);
  REQUIRE(after.player.hp == kRequestedHealth);
  REQUIRE(after.enemy_count == before.enemy_count + 1);

  dmcp_fake_destroy(fake);
}

TEST_CASE("Fake adapter hard: end-to-end smoke scenario", "[adapter][fake][hard][smoke]") {
  dmcp_fake_config_t cfg  = make_fake_config();
  dmcp_fake_t*       fake = dmcp_fake_create(&cfg);
  REQUIRE(fake != nullptr);

  dmcp_context_t* ctx = dmcp_fake_context(fake);
  REQUIRE(ctx != nullptr);

  dmcp_command_t change_level{};
  change_level.type = DMCP_CMD_CHANGE_LEVEL;
  dmcp_strcpy(change_level.data.change_level.map_name, kScenarioMap,
              sizeof(change_level.data.change_level.map_name));
  change_level.data.change_level.skill_level     = DMCP_SKILL_MIN;
  change_level.data.change_level.reset_inventory = false;
  REQUIRE(dmcp_push_command(ctx, &change_level).code == MCP_RESULT_CODE_OK);

  dmcp_command_t spawn{};
  spawn.type = DMCP_CMD_SPAWN_ENTITY;
  dmcp_strcpy(spawn.data.spawn.entity_class, kSpawnClass, sizeof(spawn.data.spawn.entity_class));
  spawn.data.spawn.tid      = kSpawnedEnemyTid;
  spawn.data.spawn.position = {kScenarioSpawnX, kScenarioSpawnY};
  spawn.data.spawn.angle    = kScenarioSpawnAngle;
  REQUIRE(dmcp_push_command(ctx, &spawn).code == MCP_RESULT_CODE_OK);

  dmcp_command_t damage{};
  damage.type                   = DMCP_CMD_DAMAGE_ENTITY;
  damage.data.damage.target_tid = kSpawnedEnemyTid;
  damage.data.damage.damage     = kDamageAmount;
  dmcp_strcpy(damage.data.damage.damage_type, kDamageTypeNormal,
              sizeof(damage.data.damage.damage_type));
  REQUIRE(dmcp_push_command(ctx, &damage).code == MCP_RESULT_CODE_OK);

  dmcp_command_t kill{};
  kill.type                 = DMCP_CMD_KILL_ENTITY;
  kill.data.kill.target_tid = kSpawnedEnemyTid;
  REQUIRE(dmcp_push_command(ctx, &kill).code == MCP_RESULT_CODE_OK);

  dmcp_command_t give_item{};
  give_item.type = DMCP_CMD_GIVE_ITEM;
  dmcp_strcpy(give_item.data.give_item.item_class, kScenarioItem,
              sizeof(give_item.data.give_item.item_class));
  give_item.data.give_item.amount = kExpectedInventoryAmount;
  REQUIRE(dmcp_push_command(ctx, &give_item).code == MCP_RESULT_CODE_OK);

  dmcp_command_t console{};
  console.type = DMCP_CMD_EXECUTE_CONSOLE;
  dmcp_strcpy(console.data.console.command, kScenarioConsoleCommand,
              sizeof(console.data.console.command));
  REQUIRE(dmcp_push_command(ctx, &console).code == MCP_RESULT_CODE_OK);

  for (uint32_t i = 0; i < kScenarioTicks; ++i) {
    REQUIRE(dmcp_fake_tick(fake).code == MCP_RESULT_CODE_OK);
  }

  const std::array<uint64_t, 6> sequences = {change_level.sequence, spawn.sequence,
                                             damage.sequence,       kill.sequence,
                                             give_item.sequence,    console.sequence};
  for (uint64_t sequence : sequences) {
    const dmcp_command_result_t result = require_completed_result(ctx, sequence);
    REQUIRE(result.success == true);
  }

  const dmcp_snapshot_t snapshot = require_snapshot(fake);
  REQUIRE(std::strcmp(snapshot.level.level_id, kScenarioMap) == 0);
  REQUIRE(snapshot.level.kill_count >= 1);
  REQUIRE(inventory_contains(snapshot, kScenarioItem, kExpectedInventoryAmount));

  char snapshot_json[MCP_MAX_JSON_SIZE] = {0};
  REQUIRE(dmcp_snapshot_to_json(&snapshot, snapshot_json, sizeof(snapshot_json)) > 0);
  REQUIRE(std::string(snapshot_json).find(kScenarioMap) != std::string::npos);

  dmcp_fake_destroy(fake);
}
