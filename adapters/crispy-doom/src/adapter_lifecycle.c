// Crispy Doom Adapter - Lifecycle
// Create, destroy, tick, and stats for DMCP integration

#include "dmcp_crispy.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dmcp_adapter_command_queue.h"
#include "dmcp/adapter/utils.h"
#include "dmcp/doom/api.h"
#include "mcp/core/memory.h"
#include "mcp/core/string.h"

#include "doomdef.h"
#include "doomstat.h"

struct dmcp_crispy_s {
  dmcp_context_t*      dmcp_ctx;
  dmcp_crispy_config_t config;
  bool                 initialized;
  int                  last_gamestate;
  bool                 last_paused;
};

static int dmcp_log_threshold(void) {
  static int  initialized = 0;
  static int  threshold   = MCP_LOG_INFO;
  const char* env;

  if (initialized) {
    return threshold;
  }

  initialized = 1;
  env         = getenv("DMCP_LOG_LEVEL");
  if (!env || !env[0]) {
    return threshold;
  }

  if (mcp_strcmp_ci(env, "debug") == 0 || strcmp(env, "0") == 0) {
    threshold = MCP_LOG_DEBUG;
  } else if (mcp_strcmp_ci(env, "info") == 0 || strcmp(env, "1") == 0) {
    threshold = MCP_LOG_INFO;
  } else if (mcp_strcmp_ci(env, "warn") == 0 || mcp_strcmp_ci(env, "warning") == 0 ||
             strcmp(env, "2") == 0) {
    threshold = MCP_LOG_WARN;
  } else if (mcp_strcmp_ci(env, "error") == 0 || strcmp(env, "3") == 0) {
    threshold = MCP_LOG_ERROR;
  }

  return threshold;
}

void dmcp_adapter_log(int level, const char* fmt, ...) {
  va_list     args;
  time_t      now;
  struct tm   tm_info;
  char        timestamp[20];
  const char* level_str;

  if (level < dmcp_log_threshold()) {
    return;
  }

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

static void crispy_log_callback(void* user_data, int level, const char* message) {
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
  dmcp_crispy_t* ctx = (dmcp_crispy_t*)user_data;
  int            current_gamestate;
  boolean        current_paused;

  if (!ctx || !snap) return;

  dmcp_crispy_populate_player(snap);
  dmcp_crispy_populate_level(snap);
  dmcp_crispy_populate_enemies(snap);
  dmcp_crispy_populate_entities(snap);

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

dmcp_crispy_t* dmcp_crispy_create(const dmcp_crispy_config_t* config) {
  dmcp_crispy_t*       ctx;
  dmcp_config_t        dmcp_cfg;
  dmcp_crispy_config_t effective_config;

  if (!config) {
    dmcp_adapter_log(MCP_LOG_ERROR, "create failed: null config");
    return NULL;
  }

  effective_config = dmcp_crispy_config_default();
  {
    size_t copy_size = config->struct_size;
    if (copy_size == 0 || copy_size > sizeof(dmcp_crispy_config_t)) {
      copy_size = sizeof(dmcp_crispy_config_t);
    }
    mcp_memcpy_safe(&effective_config, sizeof(effective_config), config, copy_size);
    effective_config.struct_size = sizeof(dmcp_crispy_config_t);
  }

  ctx = (dmcp_crispy_t*)calloc(1, sizeof(dmcp_crispy_t));
  if (!ctx) {
    dmcp_adapter_log(MCP_LOG_ERROR, "create failed: memory allocation failed");
    return NULL;
  }

  ctx->config         = effective_config;
  ctx->last_gamestate = -1;
  ctx->last_paused    = false;

  dmcp_cfg             = effective_config.base;
  dmcp_cfg.on_snapshot = snapshot_callback;
  dmcp_cfg.on_log      = crispy_log_callback;
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
  dmcp_adapter_log(MCP_LOG_INFO, "adapter created (port=%d target_hz=%u)", dmcp_cfg.port,
                   (unsigned)dmcp_cfg.target_hz);
  return ctx;
}

void dmcp_crispy_destroy(dmcp_crispy_t* ctx) {
  if (!ctx) return;

  dmcp_adapter_log(MCP_LOG_INFO, "adapter destroying");

  if (ctx->dmcp_ctx) {
    dmcp_context_destroy(ctx->dmcp_ctx);
  }

  free(ctx);
}

mcp_status_t dmcp_crispy_tick(dmcp_crispy_t* ctx) {
  if (!ctx || !ctx->dmcp_ctx || !ctx->initialized) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  if (!dmcp_context_is_running(ctx->dmcp_ctx)) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_DISABLED, "Operation disabled");
  }

  if (gamestate != GS_LEVEL) {
    return MCP_STATUS_OK("Success");
  }

  dmcp_context_tick(ctx->dmcp_ctx);
  return MCP_STATUS_OK("Success");
}

static bool crispy_execute_command_for_queue(void* adapter_ctx, const dmcp_command_t* cmd,
                                             char* out_message, size_t out_message_size) {
  dmcp_crispy_t* ctx;
  bool           success;

  if (!adapter_ctx || !cmd) {
    return false;
  }

  ctx     = (dmcp_crispy_t*)adapter_ctx;
  success = dmcp_crispy_command_execute(ctx, cmd, out_message, out_message_size);

  dmcp_adapter_log(MCP_LOG_DEBUG, "command executed: type=%d result=%s", cmd->type,
                   success ? "success" : "failed");
  return success;
}

void dmcp_crispy_commands_process(dmcp_crispy_t* ctx) {
  int cmd_count;

  if (!ctx || !ctx->dmcp_ctx) return;

  cmd_count =
      dmcp_adapter_process_command_queue(ctx->dmcp_ctx, ctx, crispy_execute_command_for_queue, 0);

  if (cmd_count > 0) {
    dmcp_adapter_log(MCP_LOG_INFO, "processed %d command(s)", cmd_count);
  }
}

bool dmcp_crispy_is_running(const dmcp_crispy_t* ctx) {
  if (!ctx || !ctx->dmcp_ctx) return false;
  return dmcp_context_is_running(ctx->dmcp_ctx);
}

void dmcp_crispy_get_stats(dmcp_crispy_t* ctx, dmcp_stats_t* stats) {
  if (!ctx || !ctx->dmcp_ctx || !stats) return;
  dmcp_stats_get(ctx->dmcp_ctx, stats);
}

dmcp_context_t* dmcp_crispy_get_context(dmcp_crispy_t* ctx) {
  if (!ctx) return NULL;
  return ctx->dmcp_ctx;
}
