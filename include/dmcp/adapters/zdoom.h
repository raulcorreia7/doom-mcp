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
 * @brief ZDoom/GZDoom adapter public API
 *
 * Public API for integrating DMCP with ZDoom-based engines.
 * Include this header for the stable C API.
 */

typedef struct dmcp_zdoom_s dmcp_zdoom_t;

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
  cfg.base.target_hz   = 35;
  cfg.log_fn           = NULL;
  cfg.log_user         = NULL;
  cfg.should_tick_fn   = NULL;
  cfg.should_tick_user = NULL;
  return cfg;
}

DMCP_API dmcp_zdoom_t*   dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg);
DMCP_API void            dmcp_zdoom_destroy(dmcp_zdoom_t* ctx);
DMCP_API mcp_status_t    dmcp_zdoom_tick(dmcp_zdoom_t* ctx);
DMCP_API bool            dmcp_zdoom_is_running(dmcp_zdoom_t* ctx);
DMCP_API void            dmcp_zdoom_get_stats(dmcp_zdoom_t* ctx, dmcp_stats_t* out_stats);
DMCP_API dmcp_context_t* dmcp_zdoom_get_context(dmcp_zdoom_t* ctx);

DMCP_API bool dmcp_zdoom_command_execute(dmcp_zdoom_t* ctx, const dmcp_command_t* cmd);
DMCP_API void dmcp_zdoom_commands_process(dmcp_zdoom_t* ctx);
DMCP_API void dmcp_zdoom_inputs_process(dmcp_zdoom_t* ctx);

#ifdef __cplusplus
}
#endif
