#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dmcp/doom/dmcp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file zdoom.h
 * @brief ZDoom/GZDoom source-level adapter API
 *
 * Source-level adapter API for integrating DMCP with ZDoom-based engines.
 * Compile the adapter into the engine or an engine-owned module; libdmcp
 * exports the game-agnostic DMCP runtime API.
 */

typedef struct dmcp_zdoom_s dmcp_zdoom_t;

#define DMCP_ZDOOM_DEFAULT_TARGET_HZ 35u

/**
 * @brief ZDoom adapter configuration
 */
typedef struct {
  uint32_t struct_size;

  dmcp_config_t base;

  void (*log_fn)(void* user, int level, const char* message);
  void* log_user;
  bool (*should_tick_fn)(void* user);
  void* should_tick_user;
} dmcp_zdoom_config_t;

static inline dmcp_zdoom_config_t dmcp_zdoom_config_default(void) {
  dmcp_zdoom_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.struct_size      = sizeof(dmcp_zdoom_config_t);
  cfg.base             = dmcp_config_default();
  cfg.base.target_hz   = DMCP_ZDOOM_DEFAULT_TARGET_HZ;
  cfg.log_fn           = NULL;
  cfg.log_user         = NULL;
  cfg.should_tick_fn   = NULL;
  cfg.should_tick_user = NULL;
  return cfg;
}

dmcp_zdoom_t*   dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg);
void            dmcp_zdoom_destroy(dmcp_zdoom_t* ctx);
mcp_status_t    dmcp_zdoom_tick(dmcp_zdoom_t* ctx);
bool            dmcp_zdoom_is_running(dmcp_zdoom_t* ctx);
void            dmcp_zdoom_stats_get(dmcp_zdoom_t* ctx, dmcp_stats_t* out_stats);
dmcp_context_t* dmcp_zdoom_context_get(dmcp_zdoom_t* ctx);

bool dmcp_zdoom_command_execute(dmcp_zdoom_t* ctx, const dmcp_command_t* cmd);
void dmcp_zdoom_commands_process(dmcp_zdoom_t* ctx);
void dmcp_zdoom_inputs_process(dmcp_zdoom_t* ctx);

#ifdef __cplusplus
}
#endif
