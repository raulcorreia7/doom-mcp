#include "dmcp/adapters/fake.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <random>
#include <string_view>

#include "dmcp_adapter_command_queue.h"
#include "mcp/core/memory.h"

struct dmcp_fake_s {
  dmcp_context_t*    dmcp_ctx = nullptr;
  dmcp_fake_config_t config{};

  dmcp_snapshot_t snapshot{};
  bool            paused = false;

  int32_t next_enemy_id  = 0;
  int32_t next_entity_id = 0;

  uint64_t simulated_ticks = 0;

  std::mt19937       rng;
  mutable std::mutex state_mutex;
};

namespace {

constexpr float kPlayerStartX     = 128.0f;
constexpr float kPlayerStartY     = 64.0f;
constexpr float kPlayerStartZ     = 0.0f;
constexpr float kPlayerStartAngle = 90.0f;
constexpr float kEnemySpawnAngle  = 180.0f;
constexpr float kRadiansToDegrees = 57.29577951308232f;

constexpr float   kEnemyMoveStep       = 2.0f;
constexpr float   kEnemyContactRange   = 16.0f;
constexpr float   kEnemyContactRangeSq = kEnemyContactRange * kEnemyContactRange;
constexpr int32_t kEnemyContactDamage  = 5;

constexpr float   kInputMoveStep     = 8.0f;
constexpr float   kInputTurnStep     = 12.0f;
constexpr int32_t kInputAttackDamage = 20;

constexpr int32_t kInitialArmor       = DMCP_ARMOR_GREEN_LIMIT / 2;
constexpr int32_t kInitialAmmoBullets = 50;
constexpr int32_t kInitialAmmoShells  = 8;

constexpr int32_t kInitialEnemyIdBase  = 100;
constexpr int32_t kInitialEntityIdBase = 400;

constexpr int32_t kEnemyHealthDefault     = 60;
constexpr int32_t kEnemyHealthShotgunGuy  = 30;
constexpr int32_t kEnemyHealthDemon       = 150;
constexpr int32_t kEnemyHealthBaronOfHell = 1000;

constexpr int32_t kCheatFlagGodMode = 1;

constexpr uint32_t kTickStep = 1;

constexpr size_t kCommandResultMessageSize = sizeof(dmcp_command_result_t{}.message);

constexpr std::string_view kModeName          = "fake";
constexpr std::string_view kVersionName       = "fake-1.0";
constexpr std::string_view kPlayerStateLive   = "live";
constexpr std::string_view kArmorTypeGreen    = "GreenArmor";
constexpr std::string_view kReadyWeaponPistol = "Pistol";
constexpr std::string_view kPendingWeaponNone = "None";

constexpr std::string_view kLevelId       = "MAP01";
constexpr std::string_view kLevelName     = "Fake Hangar";
constexpr std::string_view kSkillName     = "HurtMePlenty";
constexpr std::string_view kGameStateName = "GS_LEVEL";

constexpr std::array<const char*, 3> kInitialEnemyTypes     = {"DoomImp", "ShotgunGuy", "Demon"};
constexpr std::array<dmcp_vec3_t, 3> kInitialEnemyPositions = {
    dmcp_vec3_t{192.0f, 96.0f, 0.0f},
    dmcp_vec3_t{224.0f, 160.0f, 0.0f},
    dmcp_vec3_t{256.0f, 128.0f, 0.0f},
};

constexpr std::array<const char*, 2> kInitialEntityTypes     = {"ArmorBonus", "HealthBonus"};
constexpr std::array<dmcp_vec3_t, 2> kInitialEntityPositions = {
    dmcp_vec3_t{144.0f, 80.0f, 0.0f},
    dmcp_vec3_t{176.0f, 80.0f, 0.0f},
};

constexpr std::array<const char*, 2> kInitialInventoryItems   = {"Bullets", "Shells"};
constexpr std::array<int32_t, 2>     kInitialInventoryAmounts = {kInitialAmmoBullets,
                                                                 kInitialAmmoShells};

constexpr std::array<int32_t, DMCP_MAX_AMMO_TYPES> kDefaultMaxAmmo = {200, 50, 300, 50};
constexpr std::array<int32_t, DMCP_MAX_AMMO_TYPES> kInitialAmmo    = {kInitialAmmoBullets,
                                                                      kInitialAmmoShells, 0, 0};

constexpr std::array<const char*, 7> kWeaponSlots = {
    "Fist", "Pistol", "Shotgun", "Chaingun", "RocketLauncher", "PlasmaRifle", "BFG9000",
};

constexpr std::array<const char*, 2> kAvailableMaps = {"MAP01", "MAP02"};

dmcp_fake_config_t CopyConfig(const dmcp_fake_config_t* config) {
  dmcp_fake_config_t effective = dmcp_fake_config_default();
  if (!config) {
    return effective;
  }

  size_t copy_size = config->struct_size;
  if (copy_size == 0 || copy_size > sizeof(dmcp_fake_config_t)) {
    copy_size = sizeof(dmcp_fake_config_t);
  }
  mcp_memcpy_safe(&effective, sizeof(effective), config, copy_size);
  effective.struct_size = sizeof(dmcp_fake_config_t);
  return effective;
}

size_t clamp_enemy_capacity(const dmcp_fake_t* fake) {
  const size_t configured = static_cast<size_t>(fake->config.max_enemies);
  if (configured == 0) {
    return DMCP_MAX_ENEMIES;
  }
  return std::min(configured, static_cast<size_t>(DMCP_MAX_ENEMIES));
}

size_t clamp_entity_capacity(const dmcp_fake_t* fake) {
  const size_t configured = static_cast<size_t>(fake->config.max_entities);
  if (configured == 0) {
    return DMCP_MAX_ENTITIES;
  }
  return std::min(configured, static_cast<size_t>(DMCP_MAX_ENTITIES));
}

int32_t default_enemy_health(std::string_view enemy_type) {
  if (enemy_type == "Demon") {
    return kEnemyHealthDemon;
  }
  if (enemy_type == "ShotgunGuy") {
    return kEnemyHealthShotgunGuy;
  }
  if (enemy_type == "BaronOfHell") {
    return kEnemyHealthBaronOfHell;
  }
  return kEnemyHealthDefault;
}

void set_text(char* destination, size_t destination_size, std::string_view value) {
  if (!destination || destination_size == 0) {
    return;
  }
  mcp_strcpy_safe(destination, destination_size, value.data());
}

bool add_enemy(dmcp_fake_t* fake, int32_t enemy_id, std::string_view enemy_type,
               const dmcp_vec3_t& position, float angle_degrees) {
  if (fake->snapshot.enemy_count >= clamp_enemy_capacity(fake)) {
    return false;
  }

  dmcp_enemy_t enemy{};
  enemy.id        = enemy_id;
  enemy.hp        = default_enemy_health(enemy_type);
  enemy.max_hp    = enemy.hp;
  enemy.position  = position;
  enemy.angle     = angle_degrees;
  enemy.target_id = -1;
  set_text(enemy.type, sizeof(enemy.type), enemy_type);

  const bool added = dmcp_snapshot_add_enemy(&fake->snapshot, &enemy);
  if (added) {
    fake->snapshot.level.totalkills = static_cast<int32_t>(fake->snapshot.enemy_count);
  }
  return added;
}

bool add_entity(dmcp_fake_t* fake, int32_t entity_id, std::string_view entity_type,
                const dmcp_vec3_t& position, int32_t health) {
  if (fake->snapshot.entity_count >= clamp_entity_capacity(fake)) {
    return false;
  }

  dmcp_entity_t entity{};
  entity.id       = entity_id;
  entity.hp       = health;
  entity.max_hp   = health;
  entity.position = position;
  entity.angle    = 0.0f;
  set_text(entity.type, sizeof(entity.type), entity_type);

  return dmcp_snapshot_add_entity(&fake->snapshot, &entity);
}

void reset_inventory(dmcp_fake_t* fake) {
  fake->snapshot.inventory_count = 0;
  for (size_t i = 0; i < kInitialInventoryItems.size(); ++i) {
    dmcp_item_t item{};
    set_text(item.name, sizeof(item.name), kInitialInventoryItems[i]);
    item.amount = kInitialInventoryAmounts[i];
    dmcp_snapshot_add_item(&fake->snapshot, &item);
  }
}

void initialize_snapshot(dmcp_fake_t* fake) {
  dmcp_snapshot_clear(&fake->snapshot);

  fake->snapshot.player.hp    = DMCP_PLAYER_INITIAL_HEALTH;
  fake->snapshot.player.armor = kInitialArmor;
  set_text(fake->snapshot.player.armortype, sizeof(fake->snapshot.player.armortype),
           kArmorTypeGreen);
  fake->snapshot.player.position = {kPlayerStartX, kPlayerStartY, kPlayerStartZ};
  fake->snapshot.player.angle    = kPlayerStartAngle;
  set_text(fake->snapshot.player.readyweapon, sizeof(fake->snapshot.player.readyweapon),
           kReadyWeaponPistol);
  set_text(fake->snapshot.player.pendingweapon, sizeof(fake->snapshot.player.pendingweapon),
           kPendingWeaponNone);
  for (size_t i = 0; i < kDefaultMaxAmmo.size(); ++i) {
    fake->snapshot.player.ammo[i]    = kInitialAmmo[i];
    fake->snapshot.player.maxammo[i] = kDefaultMaxAmmo[i];
  }
  fake->snapshot.player.weaponowned[0] = 1;
  fake->snapshot.player.weaponowned[1] = 1;
  set_text(fake->snapshot.player.playerstate, sizeof(fake->snapshot.player.playerstate),
           kPlayerStateLive);

  fake->snapshot.level.tic       = 0;
  fake->snapshot.level.leveltime = 0;
  set_text(fake->snapshot.level.level_id, sizeof(fake->snapshot.level.level_id), kLevelId);
  set_text(fake->snapshot.level.level_name, sizeof(fake->snapshot.level.level_name), kLevelName);
  set_text(fake->snapshot.level.skill, sizeof(fake->snapshot.level.skill), kSkillName);
  set_text(fake->snapshot.level.gamestate, sizeof(fake->snapshot.level.gamestate), kGameStateName);

  set_text(fake->snapshot.game.mode, sizeof(fake->snapshot.game.mode), kModeName);
  set_text(fake->snapshot.game.version, sizeof(fake->snapshot.game.version), kVersionName);
  fake->snapshot.game.consoleplayer   = 0;
  fake->snapshot.game.respawnmonsters = 0;

  fake->snapshot.map_count = 0;
  for (const char* map_name : kAvailableMaps) {
    dmcp_snapshot_add_map(&fake->snapshot, map_name);
  }

  fake->snapshot.enemy_count = 0;
  for (size_t i = 0; i < kInitialEnemyTypes.size(); ++i) {
    const int32_t enemy_id = fake->next_enemy_id + static_cast<int32_t>(i);
    add_enemy(fake, enemy_id, kInitialEnemyTypes[i], kInitialEnemyPositions[i], kEnemySpawnAngle);
  }
  fake->next_enemy_id += static_cast<int32_t>(kInitialEnemyTypes.size());

  fake->snapshot.entity_count = 0;
  for (size_t i = 0; i < kInitialEntityTypes.size(); ++i) {
    const int32_t entity_id = fake->next_entity_id + static_cast<int32_t>(i);
    add_entity(fake, entity_id, kInitialEntityTypes[i], kInitialEntityPositions[i], 1);
  }
  fake->next_entity_id += static_cast<int32_t>(kInitialEntityTypes.size());

  reset_inventory(fake);
  fake->snapshot.level.totalitems = static_cast<int32_t>(fake->snapshot.entity_count);
}

dmcp_enemy_t* find_enemy(dmcp_fake_t* fake, int32_t enemy_id) {
  for (uint32_t i = 0; i < fake->snapshot.enemy_count; ++i) {
    if (fake->snapshot.enemies[i].id == enemy_id) {
      return &fake->snapshot.enemies[i];
    }
  }
  return nullptr;
}

dmcp_enemy_t* find_nearest_alive_enemy(dmcp_fake_t* fake) {
  dmcp_enemy_t* nearest_enemy       = nullptr;
  float         nearest_distance_sq = 0.0f;

  for (uint32_t i = 0; i < fake->snapshot.enemy_count; ++i) {
    dmcp_enemy_t& enemy = fake->snapshot.enemies[i];
    if (enemy.hp <= 0) {
      continue;
    }

    const float dx          = enemy.position.x - fake->snapshot.player.position.x;
    const float dy          = enemy.position.y - fake->snapshot.player.position.y;
    const float distance_sq = dx * dx + dy * dy;

    if (!nearest_enemy || distance_sq < nearest_distance_sq) {
      nearest_enemy       = &enemy;
      nearest_distance_sq = distance_sq;
    }
  }

  return nearest_enemy;
}

void apply_damage_to_enemy(dmcp_fake_t* fake, dmcp_enemy_t* enemy, int32_t damage) {
  if (!enemy || damage <= 0 || enemy->hp <= 0) {
    return;
  }

  const int32_t updated_hp = std::max(enemy->hp - damage, 0);
  if (enemy->hp > 0 && updated_hp == 0) {
    fake->snapshot.level.kill_count += 1;
  }
  enemy->hp = updated_hp;
}

void update_enemy_positions(dmcp_fake_t* fake) {
  for (uint32_t i = 0; i < fake->snapshot.enemy_count; ++i) {
    dmcp_enemy_t& enemy = fake->snapshot.enemies[i];
    if (enemy.hp <= 0) {
      continue;
    }

    const float dx          = fake->snapshot.player.position.x - enemy.position.x;
    const float dy          = fake->snapshot.player.position.y - enemy.position.y;
    const float distance_sq = dx * dx + dy * dy;

    if (distance_sq <= kEnemyContactRangeSq) {
      fake->snapshot.player.hp = std::max(fake->snapshot.player.hp - kEnemyContactDamage, 0);
      continue;
    }

    const float distance = std::sqrt(distance_sq);
    if (distance <= 0.0f) {
      continue;
    }

    const float step_x = (dx / distance) * kEnemyMoveStep;
    const float step_y = (dy / distance) * kEnemyMoveStep;
    enemy.position.x += step_x;
    enemy.position.y += step_y;
    enemy.angle = std::atan2(dy, dx) * kRadiansToDegrees;
  }
}

bool apply_input(dmcp_fake_t* fake, const dmcp_cmd_player_input_t& input, char* message,
                 size_t message_size) {
  switch (input.action) {
    case DMCP_INPUT_FORWARD: {
      fake->snapshot.player.position.y += kInputMoveStep;
      set_text(message, message_size, "Moved forward");
      return true;
    }
    case DMCP_INPUT_BACKWARD: {
      fake->snapshot.player.position.y -= kInputMoveStep;
      set_text(message, message_size, "Moved backward");
      return true;
    }
    case DMCP_INPUT_STRAFE_LEFT: {
      fake->snapshot.player.position.x -= kInputMoveStep;
      set_text(message, message_size, "Strafed left");
      return true;
    }
    case DMCP_INPUT_STRAFE_RIGHT: {
      fake->snapshot.player.position.x += kInputMoveStep;
      set_text(message, message_size, "Strafed right");
      return true;
    }
    case DMCP_INPUT_TURN_LEFT: {
      fake->snapshot.player.angle -= kInputTurnStep;
      set_text(message, message_size, "Turned left");
      return true;
    }
    case DMCP_INPUT_TURN_RIGHT: {
      fake->snapshot.player.angle += kInputTurnStep;
      set_text(message, message_size, "Turned right");
      return true;
    }
    case DMCP_INPUT_AIM: {
      fake->snapshot.player.angle = input.aim_angle;
      set_text(message, message_size, "Updated aim angle");
      return true;
    }
    case DMCP_INPUT_ATTACK: {
      dmcp_enemy_t* enemy = find_nearest_alive_enemy(fake);
      if (!enemy) {
        set_text(message, message_size, "No target to attack");
        return false;
      }
      apply_damage_to_enemy(fake, enemy, kInputAttackDamage);
      set_text(message, message_size, "Attack applied");
      return true;
    }
    case DMCP_INPUT_USE: {
      fake->snapshot.level.item_count += 1;
      set_text(message, message_size, "Use action applied");
      return true;
    }
    case DMCP_INPUT_WEAPON: {
      if (input.weapon_slot <= 0 || static_cast<size_t>(input.weapon_slot) > kWeaponSlots.size()) {
        set_text(message, message_size, "Invalid weapon slot");
        return false;
      }
      const size_t slot_index = static_cast<size_t>(input.weapon_slot - 1);
      set_text(fake->snapshot.player.readyweapon, sizeof(fake->snapshot.player.readyweapon),
               kWeaponSlots[slot_index]);
      set_text(message, message_size, "Weapon switched");
      return true;
    }
    case DMCP_INPUT_NONE:
    default: {
      set_text(message, message_size, "Unsupported input action");
      return false;
    }
  }
}

bool apply_command(dmcp_fake_t* fake, const dmcp_command_t& cmd, char* message,
                   size_t message_size) {
  switch (cmd.type) {
    case DMCP_CMD_SPAWN_ENTITY: {
      const int32_t     requested_id = cmd.data.spawn.tid;
      const int32_t     enemy_id     = requested_id > 0 ? requested_id : fake->next_enemy_id++;
      const dmcp_vec3_t position     = {cmd.data.spawn.position.x, cmd.data.spawn.position.y,
                                        kPlayerStartZ};
      const bool        added =
          add_enemy(fake, enemy_id, cmd.data.spawn.entity_class, position, cmd.data.spawn.angle);
      set_text(message, message_size, added ? "Entity spawned" : "Enemy capacity reached");
      return added;
    }
    case DMCP_CMD_CHANGE_LEVEL: {
      if (cmd.data.change_level.map_name[0] == '\0') {
        set_text(message, message_size, "Missing level name");
        return false;
      }

      set_text(fake->snapshot.level.level_id, sizeof(fake->snapshot.level.level_id),
               cmd.data.change_level.map_name);
      set_text(fake->snapshot.level.level_name, sizeof(fake->snapshot.level.level_name),
               cmd.data.change_level.map_name);

      fake->snapshot.level.tic          = 0;
      fake->snapshot.level.leveltime    = 0;
      fake->snapshot.level.kill_count   = 0;
      fake->snapshot.level.item_count   = 0;
      fake->snapshot.level.secret_count = 0;

      set_text(message, message_size, "Level changed");
      return true;
    }
    case DMCP_CMD_GIVE_ITEM: {
      bool merged = false;
      for (uint32_t i = 0; i < fake->snapshot.inventory_count; ++i) {
        dmcp_item_t& item = fake->snapshot.inventory[i];
        if (std::strcmp(item.name, cmd.data.give_item.item_class) == 0) {
          item.amount += std::max(cmd.data.give_item.amount, 1);
          merged = true;
          break;
        }
      }

      if (!merged) {
        dmcp_item_t item{};
        set_text(item.name, sizeof(item.name), cmd.data.give_item.item_class);
        item.amount = std::max(cmd.data.give_item.amount, 1);
        if (!dmcp_snapshot_add_item(&fake->snapshot, &item)) {
          set_text(message, message_size, "Inventory capacity reached");
          return false;
        }
      }

      set_text(message, message_size, "Item granted");
      return true;
    }
    case DMCP_CMD_SET_PLAYER_HEALTH: {
      const int32_t clamped_health =
          std::clamp(cmd.data.set_health.health, DMCP_ITEM_HEALTH_BONUS, DMCP_PLAYER_MAX_HEALTH);
      fake->snapshot.player.hp = clamped_health;
      set_text(message, message_size, "Player health updated");
      return true;
    }
    case DMCP_CMD_SET_PLAYER_POSITION: {
      fake->snapshot.player.position.x = cmd.data.set_position.position.x;
      fake->snapshot.player.position.y = cmd.data.set_position.position.y;
      fake->snapshot.player.angle      = cmd.data.set_position.angle;
      set_text(message, message_size, "Player position updated");
      return true;
    }
    case DMCP_CMD_EXECUTE_CONSOLE: {
      const std::string_view console_command(cmd.data.console.command);
      if (console_command == "iddqd") {
        fake->snapshot.player.cheats |= kCheatFlagGodMode;
        fake->snapshot.player.hp = DMCP_PLAYER_GOD_HEALTH;
      } else if (console_command == "give all") {
        for (size_t i = 0; i < kDefaultMaxAmmo.size(); ++i) {
          fake->snapshot.player.ammo[i] = fake->snapshot.player.maxammo[i];
        }
      } else if (console_command == "killall") {
        for (uint32_t i = 0; i < fake->snapshot.enemy_count; ++i) {
          if (fake->snapshot.enemies[i].hp > 0) {
            fake->snapshot.level.kill_count += 1;
            fake->snapshot.enemies[i].hp = 0;
          }
        }
      }

      set_text(message, message_size, "Console command executed");
      return true;
    }
    case DMCP_CMD_PAUSE_GAME: {
      fake->paused                = cmd.data.pause.paused;
      fake->snapshot.level.paused = fake->paused ? 1 : 0;
      set_text(message, message_size, fake->paused ? "Game paused" : "Game resumed");
      return true;
    }
    case DMCP_CMD_DAMAGE_ENTITY: {
      dmcp_enemy_t* enemy = find_enemy(fake, cmd.data.damage.target_tid);
      if (!enemy) {
        set_text(message, message_size, "Target entity not found");
        return false;
      }
      apply_damage_to_enemy(fake, enemy, cmd.data.damage.damage);
      set_text(message, message_size, "Damage applied");
      return true;
    }
    case DMCP_CMD_KILL_ENTITY: {
      dmcp_enemy_t* enemy = find_enemy(fake, cmd.data.kill.target_tid);
      if (!enemy) {
        set_text(message, message_size, "Target entity not found");
        return false;
      }
      apply_damage_to_enemy(fake, enemy, enemy->hp);
      set_text(message, message_size, "Entity killed");
      return true;
    }
    case DMCP_CMD_PLAYER_INPUT: {
      return apply_input(fake, cmd.data.input, message, message_size);
    }
    case DMCP_CMD_UNKNOWN:
    default: {
      set_text(message, message_size, "Unsupported command type");
      return false;
    }
  }
}

bool ExecuteQueuedCommand(void* adapter_ctx, const dmcp_command_t* cmd, char* out_message,
                          size_t out_message_size) {
  auto* fake = static_cast<dmcp_fake_t*>(adapter_ctx);
  if (!fake || !cmd) {
    set_text(out_message, out_message_size, "Invalid command");
    return false;
  }

  std::lock_guard<std::mutex> lock(fake->state_mutex);
  return apply_command(fake, *cmd, out_message, out_message_size);
}

bool ExecuteQueuedInput(void* adapter_ctx, const dmcp_command_t* input_cmd, char* out_message,
                        size_t out_message_size) {
  auto* fake = static_cast<dmcp_fake_t*>(adapter_ctx);
  if (!fake || !input_cmd || input_cmd->type != DMCP_CMD_PLAYER_INPUT) {
    set_text(out_message, out_message_size, "Invalid player input");
    return false;
  }

  std::lock_guard<std::mutex> lock(fake->state_mutex);
  return apply_input(fake, input_cmd->data.input, out_message, out_message_size);
}

void fake_snapshot_callback(void* user_data, dmcp_snapshot_t* snapshot) {
  if (!user_data || !snapshot) {
    return;
  }

  auto*                       fake = static_cast<dmcp_fake_t*>(user_data);
  std::lock_guard<std::mutex> lock(fake->state_mutex);
  *snapshot = fake->snapshot;
}

void advance_simulation(dmcp_fake_t* fake) {
  if (!fake || fake->paused) {
    return;
  }

  fake->simulated_ticks += kTickStep;
  fake->snapshot.level.tic += static_cast<int32_t>(kTickStep);
  fake->snapshot.level.leveltime += static_cast<int32_t>(kTickStep);

  update_enemy_positions(fake);
}

}  // namespace

extern "C" {

dmcp_fake_t* dmcp_fake_create(const dmcp_fake_config_t* config) {
  std::unique_ptr<dmcp_fake_s> fake;
  try {
    fake = std::make_unique<dmcp_fake_s>();
  } catch (const std::bad_alloc&) {
    return nullptr;
  }

  fake->config = CopyConfig(config);

  fake->config.base.struct_size = sizeof(dmcp_config_t);
  fake->config.base.on_snapshot = fake_snapshot_callback;
  fake->config.base.user_data   = fake.get();

  fake->rng.seed(fake->config.seed);
  fake->next_enemy_id  = kInitialEnemyIdBase;
  fake->next_entity_id = kInitialEntityIdBase;

  initialize_snapshot(fake.get());

  std::unique_ptr<dmcp_context_t, decltype(&dmcp_context_destroy)> dmcp_ctx(
      dmcp_context_create(&fake->config.base), dmcp_context_destroy);
  if (!dmcp_ctx) {
    return nullptr;
  }

  fake->dmcp_ctx = dmcp_ctx.release();
  return fake.release();
}

void dmcp_fake_destroy(dmcp_fake_t* fake) {
  if (!fake) {
    return;
  }
  std::unique_ptr<dmcp_fake_s> owned_fake(fake);

  if (owned_fake->dmcp_ctx) {
    dmcp_context_destroy(owned_fake->dmcp_ctx);
    owned_fake->dmcp_ctx = nullptr;
  }
}

mcp_status_t dmcp_fake_tick(dmcp_fake_t* fake) {
  if (!fake || !fake->dmcp_ctx) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  if (fake->config.base.start_transport && !dmcp_context_is_running(fake->dmcp_ctx)) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_DISABLED, "Operation disabled");
  }

  dmcp_fake_inputs_process(fake);
  dmcp_fake_commands_process(fake);

  {
    std::lock_guard<std::mutex> lock(fake->state_mutex);
    advance_simulation(fake);
  }

  dmcp_context_tick(fake->dmcp_ctx);
  return MCP_STATUS_OK("Success");
}

void dmcp_fake_commands_process(dmcp_fake_t* fake) {
  if (!fake || !fake->dmcp_ctx) {
    return;
  }

  dmcp_adapter_process_command_queue(fake->dmcp_ctx, fake, ExecuteQueuedCommand, 0);
}

void dmcp_fake_inputs_process(dmcp_fake_t* fake) {
  if (!fake || !fake->dmcp_ctx) {
    return;
  }

  dmcp_adapter_process_input_queue(fake->dmcp_ctx, fake, ExecuteQueuedInput, 0);
}

bool dmcp_fake_command_execute(dmcp_fake_t* fake, const dmcp_command_t* cmd) {
  char command_message[kCommandResultMessageSize] = {0};
  return ExecuteQueuedCommand(fake, cmd, command_message, sizeof(command_message));
}

bool dmcp_fake_is_running(const dmcp_fake_t* fake) {
  if (!fake || !fake->dmcp_ctx) {
    return false;
  }
  if (!fake->config.base.start_transport) {
    return true;
  }
  return dmcp_context_is_running(fake->dmcp_ctx);
}

dmcp_context_t* dmcp_fake_get_context(dmcp_fake_t* fake) {
  if (!fake) {
    return nullptr;
  }
  return fake->dmcp_ctx;
}

void dmcp_fake_get_stats(dmcp_fake_t* fake, dmcp_stats_t* out_stats) {
  if (!fake || !out_stats || !fake->dmcp_ctx) {
    return;
  }
  dmcp_stats_get(fake->dmcp_ctx, out_stats);
}

bool dmcp_fake_snapshot_get(const dmcp_fake_t* fake, dmcp_snapshot_t* out_snapshot) {
  if (!fake || !out_snapshot) {
    return false;
  }

  std::lock_guard<std::mutex> lock(fake->state_mutex);
  *out_snapshot = fake->snapshot;
  return true;
}

}  // extern "C"
