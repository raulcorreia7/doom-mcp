// Player State Extraction
// Populates dmcp_player_t from Crispy Doom's player_t

#include "dmcp_crispy.h"
#include "entity_kinds.h"
#include "crispy_types.h"

#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"
#include "p_mobj.h"

void dmcp_crispy_populate_player(dmcp_snapshot_t* snap) {
  player_t*      p;
  dmcp_player_t* player;
  int            i;
  int            current_consoleplayer;

  if (!snap) return;

  current_consoleplayer = consoleplayer;
  if (current_consoleplayer < 0 || current_consoleplayer >= MAXPLAYERS) return;
  if (!playeringame[current_consoleplayer]) return;

  // Get current player pointer dynamically
  p = &players[current_consoleplayer];
  if (!p) return;

  // Get current player's mobj pointer dynamically
  if (!p->mo) return;

  player = &snap->player;

  // Get current player health and armor
  player->hp    = p->health;
  player->armor = p->armorpoints;
  dmcp_strcpy_safe(player->armortype, dmcp_armortype_to_string(p->armortype), DMCP_MAX_STRING);

  // Get current player position and angle
  player->position.x = dmcp_fixed_to_float(p->mo->x);
  player->position.y = dmcp_fixed_to_float(p->mo->y);
  player->angle      = dmcp_angle_to_radians(p->mo->angle);

  // Get current weapon states
  dmcp_strcpy_safe(player->readyweapon, dmcp_weapon_to_string(p->readyweapon), DMCP_MAX_STRING);
  if (p->pendingweapon >= 0 && p->pendingweapon < NUMWEAPONS) {
    dmcp_strcpy_safe(player->pendingweapon, dmcp_weapon_to_string(p->pendingweapon),
                     DMCP_MAX_STRING);
  } else {
    dmcp_strcpy_safe(player->pendingweapon, "None", DMCP_MAX_STRING);
  }

  // Get current weapon ownership
  for (i = 0; i < NUMWEAPONS; i++) {
    player->weaponowned[i] = p->weaponowned[i];
  }

  // Get current ammo states
  for (i = 0; i < NUMAMMO; i++) {
    player->ammo[i]    = p->ammo[i];
    player->maxammo[i] = p->maxammo[i];
  }
  player->backpack = p->backpack;

  // Get current powerups and cards
  for (i = 0; i < NUMPOWERS; i++) {
    player->powers[i] = p->powers[i];
    player->cards[i]  = p->cards[i];
  }

  // Get current player state
  dmcp_strcpy_safe(player->playerstate, dmcp_playerstate_to_string(p->playerstate),
                   DMCP_MAX_STRING);
  player->cheats      = p->cheats;
  player->damagecount = p->damagecount;
}
