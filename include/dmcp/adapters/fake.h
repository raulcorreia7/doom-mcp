#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dmcp/doom/dmcp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dmcp_fake_s dmcp_fake_t;

#define DMCP_FAKE_DEFAULT_TARGET_HZ 35u
#define DMCP_FAKE_DEFAULT_SEED 1337u

typedef struct {
  uint32_t struct_size;

  dmcp_config_t base;

  uint32_t seed;
  uint32_t max_enemies;
  uint32_t max_entities;
} dmcp_fake_config_t;

static inline dmcp_fake_config_t dmcp_fake_config_default(void) {
  dmcp_fake_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.struct_size    = sizeof(dmcp_fake_config_t);
  cfg.base           = dmcp_config_default();
  cfg.base.target_hz = DMCP_FAKE_DEFAULT_TARGET_HZ;
  cfg.seed           = DMCP_FAKE_DEFAULT_SEED;
  cfg.max_enemies    = DMCP_MAX_ENEMIES;
  cfg.max_entities   = DMCP_MAX_ENTITIES;
  return cfg;
}

DMCP_API dmcp_fake_t* dmcp_fake_create(const dmcp_fake_config_t* config);
DMCP_API void         dmcp_fake_destroy(dmcp_fake_t* fake);

DMCP_API mcp_status_t dmcp_fake_tick(dmcp_fake_t* fake);
DMCP_API void         dmcp_fake_commands_process(dmcp_fake_t* fake);
DMCP_API void         dmcp_fake_inputs_process(dmcp_fake_t* fake);
DMCP_API bool         dmcp_fake_command_execute(dmcp_fake_t* fake, const dmcp_command_t* cmd);

/* Returns true when the fake adapter can process local ticks and queues.
   If base.start_transport is false, this does not imply an HTTP/SSE listener. */
DMCP_API bool            dmcp_fake_is_running(const dmcp_fake_t* fake);
DMCP_API dmcp_context_t* dmcp_fake_get_context(dmcp_fake_t* fake);
DMCP_API void            dmcp_fake_get_stats(dmcp_fake_t* fake, dmcp_stats_t* out_stats);

DMCP_API bool dmcp_fake_snapshot_get(const dmcp_fake_t* fake, dmcp_snapshot_t* out_snapshot);

#ifdef __cplusplus
}
#endif
