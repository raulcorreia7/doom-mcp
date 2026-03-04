// Enemy Type Lookup Table
// Maps Crispy Doom mobj_type to canonical API names (must match content.cpp)

#ifndef DMCP_ENEMY_TYPES_H
#define DMCP_ENEMY_TYPES_H

#include <stddef.h>

#include "info.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int         mobj_type;
  const char* name;
} dmcp_enemy_type_entry_t;

static const dmcp_enemy_type_entry_t dmcp_enemy_type_table[] = {
    // Canonical API names (must match dmcp_all_enemies in content.cpp)
    {MT_POSSESSED, "Zombieman"}, {MT_SHOTGUY, "ShotgunGuy"},  {MT_CHAINGUY, "ChaingunGuy"},
    {MT_TROOP, "DoomImp"},  // Canonical: "DoomImp", not "Imp"
    {MT_SERGEANT, "Demon"},      {MT_SHADOWS, "Spectre"},     {MT_SKULL, "LostSoul"},
    {MT_HEAD, "Cacodemon"},      {MT_BRUISER, "BaronOfHell"},  // Canonical: "BaronOfHell", not
                                                               // "Baron of Hell"
    {MT_KNIGHT, "HellKnight"},  // Canonical: "HellKnight", not "Hell Knight"
    {MT_BABY, "Arachnotron"},    {MT_PAIN, "PainElemental"},  {MT_UNDEAD, "Revenant"},
    {MT_FATSO, "Mancubus"},      {MT_VILE, "Archvile"},       {MT_SPIDER, "SpiderMastermind"},
    {MT_CYBORG, "Cyberdemon"},   {MT_BOSSBRAIN, "BossBrain"},
};

#define DMCP_ENEMY_TYPE_TABLE_SIZE (sizeof(dmcp_enemy_type_table) / sizeof(dmcp_enemy_type_entry_t))

static inline const char* dmcp_enemy_type_name(int mobj_type) {
  for (size_t i = 0; i < DMCP_ENEMY_TYPE_TABLE_SIZE; i++) {
    if (dmcp_enemy_type_table[i].mobj_type == mobj_type) {
      return dmcp_enemy_type_table[i].name;
    }
  }
  return "Unknown";
}

#ifdef __cplusplus
}
#endif

#endif
