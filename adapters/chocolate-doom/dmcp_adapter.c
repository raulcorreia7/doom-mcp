// Chocolate Doom Adapter - Lifecycle
// Create, destroy, tick, and stats for DMCP integration

#include "dmcp_adapter.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dmcp/doom/api.h"

#include "doomdef.h"
#include "doomstat.h"

struct dmcp_chocolate_s {
  dmcp_context_t*         dmcp_ctx;
  dmcp_chocolate_config_t config;
  bool                    initialized;
  int                     last_gamestate;
  bool                    last_paused;
};

void dmcp_adapter_log(int level, const char* fmt, ...) {
  va_list     args;
  time_t      now;
  struct tm   tm_info;
  char        timestamp[20];
  const char* level_str;

  va_start(args, fmt);

  now     = time(NULL);
  tm_info = *localtime(&now);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_info);

  level_str = "DEBUG";
  switch (level) {
    case MCP_LOG_ERROR:
      level_str = "ERROR";
      break;
    case MCP_LOG_WARN:
      level_str = "WARN";
      break;
    case MCP_LOG_INFO:
      level_str = "INFO";
      break;
    case MCP_LOG_DEBUG:
      level_str = "DEBUG";
      break;
  }

  fprintf(stderr, "[%s] [DMCP] [%s] ", timestamp, level_str);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

static void chocolate_log_callback(void* user_data, int level, const char* message) {
  (void)user_data;
  dmcp_adapter_log(level, "%s", message);
}

static const char* gamestate_to_string(int gs) {
  switch (gs) {
    case GS_LEVEL:
      return "in_level";
    case GS_INTERMISSION:
      return "intermission";
    case GS_FINALE:
      return "finale";
    case GS_DEMOSCREEN:
      return "demo";
    default:
      return "unknown";
  }
}

static void snapshot_callback(void* user_data, dmcp_snapshot_t* snap) {
  dmcp_chocolate_t* ctx = (dmcp_chocolate_t*)user_data;
  int               current_gamestate;
  boolean           current_paused;

  if (!ctx || !snap) return;

  dmcp_chocolate_populate_player(snap);
  dmcp_chocolate_populate_level(snap);
  dmcp_chocolate_populate_enemies(snap);
  dmcp_chocolate_populate_entities(snap);

  current_gamestate = gamestate;
  if (current_gamestate != ctx->last_gamestate) {
    dmcp_adapter_log(MCP_LOG_INFO, "state transition: %s -> %s",
                     gamestate_to_string(ctx->last_gamestate),
                     gamestate_to_string(current_gamestate));
    ctx->last_gamestate = current_gamestate;
  }

  current_paused = paused;
  if (current_paused != ctx->last_paused) {
    dmcp_adapter_log(MCP_LOG_INFO, "pause state: %s", current_paused ? "paused" : "resumed");
    ctx->last_paused = current_paused;
  }
}

dmcp_chocolate_t* dmcp_chocolate_create(const dmcp_chocolate_config_t* config) {
  dmcp_chocolate_t* ctx;
  dmcp_config_t     dmcp_cfg;

  if (!config) {
    dmcp_adapter_log(MCP_LOG_ERROR, "create failed: null config");
    return NULL;
  }

  ctx = (dmcp_chocolate_t*)calloc(1, sizeof(dmcp_chocolate_t));
  if (!ctx) {
    dmcp_adapter_log(MCP_LOG_ERROR, "create failed: memory allocation failed");
    return NULL;
  }

  ctx->config         = *config;
  ctx->last_gamestate = -1;
  ctx->last_paused    = false;

  dmcp_cfg             = config->base;
  dmcp_cfg.on_snapshot = snapshot_callback;
  dmcp_cfg.on_log      = chocolate_log_callback;
  dmcp_cfg.user_data   = ctx;

  ctx->dmcp_ctx = dmcp_context_create(&dmcp_cfg);
  if (!ctx->dmcp_ctx) {
    dmcp_adapter_log(MCP_LOG_ERROR,
                     "create failed: dmcp_context_create returned null (port %d in use?)",
                     dmcp_cfg.port);
    free(ctx);
    return NULL;
  }

  ctx->initialized = true;
  dmcp_adapter_log(MCP_LOG_INFO, "adapter created (port=%d, target_hz=%d)", dmcp_cfg.port,
                   dmcp_cfg.target_hz);
  return ctx;
}

void dmcp_chocolate_destroy(dmcp_chocolate_t* ctx) {
  if (!ctx) return;

  dmcp_adapter_log(MCP_LOG_INFO, "adapter destroying");

  if (ctx->dmcp_ctx) {
    dmcp_context_destroy(ctx->dmcp_ctx);
  }

  free(ctx);
}

void dmcp_chocolate_tick(dmcp_chocolate_t* ctx) {
  if (!ctx || !ctx->dmcp_ctx || !ctx->initialized) return;

  if (gamestate != GS_LEVEL) {
    return;
  }

  dmcp_context_tick(ctx->dmcp_ctx);
}

void dmcp_chocolate_commands_process(dmcp_chocolate_t* ctx) {
  dmcp_command_t cmd;
  int            cmd_count;

  if (!ctx || !ctx->dmcp_ctx) return;

  cmd_count = 0;
  while (dmcp_pop_command(ctx->dmcp_ctx, &cmd)) {
    bool        result = dmcp_chocolate_command_execute(ctx, &cmd);
    const char* message;
    if (result) {
      message = "Command executed successfully";
    } else if (cmd.type == DMCP_CMD_GIVE_ITEM || cmd.type == DMCP_CMD_SPAWN_ENTITY) {
      if (gamemode == shareware) {
        message =
            "Content not available in shareware. Restricted: Plasma Rifle, BFG, Super Shotgun, "
            "Cacodemon, Lost Soul, Cyberdemon, Spider Mastermind, and Doom II monsters";
      } else {
        message = "Command failed: invalid item/monster or unavailable in current mode";
      }
    } else {
      message = "Command failed in engine";
    }
    dmcp_command_result_complete(ctx->dmcp_ctx, &cmd, result, message);
    dmcp_adapter_log(MCP_LOG_DEBUG, "command executed: type=%d result=%s", cmd.type,
                     result ? "success" : "failed");
    cmd_count++;
  }
  if (cmd_count > 0) {
    dmcp_adapter_log(MCP_LOG_INFO, "processed %d command(s)", cmd_count);
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

dmcp_context_t* dmcp_chocolate_get_dmcp_context(dmcp_chocolate_t* ctx) {
  if (!ctx) return NULL;
  return ctx->dmcp_ctx;
}
