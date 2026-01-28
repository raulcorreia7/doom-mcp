#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mcp/generic/constants.h"
#include "mcp/generic/result.h"

// ============================================================================
// Doom MCP Version
// These are defined by CMake, provided here for reference
// ============================================================================
#ifndef DMCP_VERSION_MAJOR
#define DMCP_VERSION_MAJOR 0
#endif
#ifndef DMCP_VERSION_MINOR
#define DMCP_VERSION_MINOR 4
#endif
#ifndef DMCP_VERSION_PATCH
#define DMCP_VERSION_PATCH 0
#endif

// ============================================================================
// Result Codes
// ============================================================================

typedef mcp_result_generic_t dmcp_result_t;

// Result code constants (for comparing .code field)
#define DMCP_RESULT_CODE_OK              0
#define DMCP_RESULT_CODE_INVALID_ARGS    -1
#define DMCP_RESULT_CODE_ENCODING_FAILED -2
#define DMCP_RESULT_CODE_DISABLED        -3
#define DMCP_RESULT_CODE_QUEUE_FULL      -4
#define DMCP_RESULT_CODE_SERVER_FAILED   -5

// Convenience macros for creating results
#define DMCP_OK MCP_RESULT_OK("Success")
#define DMCP_ERROR_INVALID_ARGS \
  MCP_RESULT_ERROR(DMCP_RESULT_CODE_INVALID_ARGS, "Invalid arguments")
#define DMCP_ERROR_ENCODING_FAILED \
  MCP_RESULT_ERROR(DMCP_RESULT_CODE_ENCODING_FAILED, "Encoding failed")
#define DMCP_ERROR_DISABLED \
  MCP_RESULT_ERROR(DMCP_RESULT_CODE_DISABLED, "Operation disabled")
#define DMCP_ERROR_QUEUE_FULL \
  MCP_RESULT_ERROR(DMCP_RESULT_CODE_QUEUE_FULL, "Queue full")
#define DMCP_ERROR_SERVER_FAILED \
  MCP_RESULT_ERROR(DMCP_RESULT_CODE_SERVER_FAILED, "Server failed")

// ============================================================================
// Log Levels
// ============================================================================
typedef enum {
  DMCP_LOG_DEBUG = 0,
  DMCP_LOG_INFO  = 1,
  DMCP_LOG_WARN  = 2,
  DMCP_LOG_ERROR = 3
} dmcp_log_level_t;

// ============================================================================
// Forward Declarations
// ============================================================================
typedef struct dmcp_context_s dmcp_context_t;

// ============================================================================
// Basic Types
// ============================================================================

typedef struct {
  float x;
  float y;
} dmcp_vec2_t;

typedef struct {
  float       hp;
  float       armor;
  dmcp_vec2_t position;
  int32_t     ammo;
} dmcp_player_t;

typedef struct {
  int32_t tic;
  char    level_id[MCP_MAX_LEVEL_ID];
  char    level_name[MCP_MAX_LEVEL_NAME];
  int32_t kill_count;
  int32_t item_count;
  int32_t secret_count;
} dmcp_level_t;

typedef struct {
  int32_t     id;
  float       hp;
  float       max_hp;
  dmcp_vec2_t position;
  char        type[MCP_MAX_ENEMY_TYPE];
} dmcp_enemy_t;

typedef struct {
  char    name[MCP_MAX_ITEM_NAME];
  int32_t amount;
} dmcp_item_t;

// ============================================================================
// Snapshot
// ============================================================================
#define DMCP_MAX_ENEMIES   MCP_MAX_ENEMIES
#define DMCP_MAX_INVENTORY MCP_MAX_INVENTORY

typedef struct {
  dmcp_player_t player;
  dmcp_level_t  level;

  dmcp_enemy_t enemies[DMCP_MAX_ENEMIES];
  uint32_t     enemy_count;

  dmcp_item_t inventory[DMCP_MAX_INVENTORY];
  uint32_t    inventory_count;
} dmcp_snapshot_t;

// ============================================================================
// Screenshot
// ============================================================================
typedef struct {
  const uint8_t* pixels;
  uint32_t       width;
  uint32_t       height;
  uint32_t       stride;  // Bytes per row
} dmcp_screenshot_frame_t;

// ============================================================================
// Statistics
// ============================================================================
typedef struct {
  uint64_t dropped_snapshots;
  uint64_t dropped_screenshots;
  uint64_t connected_clients;
} dmcp_stats_t;

// ============================================================================
// Callbacks
// ============================================================================

typedef void (*dmcp_log_callback_t)(void* user_data, int level,
                                    const char* message);

typedef void (*dmcp_snapshot_callback_t)(void*            user_data,
                                         dmcp_snapshot_t* snapshot);

// ============================================================================
// Configuration
// ============================================================================
typedef struct {
  uint32_t struct_size;  // Set to sizeof(dmcp_config_t)

  // Server settings
  uint16_t port;
  uint32_t target_hz;  // Snapshot rate (default: 10)

  // Capacity settings
  size_t snapshot_pool_size;
  size_t queue_slots;

  // Screenshot settings
  struct {
    bool     enable;
    uint32_t width;
    uint32_t height;
  } screenshot;

  // Callbacks
  dmcp_snapshot_callback_t on_snapshot;  // Fill snapshot with game data
  dmcp_log_callback_t      on_log;
  void*                    user_data;

} dmcp_config_t;

// ============================================================================
// Default Configuration
// ============================================================================
static inline dmcp_config_t dmcp_config_default(void) {
  dmcp_config_t cfg      = {};
  cfg.struct_size        = sizeof(dmcp_config_t);
  cfg.port               = MCP_DEFAULT_PORT;
  cfg.target_hz          = MCP_DEFAULT_TARGET_HZ;
  cfg.snapshot_pool_size = MCP_DEFAULT_SNAPSHOT_POOL_SIZE;
  cfg.queue_slots        = MCP_DEFAULT_QUEUE_SLOTS;
  cfg.screenshot.enable  = true;
  cfg.screenshot.width   = MCP_DEFAULT_SCREENSHOT_WIDTH;
  cfg.screenshot.height  = MCP_DEFAULT_SCREENSHOT_HEIGHT;
  cfg.on_snapshot        = NULL;
  cfg.on_log             = NULL;
  cfg.user_data          = NULL;
  return cfg;
}

#ifdef __cplusplus
}
#endif
