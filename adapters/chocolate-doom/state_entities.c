// World Entity State Extraction
// Populates non-enemy interactive entities (pickups and barrels)

#include "dmcp_adapter.h"
#include "dmcp_mappings.h"

#include <string.h>

#include "d_think.h"
#include "doomdef.h"
#include "doomstat.h"
#include "info.h"
#include "p_local.h"
#include "p_mobj.h"

static const char* dmcp_world_entity_type_name(mobjtype_t type) {
  switch (type) {
    case MT_SHOTGUN:
      return "Shotgun";
    case MT_SUPERSHOTGUN:
      return "SuperShotgun";
    case MT_CHAINGUN:
      return "Chaingun";
    case MT_MISC27:
      return "RocketLauncher";
    case MT_MISC28:
      return "PlasmaRifle";
    case MT_MISC25:
      return "BFG9000";
    case MT_MISC26:
      return "Chainsaw";
    case MT_CLIP:
      return "Clip";
    case MT_MISC17:
      return "BoxOfBullets";
    case MT_MISC22:
      return "Shell";
    case MT_MISC23:
      return "BoxOfShells";
    case MT_MISC18:
      return "Rocket";
    case MT_MISC19:
      return "BoxOfRockets";
    case MT_MISC20:
      return "Cell";
    case MT_MISC21:
      return "CellPack";
    case MT_MISC10:
      return "Stimpack";
    case MT_MISC11:
      return "Medikit";
    case MT_MISC12:
      return "SoulSphere";
    case MT_MEGA:
      return "MegaSphere";
    case MT_MISC0:
      return "GreenArmor";
    case MT_MISC1:
      return "BlueArmor";
    case MT_MISC24:
      return "Backpack";
    case MT_INV:
      return "Invulnerability";
    case MT_MISC13:
      return "Berserk";
    case MT_INS:
      return "Invisibility";
    case MT_MISC14:
      return "RadiationSuit";
    case MT_MISC15:
      return "ComputerMap";
    case MT_MISC16:
      return "LightAmp";
    case MT_BARREL:
      return "Barrel";
    default:
      return "Pickup";
  }
}

static int dmcp_world_entity_is_enemy(const mobj_t* mo) {
  if (!mo) {
    return 0;
  }

  return (mo->flags & MF_COUNTKILL) && !(mo->flags & MF_CORPSE) && mo->health > 0;
}

static int dmcp_world_entity_is_interactive(const mobj_t* mo) {
  if (!mo || !mo->info) {
    return 0;
  }

  if ((mo->flags & MF_MISSILE) != 0) {
    return 0;
  }

  if (mo->player != NULL) {
    return 0;
  }

  if (dmcp_world_entity_is_enemy(mo)) {
    return 0;
  }

  if ((mo->flags & MF_SPECIAL) != 0) {
    return 1;
  }

  if (mo->type == MT_BARREL) {
    return 1;
  }

  return 0;
}

int dmcp_chocolate_populate_entities(dmcp_snapshot_t* snap) {
  thinker_t*    th;
  mobj_t*       mo;
  dmcp_entity_t entity;
  int           count;

  count = 0;
  if (!snap) return 0;

  for (th = thinkercap.next; th != &thinkercap && count < DMCP_MAX_ENTITIES; th = th->next) {
    if (!th) {
      continue;
    }

    if (th->function.acp1 != (actionf_p1)P_MobjThinker) {
      continue;
    }

    mo = (mobj_t*)th;
    if (!dmcp_world_entity_is_interactive(mo)) {
      continue;
    }

    memset(&entity, 0, sizeof(entity));
    entity.id     = count;
    entity.hp     = mo->health;
    entity.max_hp = mo->info ? mo->info->spawnhealth : 0;

    entity.position.x = dmcp_fixed_to_float(mo->x);
    entity.position.y = dmcp_fixed_to_float(mo->y);
    entity.position.z = dmcp_fixed_to_float(mo->z);
    entity.angle      = dmcp_angle_to_radians(mo->angle);

    dmcp_strcpy_safe(entity.type, dmcp_world_entity_type_name(mo->type), sizeof(entity.type));
    dmcp_snapshot_add_entity(snap, &entity);
    count++;
  }

  return count;
}
