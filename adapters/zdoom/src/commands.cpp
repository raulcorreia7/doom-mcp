#include <cstdio>
#include <cstring>

#include "dmcp_zdoom.h"
#include "internal.h"

#include "dmcp_adapter_command_queue.h"
#include "dmcp/adapter/utils.h"
#include "dmcp/adapter/validation.h"
#include "dmcp/doom/constants.h"

// ZDoom headers
#include "common/console/c_console.h"
#include "common/engine/printf.h"
#include "common/utility/cmdlib.h"
#include "doomstat.h"
#include "g_levellocals.h"
#include "gamedata/a_weapons.h"
#include "gamedata/gi.h"
#include "playsim/d_player.h"
#include "playsim/dthinker.h"
#include "playsim/p_local.h"

using namespace dmcp::zdoom;

namespace {

static void copy_command_message(char* out, size_t out_size, const char* message) {
  mcp_strcpy_safe(out, out_size, message ? message : "");
}

static player_t* get_console_player() {
  if (consoleplayer < 0 || consoleplayer >= MAXPLAYERS) return nullptr;
  return &players[consoleplayer];
}

static AActor* find_actor_by_tid(int tid) {
  TThinkerIterator<AActor> it;
  while (AActor* actor = it.Next()) {
    if (actor->tid == tid) {
      return actor;
    }
  }
  return nullptr;
}

// Helper to execute console commands
static void execute_console_command(const char* cmd) {
  if (!cmd || !cmd[0]) return;

  // Add command to console buffer
  C_DoCommand(cmd, false);
}

// Helper to spawn an entity
static bool spawn_entity(const dmcp_cmd_spawn_t* spawn) {
  if (!spawn) return false;

  // Build spawn command
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "summon %s %f %f %f", spawn->entity_class, spawn->position.x,
           spawn->position.y, spawn->angle);

  execute_console_command(cmd);
  return true;
}

// Helper to change level
static bool change_level(const dmcp_cmd_change_level_t* change) {
  if (!change || !change->map_name[0]) return false;

  char cmd[128];
  if (change->reset_inventory) {
    snprintf(cmd, sizeof(cmd), "map %s %d", change->map_name, change->skill_level);
  } else {
    snprintf(cmd, sizeof(cmd), "changemap %s", change->map_name);
  }

  execute_console_command(cmd);
  return true;
}

// Helper to give item
static bool give_item(const dmcp_cmd_give_item_t* give) {
  if (!give || !give->item_class[0]) return false;

  char cmd[256];
  snprintf(cmd, sizeof(cmd), "give %s", give->item_class);
  execute_console_command(cmd);

  // If amount > 1, we might need additional handling depending on the item
  return true;
}

// Helper to set player health
static bool set_player_health(AdapterContext* ctx, const dmcp_cmd_set_health_t* health) {
  if (!health) {
    adapter_log(ctx, MCP_LOG_WARN, "set_player_health: null command");
    return false;
  }

  // Validate health value using adapter helper
  int health_value = static_cast<int>(health->health);
  if (!dmcp_validate_health(health_value)) {
    adapter_log(ctx, MCP_LOG_WARN, "set_player_health: health %d out of range (1-%d)", health_value,
                DMCP_PLAYER_MAX_HEALTH);
    return false;
  }

  player_t* player = get_console_player();
  if (!player || !player->mo) {
    adapter_log(ctx, MCP_LOG_WARN, "set_player_health: no player or mobj");
    return false;
  }

  // Direct health modification
  player->mo->health = health_value;
  player->health     = health_value;

  return true;
}

// Helper to set player position
static bool set_player_position(const dmcp_cmd_set_position_t* pos) {
  if (!pos) return false;

  player_t* player = get_console_player();
  if (!player || !player->mo) return false;

  // Teleport player to new position
  player->mo->SetOrigin(DVector3(pos->position.x, pos->position.y, player->mo->Z()), false);

  // Set angle
  player->mo->Angles.Yaw = DAngle::fromDeg(pos->angle);

  return true;
}

// Helper to pause/unpause game
static bool pause_game(const dmcp_cmd_pause_t* pause) {
  if (!pause) return false;

  if (paused != pause->paused) {
    execute_console_command("pause");
  }
  return true;
}

// Helper to damage entity
static bool damage_entity(AdapterContext* ctx, const dmcp_cmd_damage_t* damage) {
  if (!damage) {
    adapter_log(ctx, MCP_LOG_WARN, "damage_entity: null command");
    return false;
  }

  // Validate damage amount using adapter helper
  int damage_value = static_cast<int>(damage->damage);
  if (!dmcp_validate_damage(damage_value)) {
    adapter_log(ctx, MCP_LOG_WARN, "damage_entity: damage %d out of range (1-%d)", damage_value,
                DMCP_DAMAGE_MAX);
    return false;
  }

  AActor* actor = find_actor_by_tid(damage->target_tid);
  if (!actor) {
    adapter_log(ctx, MCP_LOG_WARN, "damage_entity: target tid %d not found", damage->target_tid);
    return false;
  }

  P_DamageMobj(actor, nullptr, nullptr, damage_value, damage->damage_type);
  return true;
}

// Helper to kill entity
static bool kill_entity(const dmcp_cmd_kill_t* kill) {
  if (!kill) return false;

  AActor* actor = find_actor_by_tid(kill->target_tid);
  if (!actor) {
    return false;
  }

  P_DamageMobj(actor, nullptr, nullptr,
               actor->health * 2,  // Overkill
               NAME_None);
  return true;
}

static bool execute_queued_command(void* adapter_ctx, const dmcp_command_t* cmd, char* out_message,
                                   size_t out_message_size) {
  auto* ctx = static_cast<AdapterContext*>(adapter_ctx);
  bool  success;

  if (!ctx || !cmd) {
    copy_command_message(out_message, out_message_size, "Invalid command");
    return false;
  }

  success = dmcp_zdoom_command_execute(reinterpret_cast<dmcp_zdoom_t*>(ctx), cmd);
  if (success) {
    copy_command_message(out_message, out_message_size, "Command executed");
  } else {
    copy_command_message(out_message, out_message_size, "Command failed in engine");
    adapter_log(ctx, MCP_LOG_WARN, "Command %d failed", cmd->type);
  }
  return success;
}

static bool execute_queued_input(void* adapter_ctx, const dmcp_command_t* input_cmd,
                                 char* out_message, size_t out_message_size) {
  auto* ctx = static_cast<AdapterContext*>(adapter_ctx);
  (void)input_cmd;

  copy_command_message(out_message, out_message_size,
                       "player_input is not supported by zdoom adapter");
  if (ctx) {
    adapter_log(ctx, MCP_LOG_DEBUG, "player_input not supported");
  }
  return false;
}

}  // namespace

// ============================================================================
// Public API
// ============================================================================

bool dmcp_zdoom_command_execute(dmcp_zdoom_t* ctx_handle, const dmcp_command_t* cmd) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx || !cmd) return false;

  switch (cmd->type) {
    case DMCP_CMD_SPAWN_ENTITY:
      return spawn_entity(&cmd->data.spawn);

    case DMCP_CMD_CHANGE_LEVEL:
      return change_level(&cmd->data.change_level);

    case DMCP_CMD_GIVE_ITEM:
      return give_item(&cmd->data.give_item);

    case DMCP_CMD_SET_PLAYER_HEALTH:
      return set_player_health(ctx, &cmd->data.set_health);

    case DMCP_CMD_SET_PLAYER_POSITION:
      return set_player_position(&cmd->data.set_position);

    case DMCP_CMD_EXECUTE_CONSOLE:
      execute_console_command(cmd->data.console.command);
      return true;

    case DMCP_CMD_PAUSE_GAME:
      return pause_game(&cmd->data.pause);

    case DMCP_CMD_DAMAGE_ENTITY:
      return damage_entity(ctx, &cmd->data.damage);

    case DMCP_CMD_KILL_ENTITY:
      return kill_entity(&cmd->data.kill);

    default:
      return false;
  }
}

// ============================================================================
// Command Processing
// ============================================================================

void dmcp_zdoom_commands_process(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx) return;

  int processed = dmcp_adapter_process_command_queue(ctx->dmcp_ctx, ctx, execute_queued_command, 0);
  if (processed > 0) {
    adapter_log(ctx, MCP_LOG_DEBUG, "processed %d command(s)", processed);
  }
}

void dmcp_zdoom_inputs_process(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx) return;

  dmcp_adapter_process_input_queue(ctx->dmcp_ctx, ctx, execute_queued_input, 0);
}
