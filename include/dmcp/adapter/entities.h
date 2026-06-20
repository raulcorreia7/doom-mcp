#pragma once

#include <stdbool.h>
#include <string.h>

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
#define DMCP_ENTITY_MANCUBUS "Mancubus"
#define DMCP_ENTITY_ARCHVILE "Archvile"
#define DMCP_ENTITY_SPIDERBOSS "SpiderMastermind"
#define DMCP_ENTITY_CYBERDEMON "Cyberdemon"
#define DMCP_ENTITY_BOSSBRAIN "BossBrain"

#define DMCP_ITEM_NAME_PISTOL "Pistol"
#define DMCP_ITEM_NAME_SHOTGUN "Shotgun"
#define DMCP_ITEM_NAME_CHAINGUN "Chaingun"
#define DMCP_ITEM_NAME_ROCKETLAUNCHER "RocketLauncher"
#define DMCP_ITEM_NAME_PLASMA "PlasmaRifle"
#define DMCP_ITEM_NAME_BFG "BFG9000"
#define DMCP_ITEM_NAME_CHAINSAW "Chainsaw"
#define DMCP_ITEM_NAME_SUPERSHOTGUN "SuperShotgun"

#define DMCP_ITEM_NAME_BULLETCLIP "Clip"
#define DMCP_ITEM_NAME_BULLETBOX "BoxOfBullets"
#define DMCP_ITEM_NAME_SHELLS "Shells"
#define DMCP_ITEM_NAME_SHELLBOX "BoxOfShells"
#define DMCP_ITEM_NAME_ROCKET "RocketAmmo"
#define DMCP_ITEM_NAME_ROCKETBOX "BoxOfRockets"
#define DMCP_ITEM_NAME_CELL "Cell"
#define DMCP_ITEM_NAME_CELLPACK "CellPack"

#define DMCP_ITEM_NAME_STIMPACK "Stimpack"
#define DMCP_ITEM_NAME_MEDIKIT "Medikit"
#define DMCP_ITEM_NAME_SOULSPHERE "SoulSphere"
#define DMCP_ITEM_NAME_MEGASPHERE "MegaSphere"
#define DMCP_ITEM_NAME_ARMORGREEN "GreenArmor"
#define DMCP_ITEM_NAME_ARMORBLUE "BlueArmor"
#define DMCP_ITEM_NAME_BACKPACK "Backpack"
#define DMCP_ITEM_NAME_BLUE_KEYCARD "BlueKeycard"
#define DMCP_ITEM_NAME_YELLOW_KEYCARD "YellowKeycard"
#define DMCP_ITEM_NAME_RED_KEYCARD "RedKeycard"
#define DMCP_ITEM_NAME_BLUE_SKULL_KEY "BlueSkullKey"
#define DMCP_ITEM_NAME_YELLOW_SKULL_KEY "YellowSkullKey"
#define DMCP_ITEM_NAME_RED_SKULL_KEY "RedSkullKey"

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
    if (strcmp(name, monsters[i]) == 0) return true;
  }
  return false;
}

static inline bool dmcp_item_is_weapon(const char* name) {
  if (!name) return false;

  const char* weapons[] = {DMCP_ITEM_NAME_PISTOL,   DMCP_ITEM_NAME_SHOTGUN,
                           DMCP_ITEM_NAME_CHAINGUN, DMCP_ITEM_NAME_ROCKETLAUNCHER,
                           DMCP_ITEM_NAME_PLASMA,   DMCP_ITEM_NAME_BFG,
                           DMCP_ITEM_NAME_CHAINSAW, DMCP_ITEM_NAME_SUPERSHOTGUN};

  for (int i = 0; i < 8; i++) {
    if (strcmp(name, weapons[i]) == 0) return true;
  }
  return false;
}

#ifdef __cplusplus
}
#endif
