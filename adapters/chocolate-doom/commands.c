// Command execution for Chocolate Doom.

#include "dmcp_adapter.h"
#include "dmcp_mappings.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "dmcp/adapter/content.h"
#include "dmcp/adapter/validation.h"

#include "d_player.h"
#include "d_think.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "info.h"
#include "m_fixed.h"
#include "p_local.h"
#include "p_mobj.h"
#include "tables.h"

static int dmcp_ascii_tolower(int c) {
  if (c >= 'A' && c <= 'Z') {
    return c - 'A' + 'a';
  }
  return c;
}

static bool dmcp_str_equals_ci(const char* a, const char* b);

static bool dmcp_str_equals_ci(const char* a, const char* b) {
  if (!a || !b) {
    return false;
  }

  while (*a && *b) {
    if (dmcp_ascii_tolower((unsigned char)*a) != dmcp_ascii_tolower((unsigned char)*b)) {
      return false;
    }
    ++a;
    ++b;
  }

  return *a == '\0' && *b == '\0';
}

static int dmcp_clamp_int(int value, int min_value, int max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

static player_t* dmcp_get_player(void) {
  if (!playeringame[consoleplayer]) {
    return NULL;
  }
  return &players[consoleplayer];
}

static bool dmcp_parse_map_name(const char* map_name, int* out_episode, int* out_map) {
  int episode = 0;
  int map     = 0;

  if (!map_name || !out_episode || !out_map || !map_name[0]) {
    return false;
  }

  if (sscanf(map_name, "E%dM%d", &episode, &map) == 2) {
    if (episode < 1 || episode > 4 || map < 1 || map > 9) {
      return false;
    }
    *out_episode = episode;
    *out_map     = map;
    return true;
  }

  if (sscanf(map_name, "MAP%d", &map) == 1) {
    if (map < 1 || map > 32) {
      return false;
    }
    *out_episode = 1;
    *out_map     = map;
    return true;
  }

  return false;
}

static bool dmcp_resolve_spawn_type(const char* entity_class, mobjtype_t* out_type) {
  if (!entity_class || !entity_class[0] || !out_type) {
    return false;
  }

  if (dmcp_str_equals_ci(entity_class, "Imp") || dmcp_str_equals_ci(entity_class, "DoomImp")) {
    *out_type = MT_TROOP;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "Zombieman") || dmcp_str_equals_ci(entity_class, "Zombie")) {
    *out_type = MT_POSSESSED;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "ShotgunGuy") ||
      dmcp_str_equals_ci(entity_class, "Shotgun Guy")) {
    *out_type = MT_SHOTGUY;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "Demon") || dmcp_str_equals_ci(entity_class, "Pinky")) {
    *out_type = MT_SERGEANT;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "Spectre")) {
    *out_type = MT_SHADOWS;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "Cacodemon")) {
    *out_type = MT_HEAD;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "BaronOfHell") ||
      dmcp_str_equals_ci(entity_class, "Baron of Hell")) {
    *out_type = MT_BRUISER;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "HellKnight") ||
      dmcp_str_equals_ci(entity_class, "Hell Knight")) {
    *out_type = MT_KNIGHT;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "LostSoul") ||
      dmcp_str_equals_ci(entity_class, "Lost Soul")) {
    *out_type = MT_SKULL;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "Arachnotron")) {
    *out_type = MT_BABY;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "PainElemental") ||
      dmcp_str_equals_ci(entity_class, "Pain Elemental")) {
    *out_type = MT_PAIN;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "Revenant")) {
    *out_type = MT_UNDEAD;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "Mancubus")) {
    *out_type = MT_FATSO;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "Archvile") ||
      dmcp_str_equals_ci(entity_class, "Arch-vile")) {
    *out_type = MT_VILE;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "SpiderMastermind") ||
      dmcp_str_equals_ci(entity_class, "Spider Mastermind")) {
    *out_type = MT_SPIDER;
    return true;
  }
  if (dmcp_str_equals_ci(entity_class, "Cyberdemon")) {
    *out_type = MT_CYBORG;
    return true;
  }

  return false;
}

static bool dmcp_give_item(player_t* player, const dmcp_cmd_give_item_t* give) {
  int amount;

  if (!player || !player->mo || !give || !give->item_class[0]) {
    return false;
  }

  if (!dmcp_is_item_available(give->item_class, dmcp_to_gamemode(gamemode))) {
    return false;
  }

  amount = dmcp_clamp_int(give->amount, 1, 1000);

  if (dmcp_str_equals_ci(give->item_class, "Pistol")) {
    player->weaponowned[wp_pistol] = 1;
    player->pendingweapon          = wp_pistol;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Shotgun")) {
    player->weaponowned[wp_shotgun] = 1;
    player->pendingweapon           = wp_shotgun;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Chaingun")) {
    player->weaponowned[wp_chaingun] = 1;
    player->pendingweapon            = wp_chaingun;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "RocketLauncher") ||
      dmcp_str_equals_ci(give->item_class, "Rocket Launcher")) {
    player->weaponowned[wp_missile] = 1;
    player->pendingweapon           = wp_missile;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "PlasmaRifle") ||
      dmcp_str_equals_ci(give->item_class, "Plasma Rifle")) {
    player->weaponowned[wp_plasma] = 1;
    player->pendingweapon          = wp_plasma;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "BFG9000") ||
      dmcp_str_equals_ci(give->item_class, "BFG")) {
    player->weaponowned[wp_bfg] = 1;
    player->pendingweapon       = wp_bfg;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Chainsaw")) {
    player->weaponowned[wp_chainsaw] = 1;
    player->pendingweapon            = wp_chainsaw;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "SuperShotgun") ||
      dmcp_str_equals_ci(give->item_class, "Super Shotgun")) {
    player->weaponowned[wp_supershotgun] = 1;
    player->pendingweapon                = wp_supershotgun;
    return true;
  }

  if (dmcp_str_equals_ci(give->item_class, "Clip") ||
      dmcp_str_equals_ci(give->item_class, "Bullets")) {
    player->ammo[am_clip] += 10 * amount;
    player->ammo[am_clip] = dmcp_clamp_int(player->ammo[am_clip], 0, player->maxammo[am_clip]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Shell") ||
      dmcp_str_equals_ci(give->item_class, "Shells")) {
    player->ammo[am_shell] += 4 * amount;
    player->ammo[am_shell] = dmcp_clamp_int(player->ammo[am_shell], 0, player->maxammo[am_shell]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Rocket") ||
      dmcp_str_equals_ci(give->item_class, "Rockets")) {
    player->ammo[am_misl] += amount;
    player->ammo[am_misl] = dmcp_clamp_int(player->ammo[am_misl], 0, player->maxammo[am_misl]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Cell") ||
      dmcp_str_equals_ci(give->item_class, "Cells")) {
    player->ammo[am_cell] += 20 * amount;
    player->ammo[am_cell] = dmcp_clamp_int(player->ammo[am_cell], 0, player->maxammo[am_cell]);
    return true;
  }

  if (dmcp_str_equals_ci(give->item_class, "Stimpack")) {
    player->health =
        dmcp_clamp_int(player->health + (DMCP_ITEM_STIMPACK * amount), 1, DMCP_PLAYER_MAX_HEALTH);
    player->mo->health = player->health;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Medikit")) {
    player->health =
        dmcp_clamp_int(player->health + (DMCP_ITEM_MEDIKIT * amount), 1, DMCP_PLAYER_MAX_HEALTH);
    player->mo->health = player->health;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "GreenArmor") ||
      dmcp_str_equals_ci(give->item_class, "Green Armor")) {
    player->armorpoints = DMCP_ARMOR_GREEN_LIMIT;
    player->armortype   = 1;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "BlueArmor") ||
      dmcp_str_equals_ci(give->item_class, "Blue Armor")) {
    player->armorpoints = DMCP_PLAYER_MAX_ARMOR;
    player->armortype   = 2;
    return true;
  }

  if (dmcp_str_equals_ci(give->item_class, "Backpack")) {
    int i;
    if (!player->backpack) {
      for (i = 0; i < NUMAMMO; ++i) {
        player->maxammo[i] *= 2;
      }
      player->backpack = true;
    }
    return true;
  }

  return false;
}

static mobj_t* dmcp_find_enemy_by_id(int enemy_id) {
  thinker_t* thinker;
  int        count;

  if (enemy_id < 0) {
    return NULL;
  }

  count = 0;
  for (thinker = thinkercap.next; thinker != &thinkercap; thinker = thinker->next) {
    mobj_t* mobj;

    if (!thinker || thinker->function.acp1 != (actionf_p1)P_MobjThinker) {
      continue;
    }

    mobj = (mobj_t*)thinker;
    if (!mobj || !mobj->info) {
      continue;
    }

    if (!(mobj->flags & MF_COUNTKILL) || (mobj->flags & MF_CORPSE) || mobj->health <= 0) {
      continue;
    }

    if (count == enemy_id) {
      return mobj;
    }
    count++;
  }

  return NULL;
}

static bool dmcp_execute_spawn_entity(const dmcp_cmd_spawn_t* spawn) {
  mobjtype_t type;
  mobj_t*    spawned;
  fixed_t    x;
  fixed_t    y;

  if (!spawn || gamestate != GS_LEVEL) {
    return false;
  }
  if (!dmcp_resolve_spawn_type(spawn->entity_class, &type)) {
    return false;
  }
  if (!dmcp_is_enemy_spawnable(spawn->entity_class, dmcp_to_gamemode(gamemode))) {
    return false;
  }

  x = dmcp_float_to_fixed(spawn->position.x);
  y = dmcp_float_to_fixed(spawn->position.y);

  spawned = P_SpawnMobj(x, y, ONFLOORZ, type);
  if (!spawned) {
    return false;
  }

  if (!P_CheckPosition(spawned, x, y)) {
    P_RemoveMobj(spawned);
    return false;
  }

  spawned->angle = dmcp_degrees_to_angle(spawn->angle);
  return true;
}

static bool dmcp_execute_change_level(const dmcp_cmd_change_level_t* change_level) {
  int     episode;
  int     map;
  int     skill_level;
  skill_t skill;

  if (!change_level) {
    return false;
  }
  if (!dmcp_parse_map_name(change_level->map_name, &episode, &map)) {
    return false;
  }

  skill_level = dmcp_clamp_int(change_level->skill_level, 1, 5);
  skill       = (skill_t)(skill_level - 1);
  G_DeferedInitNew(skill, episode, map);
  return true;
}

static int dmcp_health_points_from_command(float requested_health) {
  if (requested_health > 0.0f && requested_health <= 1.0f) {
    requested_health *= 100.0f;
  }

  return dmcp_clamp_int((int)requested_health, 1, DMCP_PLAYER_MAX_HEALTH);
}

static bool dmcp_execute_set_player_health(player_t*                    player,
                                           const dmcp_cmd_set_health_t* set_health) {
  int health;

  if (!player || !player->mo || !set_health) {
    dmcp_adapter_log(MCP_LOG_WARN, "set_player_health: invalid arguments");
    return false;
  }

  health = dmcp_health_points_from_command(set_health->health);

  // Validate using adapter helper
  if (!dmcp_validate_health(health)) {
    dmcp_adapter_log(MCP_LOG_WARN, "set_player_health: health %d out of range (1-%d)", health,
                     DMCP_PLAYER_MAX_HEALTH);
    return false;
  }

  player->health     = health;
  player->mo->health = health;
  return true;
}

static bool dmcp_execute_set_player_position(player_t*                      player,
                                             const dmcp_cmd_set_position_t* set_position) {
  if (!player || !player->mo || !set_position) {
    return false;
  }
  if (!P_TeleportMove(player->mo, dmcp_float_to_fixed(set_position->position.x),
                      dmcp_float_to_fixed(set_position->position.y))) {
    return false;
  }

  player->mo->angle = dmcp_degrees_to_angle(set_position->angle);
  return true;
}

static void dmcp_normalize_command(char* out, size_t out_size, const char* in) {
  size_t write_index;
  bool   previous_space;

  if (!out || out_size == 0) {
    return;
  }

  out[0] = '\0';
  if (!in) {
    return;
  }

  write_index    = 0;
  previous_space = true;
  while (*in && write_index + 1 < out_size) {
    const unsigned char ch = (unsigned char)*in++;
    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
      if (!previous_space) {
        out[write_index++] = ' ';
        previous_space     = true;
      }
      continue;
    }

    out[write_index++] = (char)dmcp_ascii_tolower(ch);
    previous_space     = false;
  }

  if (write_index > 0 && out[write_index - 1] == ' ') {
    --write_index;
  }
  out[write_index] = '\0';
}

static bool dmcp_execute_console(player_t* player, const dmcp_cmd_console_t* console_cmd) {
  char normalized[256];

  if (!player || !console_cmd) {
    return false;
  }

  dmcp_normalize_command(normalized, sizeof(normalized), console_cmd->command);
  if (normalized[0] == '\0') {
    return false;
  }

  if (dmcp_str_equals_ci(normalized, "god") || dmcp_str_equals_ci(normalized, "godmode") ||
      dmcp_str_equals_ci(normalized, "iddqd") || dmcp_str_equals_ci(normalized, "god on") ||
      dmcp_str_equals_ci(normalized, "godmode on")) {
    player->cheats |= CF_GODMODE;
    return true;
  }

  if (dmcp_str_equals_ci(normalized, "ungod") || dmcp_str_equals_ci(normalized, "god off") ||
      dmcp_str_equals_ci(normalized, "godmode off")) {
    player->cheats &= ~CF_GODMODE;
    return true;
  }

  if (dmcp_str_equals_ci(normalized, "noclip") || dmcp_str_equals_ci(normalized, "noclip on") ||
      dmcp_str_equals_ci(normalized, "idspispopd")) {
    player->cheats |= CF_NOCLIP;
    return true;
  }

  if (dmcp_str_equals_ci(normalized, "clip") || dmcp_str_equals_ci(normalized, "noclip off")) {
    player->cheats &= ~CF_NOCLIP;
    return true;
  }

  if (dmcp_str_equals_ci(normalized, "pause")) {
    paused = true;
    return true;
  }

  if (dmcp_str_equals_ci(normalized, "resume") || dmcp_str_equals_ci(normalized, "unpause")) {
    paused = false;
    return true;
  }

  return false;
}

static bool dmcp_execute_pause(const dmcp_cmd_pause_t* pause_cmd) {
  if (!pause_cmd) {
    return false;
  }
  paused = pause_cmd->paused;
  return true;
}

static bool dmcp_execute_damage_entity(player_t* player, const dmcp_cmd_damage_t* damage_cmd) {
  mobj_t* target;
  mobj_t* source;
  int     damage;

  if (!damage_cmd) {
    dmcp_adapter_log(MCP_LOG_WARN, "damage_entity: null command");
    return false;
  }

  // Validate damage amount using adapter helper
  damage = (int)damage_cmd->damage;
  if (!dmcp_validate_damage(damage)) {
    dmcp_adapter_log(MCP_LOG_WARN, "damage_entity: damage %d out of range (1-%d)", damage,
                     DMCP_DAMAGE_MAX);
    return false;
  }

  target = dmcp_find_enemy_by_id(damage_cmd->target_tid);
  if (!target) {
    dmcp_adapter_log(MCP_LOG_WARN, "damage_entity: target %d not found", damage_cmd->target_tid);
    return false;
  }

  source = (player && player->mo) ? player->mo : NULL;
  P_DamageMobj(target, NULL, source, damage);
  return true;
}

static bool dmcp_execute_kill_entity(player_t* player, const dmcp_cmd_kill_t* kill_cmd) {
  mobj_t* target;
  mobj_t* source;
  int     damage;

  if (!kill_cmd) {
    return false;
  }

  target = dmcp_find_enemy_by_id(kill_cmd->target_tid);
  if (!target) {
    return false;
  }

  source = (player && player->mo) ? player->mo : NULL;
  damage = dmcp_clamp_int(target->health + DMCP_OVERKILL_BONUS, 1, 32000);
  P_DamageMobj(target, NULL, source, damage);
  return true;
}

bool dmcp_chocolate_command_execute(dmcp_chocolate_t* ctx, const dmcp_command_t* cmd) {
  player_t* player;

  if (!ctx || !cmd) {
    return false;
  }

  player = dmcp_get_player();

  switch (cmd->type) {
    case DMCP_CMD_SPAWN_ENTITY:
      return dmcp_execute_spawn_entity(&cmd->data.spawn);

    case DMCP_CMD_CHANGE_LEVEL:
      return dmcp_execute_change_level(&cmd->data.change_level);

    case DMCP_CMD_GIVE_ITEM:
      return dmcp_give_item(player, &cmd->data.give_item);

    case DMCP_CMD_SET_PLAYER_HEALTH:
      return dmcp_execute_set_player_health(player, &cmd->data.set_health);

    case DMCP_CMD_SET_PLAYER_POSITION:
      return dmcp_execute_set_player_position(player, &cmd->data.set_position);

    case DMCP_CMD_EXECUTE_CONSOLE:
      return dmcp_execute_console(player, &cmd->data.console);

    case DMCP_CMD_PAUSE_GAME:
      return dmcp_execute_pause(&cmd->data.pause);

    case DMCP_CMD_SET_TIMESCALE:
      // Doom simulation timing is fixed at 35 Hz in Chocolate Doom.
      return false;

    case DMCP_CMD_DAMAGE_ENTITY:
      return dmcp_execute_damage_entity(player, &cmd->data.damage);

    case DMCP_CMD_KILL_ENTITY:
      return dmcp_execute_kill_entity(player, &cmd->data.kill);

    default:
      return false;
  }
}
