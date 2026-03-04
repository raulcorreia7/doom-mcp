// Level and Game State Extraction
// Populates dmcp_level_t and dmcp_game_t from Crispy Doom globals

#include "dmcp_crispy.h"
#include "dmcp_mappings.h"

#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"

#ifdef CRISPY_DOOM_BUILD
#include "hu_stuff.h"

// Level name arrays from hu_stuff.c
extern const char* mapnames[];
extern const char* mapnames_commercial[];
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
  snprintf(level->level_id, sizeof(level->level_id), "E%dM%d", current_episode, current_map);

#ifdef CRISPY_DOOM_BUILD
  // Get level name from mapnames array using current game mode
  if (gamemode == commercial) {
    // Doom II, Plutonia, TNT
    if (current_map >= 1 && current_map <= 32) {
      dmcp_strcpy_safe(level->level_name, mapnames_commercial[current_map - 1],
                       sizeof(level->level_name));
    } else {
      level->level_name[0] = '\0';
    }
  } else {
    // Ultimate Doom, Shareware, Registered
    int idx = (current_episode - 1) * 9 + (current_map - 1);
    if (idx >= 0 && idx < 45) {
      dmcp_strcpy_safe(level->level_name, mapnames[idx], sizeof(level->level_name));
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
  dmcp_strcpy_safe(level->skill, dmcp_skill_to_string(gameskill), DMCP_MAX_STRING);
  dmcp_strcpy_safe(level->gamestate, dmcp_gamestate_to_string(gamestate), DMCP_MAX_STRING);
  level->paused = paused;

  game = &snap->game;
  dmcp_strcpy_safe(game->mode, dmcp_gamemode_to_string(netgame, deathmatch), DMCP_MAX_STRING);
  if (gamemode == shareware) {
    dmcp_strcpy_safe(game->version, "shareware", DMCP_MAX_STRING);
  } else if (gamemode == registered) {
    dmcp_strcpy_safe(game->version, "registered", DMCP_MAX_STRING);
  } else if (gamemode == commercial) {
    dmcp_strcpy_safe(game->version, "commercial", DMCP_MAX_STRING);
  } else {
    dmcp_strcpy_safe(game->version, "retail", DMCP_MAX_STRING);
  }
  game->respawnmonsters = respawnmonsters;
  game->consoleplayer   = consoleplayer;
}
