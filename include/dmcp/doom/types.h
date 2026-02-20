#pragma once

#include "config.h"
#include "constants.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mcp/generic/protocol.h"
#include "mcp/generic/result.h"

#define DMCP_MAX_STRING 64
#define DMCP_MAX_WEAPONS 9
#define DMCP_MAX_AMMO_TYPES 4
#define DMCP_MAX_POWERUPS 6
#define DMCP_MAX_KEYS 6

typedef struct {
  float x;
  float y;
} dmcp_vec2_t;

typedef struct {
  float x;
  float y;
  float z;
} dmcp_vec3_t;

typedef struct {
  int32_t     hp;
  int32_t     armor;
  char        armortype[DMCP_MAX_STRING];
  dmcp_vec3_t position;
  float       angle;

  char    readyweapon[DMCP_MAX_STRING];
  char    pendingweapon[DMCP_MAX_STRING];
  int32_t weaponowned[DMCP_MAX_WEAPONS];

  int32_t ammo[DMCP_MAX_AMMO_TYPES];
  int32_t maxammo[DMCP_MAX_AMMO_TYPES];
  int32_t backpack;

  int32_t powers[DMCP_MAX_POWERUPS];
  int32_t cards[DMCP_MAX_KEYS];

  char    playerstate[DMCP_MAX_STRING];
  int32_t cheats;
  int32_t damagecount;
} dmcp_player_t;

typedef struct {
  int32_t tic;
  int32_t leveltime;
  char    level_id[DMCP_MAX_LEVEL_ID];
  char    level_name[DMCP_MAX_LEVEL_NAME];

  int32_t kill_count;
  int32_t item_count;
  int32_t secret_count;
  int32_t totalkills;
  int32_t totalitems;
  int32_t totalsecrets;

  char    skill[DMCP_MAX_STRING];
  char    gamestate[DMCP_MAX_STRING];
  int32_t paused;
} dmcp_level_t;

typedef struct {
  char    mode[DMCP_MAX_STRING];
  char    version[DMCP_MAX_STRING];
  int32_t respawnmonsters;
  int32_t consoleplayer;
} dmcp_game_t;

typedef struct {
  int32_t     id;
  int32_t     hp;
  int32_t     max_hp;
  dmcp_vec3_t position;
  float       angle;
  int32_t     target_id;
  char        type[DMCP_MAX_ENEMY_TYPE];
} dmcp_enemy_t;

typedef struct {
  char    name[DMCP_MAX_ITEM_NAME];
  int32_t amount;
} dmcp_item_t;

typedef struct dmcp_snapshot_t {
  dmcp_player_t player;
  dmcp_level_t  level;
  dmcp_game_t   game;

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
