#include <cstdio>
#include <cstring>

#include "dmcp_adapter.h"
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

static void CopyCommandMessage(char* out, size_t out_size, const char* message) {
  dmcp_strcpy_safe(out, message ? message : "", out_size);
}

static player_t* GetConsolePlayer() {
  if (consoleplayer < 0 || consoleplayer >= MAXPLAYERS) return nullptr;
  return &players[consoleplayer];
}

static AActor* FindActorByTid(int tid) {
  TThinkerIterator<AActor> it;
  while (AActor* actor = it.Next()) {
    if (actor->tid == tid) {
      return actor;
    }
  }
  return nullptr;
}

// Helper to execute console commands
static void ExecuteConsoleCommand(const char* cmd) {
  if (!cmd || !cmd[0]) return;

  // Add command to console buffer
  C_DoCommand(cmd, false);
}

// Helper to spawn an entity
static bool SpawnEntity(const dmcp_cmd_spawn_t* spawn) {
  if (!spawn) return false;

  // Build spawn command
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "summon %s %f %f %f", spawn->entity_class, spawn->position.x,
           spawn->position.y, spawn->angle);

  ExecuteConsoleCommand(cmd);
  return true;
}

// Helper to change level
static bool ChangeLevel(const dmcp_cmd_change_level_t* change) {
  if (!change || !change->map_name[0]) return false;

  char cmd[128];
  if (change->reset_inventory) {
    snprintf(cmd, sizeof(cmd), "map %s %d", change->map_name, change->skill_level);
  } else {
    snprintf(cmd, sizeof(cmd), "changemap %s", change->map_name);
  }

  ExecuteConsoleCommand(cmd);
  return true;
}

// Helper to give item
static bool GiveItem(const dmcp_cmd_give_item_t* give) {
  if (!give || !give->item_class[0]) return false;

  char cmd[256];
  snprintf(cmd, sizeof(cmd), "give %s", give->item_class);
  ExecuteConsoleCommand(cmd);

  // If amount > 1, we might need additional handling depending on the item
  return true;
}

// Helper to set player health
static bool SetPlayerHealth(AdapterContext* ctx, const dmcp_cmd_set_health_t* health) {
  if (!health) {
    Log(ctx, MCP_LOG_WARN, "SetPlayerHealth: null command");
    return false;
  }

  // Validate health value using adapter helper
  int health_value = static_cast<int>(health->health);
  if (!dmcp_validate_health(health_value)) {
    Log(ctx, MCP_LOG_WARN, "SetPlayerHealth: health %d out of range (1-%d)", health_value,
        DMCP_PLAYER_MAX_HEALTH);
    return false;
  }

  player_t* player = GetConsolePlayer();
  if (!player || !player->mo) {
    Log(ctx, MCP_LOG_WARN, "SetPlayerHealth: no player or mobj");
    return false;
  }

  // Direct health modification
  player->mo->health = health_value;
  player->health     = health_value;

  return true;
}

// Helper to set player position
static bool SetPlayerPosition(const dmcp_cmd_set_position_t* pos) {
  if (!pos) return false;

  player_t* player = GetConsolePlayer();
  if (!player || !player->mo) return false;

  // Teleport player to new position
  player->mo->SetOrigin(DVector3(pos->position.x, pos->position.y, player->mo->Z()), false);

  // Set angle
  player->mo->Angles.Yaw = DAngle::fromDeg(pos->angle);

  return true;
}

// Helper to pause/unpause game
static bool PauseGame(const dmcp_cmd_pause_t* pause) {
  if (!pause) return false;

  if (paused != pause->paused) {
    ExecuteConsoleCommand("pause");
  }
  return true;
}

// Helper to set timescale
static bool SetTimescale(const dmcp_cmd_timescale_t* timescale) {
  if (!timescale || !dmcp_validate_timescale(timescale->scale)) {
    return false;
  }

  char cmd[64];
  snprintf(cmd, sizeof(cmd), "timescale %f", timescale->scale);
  ExecuteConsoleCommand(cmd);
  return true;
}

// Helper to damage entity
static bool DamageEntity(AdapterContext* ctx, const dmcp_cmd_damage_t* damage) {
  if (!damage) {
    Log(ctx, MCP_LOG_WARN, "DamageEntity: null command");
    return false;
  }

  // Validate damage amount using adapter helper
  int damage_value = static_cast<int>(damage->damage);
  if (!dmcp_validate_damage(damage_value)) {
    Log(ctx, MCP_LOG_WARN, "DamageEntity: damage %d out of range (1-%d)", damage_value,
        DMCP_DAMAGE_MAX);
    return false;
  }

  AActor* actor = FindActorByTid(damage->target_tid);
  if (!actor) {
    Log(ctx, MCP_LOG_WARN, "DamageEntity: target tid %d not found", damage->target_tid);
    return false;
  }

  P_DamageMobj(actor, nullptr, nullptr, damage_value, damage->damage_type);
  return true;
}

// Helper to kill entity
static bool KillEntity(const dmcp_cmd_kill_t* kill) {
  if (!kill) return false;

  AActor* actor = FindActorByTid(kill->target_tid);
  if (!actor) {
    return false;
  }

  P_DamageMobj(actor, nullptr, nullptr,
               actor->health * 2,  // Overkill
               NAME_None);
  return true;
}

static bool ExecuteQueuedCommand(void* adapter_ctx, const dmcp_command_t* cmd, char* out_message,
                                 size_t out_message_size) {
  auto* ctx = static_cast<AdapterContext*>(adapter_ctx);
  bool  success;

  if (!ctx || !cmd) {
    CopyCommandMessage(out_message, out_message_size, "Invalid command");
    return false;
  }

  success = dmcp_zdoom_command_execute(reinterpret_cast<dmcp_zdoom_t*>(ctx), cmd);
  if (success) {
    CopyCommandMessage(out_message, out_message_size, "Command executed");
  } else {
    CopyCommandMessage(out_message, out_message_size, "Command failed in engine");
    Log(ctx, MCP_LOG_WARN, "Command %d failed", cmd->type);
  }
  return success;
}

static bool ExecuteQueuedInput(void* adapter_ctx, const dmcp_command_t* input_cmd,
                               char* out_message, size_t out_message_size) {
  auto* ctx = static_cast<AdapterContext*>(adapter_ctx);
  (void)input_cmd;

  CopyCommandMessage(out_message, out_message_size,
                     "player_input is not supported by zdoom adapter");
  if (ctx) {
    Log(ctx, MCP_LOG_DEBUG, "player_input not supported");
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
      return SpawnEntity(&cmd->data.spawn);

    case DMCP_CMD_CHANGE_LEVEL:
      return ChangeLevel(&cmd->data.change_level);

    case DMCP_CMD_GIVE_ITEM:
      return GiveItem(&cmd->data.give_item);

    case DMCP_CMD_SET_PLAYER_HEALTH:
      return SetPlayerHealth(ctx, &cmd->data.set_health);

    case DMCP_CMD_SET_PLAYER_POSITION:
      return SetPlayerPosition(&cmd->data.set_position);

    case DMCP_CMD_EXECUTE_CONSOLE:
      ExecuteConsoleCommand(cmd->data.console.command);
      return true;

    case DMCP_CMD_PAUSE_GAME:
      return PauseGame(&cmd->data.pause);

    case DMCP_CMD_SET_TIMESCALE:
      return SetTimescale(&cmd->data.timescale);

    case DMCP_CMD_DAMAGE_ENTITY:
      return DamageEntity(ctx, &cmd->data.damage);

    case DMCP_CMD_KILL_ENTITY:
      return KillEntity(&cmd->data.kill);

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

  int processed = dmcp_adapter_process_command_queue(ctx->dmcp_ctx, ctx, ExecuteQueuedCommand, 0);
  if (processed > 0) {
    Log(ctx, MCP_LOG_INFO, "processed %d command(s)", processed);
  }
}

void dmcp_zdoom_inputs_process(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx) return;

  dmcp_adapter_process_input_queue(ctx->dmcp_ctx, ctx, ExecuteQueuedInput, 0);
}
