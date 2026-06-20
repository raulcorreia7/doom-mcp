#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dmcp/doom/dmcp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dmcp_crispy_s dmcp_crispy_t;

#define DMCP_CRISPY_DEFAULT_TARGET_HZ 35u

typedef struct {
  uint32_t struct_size;

  dmcp_config_t base;
} dmcp_crispy_config_t;

static inline dmcp_crispy_config_t dmcp_crispy_config_default(void) {
  dmcp_crispy_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.struct_size    = sizeof(dmcp_crispy_config_t);
  cfg.base           = dmcp_config_default();
  cfg.base.target_hz = DMCP_CRISPY_DEFAULT_TARGET_HZ;
  return cfg;
}

dmcp_crispy_t* dmcp_crispy_create(const dmcp_crispy_config_t* config);
void           dmcp_crispy_destroy(dmcp_crispy_t* ctx);

mcp_status_t dmcp_crispy_tick(dmcp_crispy_t* ctx);
void         dmcp_crispy_commands_process(dmcp_crispy_t* ctx);
void         dmcp_crispy_inputs_process(dmcp_crispy_t* ctx);

bool            dmcp_crispy_is_running(const dmcp_crispy_t* ctx);
void            dmcp_crispy_stats_get(dmcp_crispy_t* ctx, dmcp_stats_t* stats);
dmcp_context_t* dmcp_crispy_context_get(dmcp_crispy_t* ctx);

#ifdef __cplusplus
}
#endif
