#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dmcp/doom/constants.h"
#include "dmcp/doom/export.h"
#include "mcp/generic/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dmcp_context_s dmcp_context_t;
struct dmcp_snapshot_t;

// Log messages are borrowed, null-terminated strings and must not be freed by
// the callback.
typedef void (*dmcp_log_callback_t)(void* user_data, int level, const char* message);

// Called from dmcp_context_tick(). The callback fills the provided snapshot;
// DMCP owns the snapshot storage and the pointer is only valid for the call.
typedef void (*dmcp_snapshot_callback_t)(void* user_data, struct dmcp_snapshot_t* snapshot);

typedef struct {
  // Must be initialized to sizeof(dmcp_config_t). Older callers may pass a
  // smaller size; fields beyond struct_size are filled from dmcp_config_default().
  uint32_t struct_size;

  uint16_t port;
  uint32_t target_hz;
  size_t   command_queue_slots;

  struct {
    bool     enable;
    uint32_t width;
    uint32_t height;
  } screenshot;

  struct {
    bool game;
    bool input;
  } tools;

  /*
   * Privileged capabilities are opt-in so embedders can expose rich gameplay
   * state by default without also exposing raw console access or cheat-style
   * mutation. Normal gameplay automation such as spawn_entity, give_item,
   * change_level, pause_game, execute_batch, and player_input is not gated
   * here.
   */
  struct {
    bool allow_console_commands;
    bool allow_cheats;
  } permissions;

  dmcp_snapshot_callback_t on_snapshot;
  dmcp_log_callback_t      on_log;
  void*                    user_data;

  bool start_transport;

} dmcp_config_t;

static inline dmcp_config_t dmcp_config_default(void) {
  dmcp_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.struct_size                        = sizeof(dmcp_config_t);
  cfg.port                               = MCP_DEFAULT_PORT;
  cfg.target_hz                          = DMCP_DEFAULT_TARGET_HZ;
  cfg.command_queue_slots                = DMCP_DEFAULT_QUEUE_SLOTS;
  cfg.screenshot.enable                  = false;
  cfg.screenshot.width                   = DMCP_DEFAULT_SCREENSHOT_WIDTH;
  cfg.screenshot.height                  = DMCP_DEFAULT_SCREENSHOT_HEIGHT;
  cfg.tools.game                         = true;
  cfg.tools.input                        = true;
  cfg.permissions.allow_console_commands = true;
  cfg.permissions.allow_cheats           = true;
  cfg.on_snapshot                        = NULL;
  cfg.on_log                             = NULL;
  cfg.user_data                          = NULL;
  cfg.start_transport                    = true;
  return cfg;
}

#ifdef __cplusplus
}
#endif
