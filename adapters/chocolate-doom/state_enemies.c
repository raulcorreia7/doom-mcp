// Enemy State Extraction
// Enumerates and populates enemies from Chocolate Doom's thinker list

#include "dmcp_adapter.h"
#include "enemy_types.h"
#include "dmcp_mappings.h"

#include "doomdef.h"
#include "doomstat.h"
#include "d_think.h"
#include "p_local.h"
#include "p_mobj.h"

int dmcp_chocolate_populate_enemies(dmcp_snapshot_t* snap) {
  thinker_t*   th;
  mobj_t*      mo;
  thinker_t*   t2;
  const char*  type_name;
  dmcp_enemy_t enemy;
  int          count;
  int          target_idx;
  int          target_found;

  count = 0;
  if (!snap) return 0;

  // Traverse current thinker list dynamically
  for (th = thinkercap.next; th != &thinkercap && count < DMCP_MAX_ENEMIES; th = th->next) {
    if (!th) {
      continue;
    }

    // Check if this thinker is a valid mobj thinker
    if (th->function.acp1 != (actionf_p1)P_MobjThinker) {
      continue;
    }

    mo = (mobj_t*)th;

    if (!mo || !mo->info) {
      continue;
    }

    // Check if this is a valid enemy (countkill, not corpse, has health)
    if ((mo->flags & MF_COUNTKILL) && !(mo->flags & MF_CORPSE) && mo->health > 0) {
      memset(&enemy, 0, sizeof(enemy));

      enemy.id     = count;
      enemy.hp     = (float)mo->health;
      enemy.max_hp = (float)mo->info->spawnhealth;

      // Get current position and angle
      enemy.position.x = dmcp_fixed_to_float(mo->x);
      enemy.position.y = dmcp_fixed_to_float(mo->y);
      enemy.position.z = dmcp_fixed_to_float(mo->z);
      enemy.angle      = dmcp_angle_to_radians(mo->angle);

      // Find target index dynamically
      if (mo->target) {
        target_found = -1;
        target_idx   = 0;
        for (t2 = thinkercap.next; t2 != &thinkercap; t2 = t2->next, target_idx++) {
          if (t2 == (thinker_t*)mo->target) {
            target_found = target_idx;
            break;
          }
        }
        enemy.target_id = target_found;
      } else {
        enemy.target_id = -1;
      }

      // Get current enemy type name
      type_name = dmcp_enemy_type_name(mo->type);
      dmcp_strcpy_safe(enemy.type, type_name, sizeof(enemy.type));

      dmcp_snapshot_add_enemy(snap, &enemy);
      count++;
    }
  }

  return count;
}
