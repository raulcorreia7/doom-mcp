// Level and Game State Extraction
// Populates dmcp_level_t and dmcp_game_t from Chocolate Doom globals

#include "dmcp_adapter.h"
#include "dmcp_mappings.h"

#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"

#ifdef CHOCOLATE_DOOM_BUILD
#include "hu_stuff.h"

// Level name arrays from hu_stuff.c
extern const char* mapnames[];
extern const char* mapnames_commercial[];
#endif

void dmcp_chocolate_populate_level(dmcp_snapshot_t* snap) {
  dmcp_level_t* level;
  dmcp_game_t*  game;
  player_t*     p;

  if (!snap) return;

  level = &snap->level;

  level->tic       = leveltime;
  level->leveltime = leveltime;

  snprintf(level->level_id, sizeof(level->level_id), "E%dM%d", gameepisode, gamemap);

#ifdef CHOCOLATE_DOOM_BUILD
  // Get level name from mapnames array
  if (gamemode == commercial) {
    // Doom II, Plutonia, TNT
    if (gamemap >= 1 && gamemap <= 32) {
      dmcp_strcpy_safe(level->level_name, mapnames_commercial[gamemap - 1],
                       sizeof(level->level_name));
    } else {
      level->level_name[0] = '\0';
    }
  } else {
    // Ultimate Doom, Shareware, Registered
    int idx = (gameepisode - 1) * 9 + (gamemap - 1);
    if (idx >= 0 && idx < 45) {
      dmcp_strcpy_safe(level->level_name, mapnames[idx], sizeof(level->level_name));
    } else {
      level->level_name[0] = '\0';
    }
  }
#endif

  level->kill_count   = 0;
  level->item_count   = 0;
  level->secret_count = 0;

  if (playeringame[consoleplayer]) {
    p = &players[consoleplayer];
    if (p) {
      level->kill_count   = p->killcount;
      level->item_count   = p->itemcount;
      level->secret_count = p->secretcount;
    }
  }

  level->totalkills   = totalkills;
  level->totalitems   = totalitems;
  level->totalsecrets = totalsecret;

  dmcp_strcpy_safe(level->skill, dmcp_skill_to_string(gameskill), DMCP_MAX_STRING);
  dmcp_strcpy_safe(level->gamestate, dmcp_gamestate_to_string(gamestate), DMCP_MAX_STRING);
  level->paused = paused;

  game = &snap->game;
  dmcp_strcpy_safe(game->mode, dmcp_gamemode_to_string(netgame, deathmatch), DMCP_MAX_STRING);
  game->respawnmonsters = respawnmonsters;
  game->consoleplayer   = consoleplayer;
}
