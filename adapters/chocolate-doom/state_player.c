// Player State Extraction
// Populates dmcp_player_t from Chocolate Doom's player_t

#include "dmcp_adapter.h"
#include "enemy_types.h"
#include "dmcp_mappings.h"

#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"
#include "p_mobj.h"

void dmcp_chocolate_populate_player(dmcp_snapshot_t* snap) {
  player_t*      p;
  dmcp_player_t* player;
  int            i;

  if (!snap) return;
  if (!playeringame[consoleplayer]) return;

  p = &players[consoleplayer];
  if (!p || !p->mo) return;

  player = &snap->player;

  player->hp    = (float)p->health;
  player->armor = (float)p->armorpoints;
  dmcp_strcpy_safe(player->armortype, dmcp_armortype_to_string(p->armortype), DMCP_MAX_STRING);

  player->position.x = dmcp_fixed_to_float(p->mo->x);
  player->position.y = dmcp_fixed_to_float(p->mo->y);
  player->angle      = dmcp_angle_to_radians(p->mo->angle);

  dmcp_strcpy_safe(player->readyweapon, dmcp_weapon_to_string(p->readyweapon), DMCP_MAX_STRING);
  if (p->pendingweapon >= 0 && p->pendingweapon < NUMWEAPONS) {
    dmcp_strcpy_safe(player->pendingweapon, dmcp_weapon_to_string(p->pendingweapon),
                     DMCP_MAX_STRING);
  } else {
    dmcp_strcpy_safe(player->pendingweapon, "None", DMCP_MAX_STRING);
  }

  for (i = 0; i < NUMWEAPONS; i++) {
    player->weaponowned[i] = p->weaponowned[i];
  }

  for (i = 0; i < NUMAMMO; i++) {
    player->ammo[i]    = p->ammo[i];
    player->maxammo[i] = p->maxammo[i];
  }
  player->backpack = p->backpack;

  for (i = 0; i < NUMPOWERS; i++) {
    player->powers[i] = p->powers[i];
    player->cards[i]  = p->cards[i];
  }

  dmcp_strcpy_safe(player->playerstate, dmcp_playerstate_to_string(p->playerstate),
                   DMCP_MAX_STRING);
  player->cheats      = p->cheats;
  player->damagecount = p->damagecount;
}
