#pragma once

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mcp/generic/constants.h"
#include "mcp/generic/protocol.h"
#include "mcp/generic/result.h"

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

#define DMCP_MAX_ENEMIES   MCP_MAX_ENEMIES
#define DMCP_MAX_INVENTORY MCP_MAX_INVENTORY

typedef struct dmcp_snapshot_t {
  dmcp_player_t player;
  dmcp_level_t  level;

  dmcp_enemy_t enemies[DMCP_MAX_ENEMIES];
  uint32_t     enemy_count;

  dmcp_item_t inventory[DMCP_MAX_INVENTORY];
  uint32_t    inventory_count;
} dmcp_snapshot_t;

typedef struct {
  const uint8_t* pixels;
  uint32_t       width;
  uint32_t       height;
  uint32_t       stride;
} dmcp_screenshot_frame_t;

typedef struct {
  size_t   struct_size;
  uint64_t dropped_snapshots;
  uint64_t dropped_screenshots;
  uint64_t connected_clients;
} dmcp_stats_t;

#ifdef __cplusplus
}
#endif
