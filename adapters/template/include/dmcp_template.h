#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dmcp/doom/dmcp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dmcp_template_s dmcp_template_t;

/*
 * Fill one DMCP snapshot from engine state.
 *
 * The callback runs on the DMCP tick path. Keep it bounded and avoid blocking
 * on rendering, asset loading, or long engine locks.
 */
typedef bool (*dmcp_template_snapshot_fn)(void* engine_user, dmcp_snapshot_t* out_snapshot);

/*
 * Execute one queued command on the game thread.
 *
 * The message buffer is optional but useful for agent feedback and command
 * result diagnostics.
 */
typedef bool (*dmcp_template_command_fn)(void* engine_user, const dmcp_command_t* command,
                                         char* out_message, size_t out_message_size);

/*
 * Capture a rendered frame when DMCP asks for a screenshot.
 *
 * DMCP copies the pixel data before the callback returns. The engine keeps
 * ownership of any memory referenced by out_frame.
 */
typedef bool (*dmcp_template_screenshot_fn)(void* engine_user, dmcp_screenshot_frame_t* out_frame);

typedef struct {
  uint32_t struct_size;

  dmcp_config_t base;

  void* engine_user;

  dmcp_template_snapshot_fn   fill_snapshot;
  dmcp_template_command_fn    execute_command;
  dmcp_template_command_fn    execute_input;
  dmcp_template_screenshot_fn capture_frame;
} dmcp_template_config_t;

static inline dmcp_template_config_t dmcp_template_config_default(void) {
  dmcp_template_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.struct_size    = sizeof(dmcp_template_config_t);
  cfg.base           = dmcp_config_default();
  cfg.base.target_hz = DMCP_DEFAULT_TARGET_HZ;
  return cfg;
}

DMCP_API dmcp_template_t* dmcp_template_create(const dmcp_template_config_t* config);
DMCP_API void             dmcp_template_destroy(dmcp_template_t* adapter);

DMCP_API mcp_status_t dmcp_template_tick(dmcp_template_t* adapter);
DMCP_API void         dmcp_template_commands_process(dmcp_template_t* adapter);
DMCP_API void         dmcp_template_inputs_process(dmcp_template_t* adapter);
DMCP_API bool         dmcp_template_command_execute(dmcp_template_t*      adapter,
                                                    const dmcp_command_t* command);
DMCP_API mcp_status_t dmcp_template_capture_frame(dmcp_template_t* adapter);

DMCP_API bool            dmcp_template_is_running(const dmcp_template_t* adapter);
DMCP_API dmcp_context_t* dmcp_template_get_context(dmcp_template_t* adapter);
DMCP_API void            dmcp_template_get_stats(dmcp_template_t* adapter, dmcp_stats_t* out_stats);

#ifdef __cplusplus
}
#endif
