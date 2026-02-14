// Chocolate Doom Adapter Implementation
// Bridges DMCP SDK with Chocolate Doom game state

#include "adapter.h"

#include <stdlib.h>
#include <string.h>

#include "dmcp/doom/api.h"

// Chocolate Doom includes - these are relative to chocolate-doom/src/
// When building, include paths will be set by CMake
#ifdef CHOCOLATE_DOOM_BUILD
#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "info.h"
#include "p_mobj.h"
#endif

struct dmcp_chocolate_s {
  dmcp_context_t*         dmcp_ctx;
  dmcp_chocolate_config_t config;
  bool                    initialized;
};

// Enemy type name lookup
static const char* get_enemy_type_name(int mobj_type) {
#ifdef CHOCOLATE_DOOM_BUILD
  switch (mobj_type) {
    case MT_POSSESSED:
      return "Zombieman";
    case MT_SHOTGUY:
      return "ShotgunGuy";
    case MT_VILE:
      return "Archvile";
    case MT_UNDEAD:
      return "Revenant";
    case MT_FATSO:
      return "Mancubus";
    case MT_CHAINGUY:
      return "Chaingunner";
    case MT_TROOP:
      return "Imp";
    case MT_SERGEANT:
      return "Demon";
    case MT_SHADOWS:
      return "Spectre";
    case MT_HEAD:
      return "Cacodemon";
    case MT_BRUISER:
      return "BaronOfHell";
    case MT_KNIGHT:
      return "HellKnight";
    case MT_SKULL:
      return "LostSoul";
    case MT_SPIDER:
      return "SpiderMastermind";
    case MT_BABY:
      return "Arachnotron";
    case MT_CYBORG:
      return "Cyberdemon";
    case MT_PAIN:
      return "PainElemental";
    default:
      return "Unknown";
  }
#else
  (void)mobj_type;
  return "Unknown";
#endif
}

// Snapshot callback for DMCP
static void snapshot_callback(void* user_data, dmcp_snapshot_t* snap) {
  (void)user_data;
  if (!snap) return;

#ifdef CHOCOLATE_DOOM_BUILD
  dmcp_chocolate_populate_player(snap);
  dmcp_chocolate_populate_level(snap);
  dmcp_chocolate_populate_enemies(snap);
#else
  (void)snap;
#endif
}

dmcp_chocolate_t* dmcp_chocolate_create(const dmcp_chocolate_config_t* config) {
  if (!config) return NULL;

  dmcp_chocolate_t* ctx =
      (dmcp_chocolate_t*)calloc(1, sizeof(dmcp_chocolate_t));
  if (!ctx) return NULL;

  ctx->config = *config;

  // Set up DMCP config with our callback
  dmcp_config_t dmcp_cfg = config->base;
  dmcp_cfg.on_snapshot   = snapshot_callback;
  dmcp_cfg.user_data     = ctx;

  // Create DMCP context
  ctx->dmcp_ctx = dmcp_context_create(&dmcp_cfg);
  if (!ctx->dmcp_ctx) {
    free(ctx);
    return NULL;
  }

  ctx->initialized = true;
  return ctx;
}

void dmcp_chocolate_destroy(dmcp_chocolate_t* ctx) {
  if (!ctx) return;

  if (ctx->dmcp_ctx) {
    dmcp_context_destroy(ctx->dmcp_ctx);
  }

  free(ctx);
}

void dmcp_chocolate_tick(dmcp_chocolate_t* ctx) {
  if (!ctx || !ctx->dmcp_ctx || !ctx->initialized) return;

#ifdef CHOCOLATE_DOOM_BUILD
  // Only process if in a level
  if (gamestate != GS_LEVEL) return;
  if (paused) return;
#endif

  dmcp_context_tick(ctx->dmcp_ctx);
}

void dmcp_chocolate_commands_process(dmcp_chocolate_t* ctx) {
  if (!ctx || !ctx->dmcp_ctx) return;

  dmcp_command_t cmd;
  while (dmcp_pop_command(ctx->dmcp_ctx, &cmd)) {
    dmcp_chocolate_command_execute(ctx, &cmd);
  }
}

bool dmcp_chocolate_is_running(const dmcp_chocolate_t* ctx) {
  if (!ctx || !ctx->dmcp_ctx) return false;
  return dmcp_context_is_running(ctx->dmcp_ctx);
}

void dmcp_chocolate_get_stats(dmcp_chocolate_t* ctx, dmcp_stats_t* stats) {
  if (!ctx || !ctx->dmcp_ctx || !stats) return;
  dmcp_stats_get(ctx->dmcp_ctx, stats);
}

bool dmcp_chocolate_command_execute(dmcp_chocolate_t*     ctx,
                                    const dmcp_command_t* cmd) {
  if (!ctx || !cmd) return false;

#ifdef CHOCOLATE_DOOM_BUILD
  switch (cmd->type) {
    case DMCP_CMD_PAUSE_GAME:
      if (gamestate == GS_LEVEL) {
	paused = cmd->data.pause_game.paused;
	return true;
      }
      break;

    case DMCP_CMD_SET_PLAYER_HEALTH: {
      player_t* p = &players[consoleplayer];
      if (p && playeringame[consoleplayer]) {
	p->health = (int)cmd->data.set_player_health.health;
	if (p->mo) {
	  p->mo->health = p->health;
	}
	return true;
      }
      break;
    }

    default:
      break;
  }
#else
  (void)ctx;
  (void)cmd;
#endif

  return false;
}

void dmcp_chocolate_populate_player(dmcp_snapshot_t* snap) {
  if (!snap) return;

#ifdef CHOCOLATE_DOOM_BUILD
  if (!playeringame[consoleplayer]) return;

  player_t* p = &players[consoleplayer];
  if (!p || !p->mo) return;

  // Convert from fixed_t (16.16) to float
  snap->player.position.x = FIXED_TO_FLOAT(p->mo->x);
  snap->player.position.y = FIXED_TO_FLOAT(p->mo->y);
  snap->player.hp         = (float)p->health;
  snap->player.armor      = (float)p->armorpoints;
  snap->player.ammo       = p->ammo[am_clip];
#endif
}

void dmcp_chocolate_populate_level(dmcp_snapshot_t* snap) {
  if (!snap) return;

#ifdef CHOCOLATE_DOOM_BUILD
  snap->level.tic = leveltime;

  // Level ID in E1M1 format
  snprintf(snap->level.level_id, MCP_MAX_LEVEL_ID, "E%dM%d", gameepisode,
           gamemap);

  player_t* p = &players[consoleplayer];
  if (p && playeringame[consoleplayer]) {
    snap->level.kill_count   = p->killcount;
    snap->level.item_count   = p->itemcount;
    snap->level.secret_count = p->secretcount;
  }
#endif
}

int dmcp_chocolate_populate_enemies(dmcp_snapshot_t* snap) {
  int count = 0;
  if (!snap) return 0;

#ifdef CHOCOLATE_DOOM_BUILD
  thinker_t* th;

  for (th = thinkercap.next; th != &thinkercap && count < DMCP_MAX_ENEMIES;
       th = th->next) {
    mobj_t* mo = (mobj_t*)th;

    // Check if it's a living enemy
    if ((mo->flags & MF_COUNTKILL) && !(mo->flags & MF_CORPSE) &&
        mo->health > 0) {
      dmcp_enemy_t enemy = {0};
      enemy.id           = count;
      enemy.hp           = (float)mo->health;
      enemy.max_hp       = (float)mo->info->spawnhealth;
      enemy.position.x   = FIXED_TO_FLOAT(mo->x);
      enemy.position.y   = FIXED_TO_FLOAT(mo->y);

      const char* type = get_enemy_type_name(mo->type);
      dmcp_strcpy(enemy.type, type, sizeof(enemy.type));

      dmcp_snapshot_add_enemy(snap, &enemy);
      count++;
    }
  }
#endif

  return count;
}
