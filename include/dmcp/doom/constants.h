#pragma once

#include "dmcp/doom/export.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Doom MCP Runtime Defaults
// ============================================================================

// Default rate for sampling game state into DMCP's latest snapshot.
// Engines can call dmcp_context_tick every frame; DMCP samples at this rate.
#define DMCP_DEFAULT_TARGET_HZ 10

// Number of pending mutating commands accepted from agent/MCP clients.
#define DMCP_DEFAULT_QUEUE_SLOTS 64

// Default framebuffer dimensions used when an adapter does not provide a
// native screenshot size.
#define DMCP_DEFAULT_SCREENSHOT_WIDTH 640
#define DMCP_DEFAULT_SCREENSHOT_HEIGHT 480

// ============================================================================
// Game-Specific Array Limits
// ============================================================================

#define DMCP_MAX_ENEMIES 1024
#define DMCP_MAX_INVENTORY 64
#define DMCP_MAX_ENTITIES 1024
#define DMCP_MAX_ITEM_NAME 64
#define DMCP_MAX_ENEMY_TYPE 128
#define DMCP_MAX_ENTITY_TYPE 128
#define DMCP_MAX_LEVEL_ID 32
#define DMCP_MAX_LEVEL_NAME 96
#define DMCP_MAX_MAPS 128
#define DMCP_MAX_MAP_NAME 16

// ============================================================================
// Player Limits (from Doom engine: deh_misc.h, p_local.h)
// ============================================================================

#define DMCP_PLAYER_INITIAL_HEALTH 100
#define DMCP_PLAYER_MAX_HEALTH 200
#define DMCP_PLAYER_MAX_ARMOR 200
#define DMCP_PLAYER_GOD_HEALTH 100
#define DMCP_ARMOR_GREEN_LIMIT 100
#define DMCP_ARMOR_BLUE_LIMIT 200

// ============================================================================
// Item Health Bonuses (from Doom engine: deh_misc.h)
// ============================================================================

#define DMCP_ITEM_HEALTH_BONUS 1
#define DMCP_ITEM_STIMPACK 10
#define DMCP_ITEM_MEDIKIT 25
#define DMCP_ITEM_SOULSPHERE 100
#define DMCP_ITEM_MEGASPHERE 200

// ============================================================================
// Combat Limits
// ============================================================================

#define DMCP_DAMAGE_MIN 1
#define DMCP_DAMAGE_MAX 10000
#define DMCP_OVERKILL_BONUS 1024

// ============================================================================
// Ammo Limits
// ============================================================================

#define DMCP_AMMO_MAX_DEFAULT 500

// ============================================================================
// Entity Limits
// ============================================================================

#define DMCP_TID_MIN 0
#define DMCP_TID_MAX 32767

// ============================================================================
// Skill Levels
// ============================================================================

#define DMCP_SKILL_MIN 1
#define DMCP_SKILL_MAX 5

// ============================================================================
// Episode/Map Limits
// ============================================================================

#define DMCP_EPISODE_MIN 1
#define DMCP_EPISODE_MAX_DOOM1 4
#define DMCP_EPISODE_MAX_SHAREWARE 1
#define DMCP_MAP_MIN 1
#define DMCP_MAP_MAX_DOOM1 9
#define DMCP_MAP_MAX_DOOM2 32

// ============================================================================
// Game Modes (matching Chocolate Doom's GameMode_t)
// ============================================================================

typedef enum {
  DMCP_GAMEMODE_UNKNOWN = 0,
  DMCP_GAMEMODE_SHAREWARE,
  DMCP_GAMEMODE_REGISTERED,
  DMCP_GAMEMODE_COMMERCIAL,
  DMCP_GAMEMODE_RETAIL
} dmcp_gamemode_t;

// ============================================================================
// Game Missions (matching Chocolate Doom's GameMission_t)
// ============================================================================

typedef enum {
  DMCP_GAMEMISSION_UNKNOWN = 0,
  DMCP_GAMEMISSION_DOOM1,
  DMCP_GAMEMISSION_DOOM2,
  DMCP_GAMEMISSION_TNT,
  DMCP_GAMEMISSION_PLUTONIA,
  DMCP_GAMEMISSION_HERETIC,
  DMCP_GAMEMISSION_HEXEN,
  DMCP_GAMEMISSION_STRIFE
} dmcp_gamemission_t;

#ifdef __cplusplus
}
#endif
