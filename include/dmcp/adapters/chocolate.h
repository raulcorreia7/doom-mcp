#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dmcp/doom/dmcp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file chocolate.h
 * @brief Chocolate Doom adapter public API
 *
 * Public API for integrating DMCP with Chocolate Doom.
 * Include this header for the stable C ABI.
 */

typedef struct dmcp_chocolate_s dmcp_chocolate_t;

/**
 * @brief Chocolate Doom adapter configuration
 */
typedef struct {
  dmcp_config_t base;
  const char*   iwad_path;
  const char*   pwad_path;
} dmcp_chocolate_config_t;

/**
 * @brief Get default Chocolate Doom configuration
 */
static inline dmcp_chocolate_config_t dmcp_chocolate_config_default(void) {
  dmcp_chocolate_config_t cfg = {0};
  cfg.base                    = dmcp_config_default();
  cfg.base.target_hz          = 35;
  cfg.base.screenshot.enable  = false;
  cfg.iwad_path               = NULL;
  cfg.pwad_path               = NULL;
  return cfg;
}

dmcp_chocolate_t* dmcp_chocolate_create(const dmcp_chocolate_config_t* config);
void              dmcp_chocolate_destroy(dmcp_chocolate_t* ctx);
void              dmcp_chocolate_tick(dmcp_chocolate_t* ctx);
void              dmcp_chocolate_commands_process(dmcp_chocolate_t* ctx);
void              dmcp_chocolate_inputs_process(dmcp_chocolate_t* ctx);
dmcp_context_t*   dmcp_chocolate_get_dmcp_context(dmcp_chocolate_t* ctx);
bool              dmcp_chocolate_is_running(const dmcp_chocolate_t* ctx);
void              dmcp_chocolate_get_stats(dmcp_chocolate_t* ctx, dmcp_stats_t* stats);
bool              dmcp_chocolate_command_execute(dmcp_chocolate_t* ctx, const dmcp_command_t* cmd);
void              dmcp_adapter_log(int level, const char* fmt, ...);

#ifdef __cplusplus
}
#endif
