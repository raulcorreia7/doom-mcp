// Level and Game State Extraction
// Populates dmcp_level_t and dmcp_game_t from Crispy Doom globals

#include "dmcp_crispy.h"
#include "crispy_types.h"

#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"

#ifdef CRISPY_DOOM_BUILD
#include "hu_stuff.h"

// Level name arrays from hu_stuff.c
extern const char* mapnames[];
extern const char* mapnames_commercial[];
#endif

#ifdef CRISPY_DOOM_BUILD
static void dmcp_crispy_format_map_id(char* out, size_t out_size, int episode, int map) {
  if (!out || out_size == 0) return;

  if (D_IsEpisodeMap(gamemission)) {
    snprintf(out, out_size, "E%dM%d", episode, map);
  } else {
    snprintf(out, out_size, "MAP%02d", map);
  }
}

static void dmcp_crispy_rebuild_map_catalog(dmcp_map_t* maps, uint32_t* count) {
  int episode;
  int map;

  if (!maps || !count) return;

  *count = 0;
  if (gamemode == indetermined || gamemission == none) return;

  if (D_IsEpisodeMap(gamemission)) {
    for (episode = 1; episode <= 9 && *count < DMCP_MAX_MAPS; ++episode) {
      for (map = 1; map <= 9 && *count < DMCP_MAX_MAPS; ++map) {
        if (D_ValidEpisodeMap(gamemission, gamemode, episode, map)) {
          dmcp_crispy_format_map_id(maps[*count].name, sizeof(maps[*count].name), episode, map);
          (*count)++;
        }
      }
    }
    return;
  }

  for (map = 1; map <= 99 && *count < DMCP_MAX_MAPS; ++map) {
    if (D_ValidEpisodeMap(gamemission, gamemode, 1, map)) {
      dmcp_crispy_format_map_id(maps[*count].name, sizeof(maps[*count].name), 1, map);
      (*count)++;
    }
  }
}

static void dmcp_crispy_populate_maps(dmcp_snapshot_t* snap) {
  static GameMission_t cached_mission = none;
  static GameMode_t    cached_mode    = indetermined;
  static int           cache_ready    = 0;
  static dmcp_map_t    cached_maps[DMCP_MAX_MAPS];
  static uint32_t      cached_map_count = 0;
  uint32_t             i;

  if (!snap) return;

  if (!cache_ready || cached_mission != gamemission || cached_mode != gamemode) {
    dmcp_crispy_rebuild_map_catalog(cached_maps, &cached_map_count);
    cached_mission = gamemission;
    cached_mode    = gamemode;
    cache_ready    = 1;
  }

  snap->map_count = 0;
  for (i = 0; i < cached_map_count; ++i) {
    dmcp_snapshot_add_map(snap, cached_maps[i].name);
  }
}
#else
static void dmcp_crispy_populate_maps(dmcp_snapshot_t* snap) {
  if (snap) {
    snap->map_count = 0;
  }
}
#endif

void dmcp_crispy_populate_level(dmcp_snapshot_t* snap) {
  dmcp_level_t* level;
  dmcp_game_t*  game;
  player_t*     p;
  int           current_episode;
  int           current_map;

  if (!snap) return;

  level = &snap->level;

  level->tic       = leveltime;
  level->leveltime = leveltime;

  current_episode = gameepisode;
  current_map     = gamemap;
#ifdef CRISPY_DOOM_BUILD
  dmcp_crispy_format_map_id(level->level_id, sizeof(level->level_id), current_episode, current_map);
#else
  snprintf(level->level_id, sizeof(level->level_id), "E%dM%d", current_episode, current_map);
#endif

#ifdef CRISPY_DOOM_BUILD
  // Get level name from mapnames array using current game mode
  if (gamemode == commercial) {
    // Doom II, Plutonia, TNT
    if (current_map >= 1 && current_map <= 32) {
      mcp_strcpy_safe(level->level_name, sizeof(level->level_name),
                      mapnames_commercial[current_map - 1]);
    } else {
      level->level_name[0] = '\0';
    }
  } else {
    // Ultimate Doom, Shareware, Registered
    int idx = (current_episode - 1) * 9 + (current_map - 1);
    if (idx >= 0 && idx < 45) {
      mcp_strcpy_safe(level->level_name, sizeof(level->level_name), mapnames[idx]);
    } else {
      level->level_name[0] = '\0';
    }
  }
#endif

  // Initialize counters
  level->kill_count   = 0;
  level->item_count   = 0;
  level->secret_count = 0;

  // Get current player pointer dynamically with validation
  if (consoleplayer >= 0 && consoleplayer < MAXPLAYERS && playeringame[consoleplayer]) {
    p = &players[consoleplayer];
    if (p) {
      level->kill_count   = p->killcount;
      level->item_count   = p->itemcount;
      level->secret_count = p->secretcount;
    }
  }

  // Get current global counters
  level->totalkills   = totalkills;
  level->totalitems   = totalitems;
  level->totalsecrets = totalsecret;

  // Get current game state and settings
  mcp_strcpy_safe(level->skill, DMCP_MAX_STRING, dmcp_skill_to_string(gameskill));
  mcp_strcpy_safe(level->gamestate, DMCP_MAX_STRING, dmcp_gamestate_to_string(gamestate));
  level->paused = paused;

  game = &snap->game;
  mcp_strcpy_safe(game->mode, DMCP_MAX_STRING, dmcp_gamemode_to_string(netgame, deathmatch));
  if (gamemode == shareware) {
    mcp_strcpy_safe(game->version, DMCP_MAX_STRING, "shareware");
  } else if (gamemode == registered) {
    mcp_strcpy_safe(game->version, DMCP_MAX_STRING, "registered");
  } else if (gamemode == commercial) {
    mcp_strcpy_safe(game->version, DMCP_MAX_STRING, "commercial");
  } else {
    mcp_strcpy_safe(game->version, DMCP_MAX_STRING, "retail");
  }
  game->respawnmonsters = respawnmonsters;
  game->consoleplayer   = consoleplayer;

  dmcp_crispy_populate_maps(snap);
}
