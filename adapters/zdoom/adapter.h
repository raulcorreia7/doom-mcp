#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "dmcp/dmcp.h"

typedef struct dmcp_zdoom_s dmcp_zdoom_t;

typedef struct {
  // Optional override of the base DMCP config. If NULL, defaults are used.
  const dmcp_config_t* dmcp_config;

  // Optional logging hook. If NULL, the adapter uses the engine Printf
  // fallback. level: dmcp_log_level_t; message: null-terminated string.
  void (*log_fn)(void* user, int level, const char* message);
  void* log_user;

  // Optional gate to skip ticking (e.g., pause/menu). If NULL, always ticks.
  bool (*should_tick_fn)(void* user);
  void* should_tick_user;
} dmcp_zdoom_config_t;

// Create/destroy the adapter. Returns NULL on failure (e.g., DMCP server start
// failure).
dmcp_zdoom_t* dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg);
void          dmcp_zdoom_destroy(dmcp_zdoom_t* ctx);

// Per-tic update. Returns DMCP_OK or negative dmcp_result_t on error.
int dmcp_zdoom_tick(dmcp_zdoom_t* ctx);

// State/health helpers.
bool dmcp_zdoom_is_running(dmcp_zdoom_t* ctx);
void dmcp_zdoom_get_stats(dmcp_zdoom_t* ctx, dmcp_stats_t* out_stats);

#ifdef __cplusplus
}
#endif
