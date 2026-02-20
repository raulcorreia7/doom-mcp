#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DMCP_ENTITY_ZOMBIEMAN "Zombieman"
#define DMCP_ENTITY_SHOTGUY "ShotgunGuy"
#define DMCP_ENTITY_SERGEANT "ChaingunGuy"
#define DMCP_ENTITY_IMP "DoomImp"
#define DMCP_ENTITY_DEMON "Demon"
#define DMCP_ENTITY_SPECTRE "Spectre"
#define DMCP_ENTITY_LOSTSOUL "LostSoul"
#define DMCP_ENTITY_CACODEMON "Cacodemon"
#define DMCP_ENTITY_BARON "BaronOfHell"
#define DMCP_ENTITY_HELLKNIGHT "HellKnight"
#define DMCP_ENTITY_ARACHNOTRON "Arachnotron"
#define DMCP_ENTITY_PAINELEMENTAL "PainElemental"
#define DMCP_ENTITY_REVENANT "Revenant"
#define DMCP_ENTITY_MANCUBUS "Fatso"
#define DMCP_ENTITY_ARCHVILE "Archvile"
#define DMCP_ENTITY_SPIDERBOSS "SpiderMastermind"
#define DMCP_ENTITY_CYBERDEMON "Cyberdemon"
#define DMCP_ENTITY_BOSSBRAIN "BossBrain"

#define DMCP_ITEM_PISTOL "Pistol"
#define DMCP_ITEM_SHOTGUN "Shotgun"
#define DMCP_ITEM_CHAINGUN "Chaingun"
#define DMCP_ITEM_ROCKETLAUNCHER "RocketLauncher"
#define DMCP_ITEM_PLASMA "PlasmaRifle"
#define DMCP_ITEM_BFG "BFG9000"
#define DMCP_ITEM_CHAINSAW "Chainsaw"
#define DMCP_ITEM_SUPERSHOTGUN "SuperShotgun"

#define DMCP_ITEM_BULLETCLIP "Clip"
#define DMCP_ITEM_BULLETBOX "BoxOfBullets"
#define DMCP_ITEM_SHELLS "Shells"
#define DMCP_ITEM_SHELLBOX "BoxOfShells"
#define DMCP_ITEM_ROCKET "RocketAmmo"
#define DMCP_ITEM_ROCKETBOX "BoxOfRockets"
#define DMCP_ITEM_CELL "Cell"
#define DMCP_ITEM_CELLPACK "CellPack"

#define DMCP_ITEM_STIMPACK "Stimpack"
#define DMCP_ITEM_MEDIKIT "Medikit"
#define DMCP_ITEM_SOULSPHERE "SoulSphere"
#define DMCP_ITEM_MEGASPHERE "MegaSphere"
#define DMCP_ITEM_ARMORGREEN "GreenArmor"
#define DMCP_ITEM_ARMORBLUE "BlueArmor"
#define DMCP_ITEM_BACKPACK "Backpack"

static inline bool dmcp_entity_is_monster(const char* name) {
  if (!name) return false;

  const char* monsters[] = {
      DMCP_ENTITY_ZOMBIEMAN,  DMCP_ENTITY_SHOTGUY,     DMCP_ENTITY_SERGEANT,
      DMCP_ENTITY_IMP,        DMCP_ENTITY_DEMON,       DMCP_ENTITY_SPECTRE,
      DMCP_ENTITY_LOSTSOUL,   DMCP_ENTITY_CACODEMON,   DMCP_ENTITY_BARON,
      DMCP_ENTITY_HELLKNIGHT, DMCP_ENTITY_ARACHNOTRON, DMCP_ENTITY_PAINELEMENTAL,
      DMCP_ENTITY_REVENANT,   DMCP_ENTITY_MANCUBUS,    DMCP_ENTITY_ARCHVILE,
      DMCP_ENTITY_SPIDERBOSS, DMCP_ENTITY_CYBERDEMON,  DMCP_ENTITY_BOSSBRAIN};

  for (int i = 0; i < 18; i++) {
    const char* m = monsters[i];
    int         j = 0;
    while (name[j] && m[j] && name[j] == m[j]) j++;
    if (name[j] == '\0' && m[j] == '\0') return true;
  }
  return false;
}

static inline bool dmcp_item_is_weapon(const char* name) {
  if (!name) return false;

  const char* weapons[] = {DMCP_ITEM_PISTOL,         DMCP_ITEM_SHOTGUN,     DMCP_ITEM_CHAINGUN,
                           DMCP_ITEM_ROCKETLAUNCHER, DMCP_ITEM_PLASMA,      DMCP_ITEM_BFG,
                           DMCP_ITEM_CHAINSAW,       DMCP_ITEM_SUPERSHOTGUN};

  for (int i = 0; i < 8; i++) {
    const char* w = weapons[i];
    int         j = 0;
    while (name[j] && w[j] && name[j] == w[j]) j++;
    if (name[j] == '\0' && w[j] == '\0') return true;
  }
  return false;
}

#ifdef __cplusplus
}
#endif
