#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mcp/generic/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dmcp_context_s dmcp_context_t;
struct dmcp_snapshot_t;

typedef void (*dmcp_log_callback_t)(void* user_data, int level,
                                    const char* message);

typedef void (*dmcp_snapshot_callback_t)(void*                   user_data,
                                         struct dmcp_snapshot_t* snapshot);

typedef struct {
  uint32_t struct_size;

  uint16_t port;
  uint32_t target_hz;

  size_t snapshot_pool_size;
  size_t _reserved_queue_slots;

  struct {
    bool     enable;
    uint32_t width;
    uint32_t height;
  } screenshot;

  dmcp_snapshot_callback_t on_snapshot;
  dmcp_log_callback_t      on_log;
  void*                    user_data;

} dmcp_config_t;

static inline dmcp_config_t dmcp_config_default(void) {
  dmcp_config_t cfg         = {};
  cfg.struct_size           = sizeof(dmcp_config_t);
  cfg.port                  = MCP_DEFAULT_PORT;
  cfg.target_hz             = MCP_DEFAULT_TARGET_HZ;
  cfg.snapshot_pool_size    = MCP_DEFAULT_SNAPSHOT_POOL_SIZE;
  cfg._reserved_queue_slots = MCP_DEFAULT_QUEUE_SLOTS;
  cfg.screenshot.enable     = true;
  cfg.screenshot.width      = MCP_DEFAULT_SCREENSHOT_WIDTH;
  cfg.screenshot.height     = MCP_DEFAULT_SCREENSHOT_HEIGHT;
  cfg.on_snapshot           = NULL;
  cfg.on_log                = NULL;
  cfg.user_data             = NULL;
  return cfg;
}

#ifdef __cplusplus
}
#endif
