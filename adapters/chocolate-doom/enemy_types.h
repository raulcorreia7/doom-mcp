// Enemy Type Lookup Table
// Data-driven mapping from Chocolate Doom mobj_type to human-readable names

#ifndef DMCP_ENEMY_TYPES_H
#define DMCP_ENEMY_TYPES_H

#include <stddef.h>

#ifdef CHOCOLATE_DOOM_BUILD
#include "info.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int         mobj_type;
  const char* name;
} dmcp_enemy_type_entry_t;

static const dmcp_enemy_type_entry_t dmcp_enemy_type_table[] = {
#ifdef CHOCOLATE_DOOM_BUILD
    {MT_POSSESSED, "Zombieman"}, {MT_SHOTGUY, "Shotgun Guy"},      {MT_VILE, "Archvile"},
    {MT_UNDEAD, "Revenant"},     {MT_FATSO, "Mancubus"},           {MT_CHAINGUY, "Chaingunner"},
    {MT_TROOP, "Imp"},           {MT_SERGEANT, "Demon"},           {MT_SHADOWS, "Spectre"},
    {MT_HEAD, "Cacodemon"},      {MT_BRUISER, "Baron of Hell"},    {MT_KNIGHT, "Hell Knight"},
    {MT_SKULL, "Lost Soul"},     {MT_SPIDER, "Spider Mastermind"}, {MT_BABY, "Arachnotron"},
    {MT_CYBORG, "Cyberdemon"},   {MT_PAIN, "Pain Elemental"},
#endif
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
