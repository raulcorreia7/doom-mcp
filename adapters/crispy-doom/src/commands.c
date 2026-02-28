// Command execution for Crispy Doom.

#include "dmcp_adapter.h"
#include "dmcp_mappings.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "dmcp/adapter/content.h"
#include "dmcp/adapter/utils.h"
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
#include "p_inter.h"
#include "tables.h"
#include "r_state.h"
#include "sounds.h"
#include "w_wad.h"

static bool dmcp_float_to_fixed_checked(float value, fixed_t* out) {
  double scaled;

  if (!out || !isfinite(value)) {
    return false;
  }

  scaled = (double)value * (double)FRACUNIT;
  if (scaled < (double)INT_MIN || scaled > (double)INT_MAX) {
    return false;
  }

  *out = (fixed_t)scaled;
  return true;
}

player_t* dmcp_get_player(void) {
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

typedef enum {
  DMCP_SPAWN_KIND_ENEMY = 0,
  DMCP_SPAWN_KIND_ITEM  = 1,
} dmcp_spawn_kind_t;

typedef struct {
  const char*       name;
  mobjtype_t        type;
  dmcp_spawn_kind_t kind;
} dmcp_spawn_mapping_t;

static const dmcp_spawn_mapping_t k_spawn_mappings[] = {
    // Enemies
    {"DoomImp", MT_TROOP, DMCP_SPAWN_KIND_ENEMY},
    {"Imp", MT_TROOP, DMCP_SPAWN_KIND_ENEMY},
    {"Zombieman", MT_POSSESSED, DMCP_SPAWN_KIND_ENEMY},
    {"Zombie", MT_POSSESSED, DMCP_SPAWN_KIND_ENEMY},
    {"ShotgunGuy", MT_SHOTGUY, DMCP_SPAWN_KIND_ENEMY},
    {"Shotgun Guy", MT_SHOTGUY, DMCP_SPAWN_KIND_ENEMY},
    {"ChaingunGuy", MT_CHAINGUY, DMCP_SPAWN_KIND_ENEMY},
    {"Chaingun Guy", MT_CHAINGUY, DMCP_SPAWN_KIND_ENEMY},
    {"Demon", MT_SERGEANT, DMCP_SPAWN_KIND_ENEMY},
    {"Pinky", MT_SERGEANT, DMCP_SPAWN_KIND_ENEMY},
    {"Spectre", MT_SHADOWS, DMCP_SPAWN_KIND_ENEMY},
    {"Cacodemon", MT_HEAD, DMCP_SPAWN_KIND_ENEMY},
    {"BaronOfHell", MT_BRUISER, DMCP_SPAWN_KIND_ENEMY},
    {"Baron of Hell", MT_BRUISER, DMCP_SPAWN_KIND_ENEMY},
    {"HellKnight", MT_KNIGHT, DMCP_SPAWN_KIND_ENEMY},
    {"Hell Knight", MT_KNIGHT, DMCP_SPAWN_KIND_ENEMY},
    {"LostSoul", MT_SKULL, DMCP_SPAWN_KIND_ENEMY},
    {"Lost Soul", MT_SKULL, DMCP_SPAWN_KIND_ENEMY},
    {"Arachnotron", MT_BABY, DMCP_SPAWN_KIND_ENEMY},
    {"PainElemental", MT_PAIN, DMCP_SPAWN_KIND_ENEMY},
    {"Pain Elemental", MT_PAIN, DMCP_SPAWN_KIND_ENEMY},
    {"Revenant", MT_UNDEAD, DMCP_SPAWN_KIND_ENEMY},
    {"Mancubus", MT_FATSO, DMCP_SPAWN_KIND_ENEMY},
    {"Archvile", MT_VILE, DMCP_SPAWN_KIND_ENEMY},
    {"Arch-vile", MT_VILE, DMCP_SPAWN_KIND_ENEMY},
    {"SpiderMastermind", MT_SPIDER, DMCP_SPAWN_KIND_ENEMY},
    {"Spider Mastermind", MT_SPIDER, DMCP_SPAWN_KIND_ENEMY},
    {"Cyberdemon", MT_CYBORG, DMCP_SPAWN_KIND_ENEMY},
    {"BossBrain", MT_BOSSBRAIN, DMCP_SPAWN_KIND_ENEMY},
    {"Icon Of Sin", MT_BOSSBRAIN, DMCP_SPAWN_KIND_ENEMY},

    // Weapon pickups
    {"Shotgun", MT_SHOTGUN, DMCP_SPAWN_KIND_ITEM},
    {"SuperShotgun", MT_SUPERSHOTGUN, DMCP_SPAWN_KIND_ITEM},
    {"Super Shotgun", MT_SUPERSHOTGUN, DMCP_SPAWN_KIND_ITEM},
    {"SSG", MT_SUPERSHOTGUN, DMCP_SPAWN_KIND_ITEM},
    {"Chaingun", MT_CHAINGUN, DMCP_SPAWN_KIND_ITEM},
    {"RocketLauncher", MT_MISC27, DMCP_SPAWN_KIND_ITEM},
    {"Rocket Launcher", MT_MISC27, DMCP_SPAWN_KIND_ITEM},
    {"PlasmaRifle", MT_MISC28, DMCP_SPAWN_KIND_ITEM},
    {"Plasma Rifle", MT_MISC28, DMCP_SPAWN_KIND_ITEM},
    {"Plasma", MT_MISC28, DMCP_SPAWN_KIND_ITEM},
    {"BFG9000", MT_MISC25, DMCP_SPAWN_KIND_ITEM},
    {"BFG", MT_MISC25, DMCP_SPAWN_KIND_ITEM},
    {"BFG 9000", MT_MISC25, DMCP_SPAWN_KIND_ITEM},
    {"Chainsaw", MT_MISC26, DMCP_SPAWN_KIND_ITEM},

    // Ammo and pickups
    {"Clip", MT_CLIP, DMCP_SPAWN_KIND_ITEM},
    {"Bullets", MT_CLIP, DMCP_SPAWN_KIND_ITEM},
    {"BoxOfBullets", MT_MISC17, DMCP_SPAWN_KIND_ITEM},
    {"Box of Bullets", MT_MISC17, DMCP_SPAWN_KIND_ITEM},
    {"Shells", MT_MISC22, DMCP_SPAWN_KIND_ITEM},
    {"Shell", MT_MISC22, DMCP_SPAWN_KIND_ITEM},
    {"BoxOfShells", MT_MISC23, DMCP_SPAWN_KIND_ITEM},
    {"Box of Shells", MT_MISC23, DMCP_SPAWN_KIND_ITEM},
    {"RocketAmmo", MT_MISC18, DMCP_SPAWN_KIND_ITEM},
    {"Rocket", MT_MISC18, DMCP_SPAWN_KIND_ITEM},
    {"Rockets", MT_MISC18, DMCP_SPAWN_KIND_ITEM},
    {"Rocket Ammo", MT_MISC18, DMCP_SPAWN_KIND_ITEM},
    {"BoxOfRockets", MT_MISC19, DMCP_SPAWN_KIND_ITEM},
    {"Box of Rockets", MT_MISC19, DMCP_SPAWN_KIND_ITEM},
    {"Cell", MT_MISC20, DMCP_SPAWN_KIND_ITEM},
    {"Cells", MT_MISC20, DMCP_SPAWN_KIND_ITEM},
    {"CellPack", MT_MISC21, DMCP_SPAWN_KIND_ITEM},
    {"Cell Pack", MT_MISC21, DMCP_SPAWN_KIND_ITEM},

    // Health / armor / powerups
    {"Stimpack", MT_MISC10, DMCP_SPAWN_KIND_ITEM},
    {"Medikit", MT_MISC11, DMCP_SPAWN_KIND_ITEM},
    {"SoulSphere", MT_MISC12, DMCP_SPAWN_KIND_ITEM},
    {"Soul Sphere", MT_MISC12, DMCP_SPAWN_KIND_ITEM},
    {"MegaSphere", MT_MEGA, DMCP_SPAWN_KIND_ITEM},
    {"Mega Sphere", MT_MEGA, DMCP_SPAWN_KIND_ITEM},
    {"GreenArmor", MT_MISC0, DMCP_SPAWN_KIND_ITEM},
    {"Green Armor", MT_MISC0, DMCP_SPAWN_KIND_ITEM},
    {"BlueArmor", MT_MISC1, DMCP_SPAWN_KIND_ITEM},
    {"Blue Armor", MT_MISC1, DMCP_SPAWN_KIND_ITEM},
    {"MegaArmor", MT_MISC1, DMCP_SPAWN_KIND_ITEM},
    {"Backpack", MT_MISC24, DMCP_SPAWN_KIND_ITEM},
    {"Invulnerability", MT_INV, DMCP_SPAWN_KIND_ITEM},
    {"Berserk", MT_MISC13, DMCP_SPAWN_KIND_ITEM},
    {"Invisibility", MT_INS, DMCP_SPAWN_KIND_ITEM},
    {"RadiationSuit", MT_MISC14, DMCP_SPAWN_KIND_ITEM},
    {"Radiation Suit", MT_MISC14, DMCP_SPAWN_KIND_ITEM},
    {"ComputerMap", MT_MISC15, DMCP_SPAWN_KIND_ITEM},
    {"Computer Map", MT_MISC15, DMCP_SPAWN_KIND_ITEM},
    {"LightAmp", MT_MISC16, DMCP_SPAWN_KIND_ITEM},
    {"Light Amp", MT_MISC16, DMCP_SPAWN_KIND_ITEM},
};

static bool dmcp_resolve_spawn_type(const char* entity_class, mobjtype_t* out_type,
                                    dmcp_spawn_kind_t* out_kind) {
  size_t i;

  if (!entity_class || !entity_class[0] || !out_type || !out_kind) {
    return false;
  }

  for (i = 0; i < sizeof(k_spawn_mappings) / sizeof(k_spawn_mappings[0]); ++i) {
    if (dmcp_str_equals_ci(entity_class, k_spawn_mappings[i].name)) {
      *out_type = k_spawn_mappings[i].type;
      *out_kind = k_spawn_mappings[i].kind;
      return true;
    }
  }

  return false;
}

static bool dmcp_has_required_sound_lump(int sound_id) {
  char lump_name[11];

  if (sound_id <= 0 || sound_id >= NUMSFX) {
    return true;
  }

  if (!S_sfx[sound_id].name[0]) {
    return true;
  }

  snprintf(lump_name, sizeof(lump_name), "DS%.8s", S_sfx[sound_id].name);
  return W_CheckNumForName(lump_name) >= 0;
}

static bool dmcp_spawn_assets_available(mobjtype_t type) {
  const mobjinfo_t*  info;
  const state_t*     spawn_state;
  const spritedef_t* sprite;
  int                frame_index;
  int                rot;

  if (type < 0 || type >= NUMMOBJTYPES) {
    return false;
  }

  info = &mobjinfo[type];
  if (info->spawnstate < 0 || info->spawnstate >= NUMSTATES) {
    return false;
  }

  spawn_state = &states[info->spawnstate];
  if (spawn_state->sprite < 0 || spawn_state->sprite >= numsprites || !sprites) {
    return false;
  }

  sprite = &sprites[spawn_state->sprite];
  if (!sprite->spriteframes || sprite->numframes <= 0) {
    return false;
  }

  frame_index = spawn_state->frame & 0x7fff;
  if (frame_index < 0 || frame_index >= sprite->numframes) {
    return false;
  }

  for (rot = 0; rot < 8; ++rot) {
    if (sprite->spriteframes[frame_index].lump[rot] >= 0) {
      break;
    }
  }

  if (rot == 8) {
    return false;
  }

  if (!dmcp_has_required_sound_lump(info->seesound) ||
      !dmcp_has_required_sound_lump(info->activesound) ||
      !dmcp_has_required_sound_lump(info->painsound) ||
      !dmcp_has_required_sound_lump(info->deathsound)) {
    return false;
  }

  return true;
}

static bool dmcp_spawn_position_in_bounds(mobjtype_t type, fixed_t x, fixed_t y) {
  int     radius;
  int64_t min_x;
  int64_t min_y;
  int64_t max_x;
  int64_t max_y;
  int64_t left;
  int64_t right;
  int64_t bottom;
  int64_t top;

  if (type < 0 || type >= NUMMOBJTYPES) {
    return false;
  }

  // Blockmap bounds are authoritative map-space limits for collision queries.
  if (bmapwidth <= 0 || bmapheight <= 0) {
    return true;
  }

  radius = mobjinfo[type].radius;
  min_x  = (int64_t)bmaporgx;
  min_y  = (int64_t)bmaporgy;
  max_x  = min_x + (int64_t)bmapwidth * (int64_t)MAPBLOCKSIZE;
  max_y  = min_y + (int64_t)bmapheight * (int64_t)MAPBLOCKSIZE;

  left   = (int64_t)x - (int64_t)radius;
  right  = (int64_t)x + (int64_t)radius;
  bottom = (int64_t)y - (int64_t)radius;
  top    = (int64_t)y + (int64_t)radius;

  if (left < min_x || right >= max_x || bottom < min_y || top >= max_y) {
    return false;
  }

  return true;
}

static bool dmcp_give_item(player_t* player, const dmcp_cmd_give_item_t* give) {
  int32_t amount;

  if (!player || !player->mo || !give || !give->item_class[0]) {
    return false;
  }

  if (!dmcp_is_item_available(give->item_class, dmcp_to_gamemode(gamemode))) {
    return false;
  }

  amount = dmcp_clamp_int((int)give->amount, 1, 1000);

  if (dmcp_str_equals_ci(give->item_class, "Pistol")) {
    player->weaponowned[wp_pistol] = 1;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Shotgun")) {
    player->weaponowned[wp_shotgun] = 1;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Chaingun")) {
    player->weaponowned[wp_chaingun] = 1;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "RocketLauncher") ||
      dmcp_str_equals_ci(give->item_class, "Rocket Launcher")) {
    player->weaponowned[wp_missile] = 1;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "PlasmaRifle") ||
      dmcp_str_equals_ci(give->item_class, "Plasma Rifle")) {
    player->weaponowned[wp_plasma] = 1;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "BFG9000") ||
      dmcp_str_equals_ci(give->item_class, "BFG")) {
    player->weaponowned[wp_bfg] = 1;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Chainsaw")) {
    player->weaponowned[wp_chainsaw] = 1;
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "SuperShotgun") ||
      dmcp_str_equals_ci(give->item_class, "Super Shotgun")) {
    player->weaponowned[wp_supershotgun] = 1;
    return true;
  }

  if (dmcp_str_equals_ci(give->item_class, "Clip") ||
      dmcp_str_equals_ci(give->item_class, "Bullets")) {
    player->ammo[am_clip] += 10 * amount;
    player->ammo[am_clip] = dmcp_clamp_int(player->ammo[am_clip], 0, player->maxammo[am_clip]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "BoxOfBullets") ||
      dmcp_str_equals_ci(give->item_class, "Box of Bullets")) {
    player->ammo[am_clip] += 50 * amount;
    player->ammo[am_clip] = dmcp_clamp_int(player->ammo[am_clip], 0, player->maxammo[am_clip]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Shell") ||
      dmcp_str_equals_ci(give->item_class, "Shells")) {
    player->ammo[am_shell] += 4 * amount;
    player->ammo[am_shell] = dmcp_clamp_int(player->ammo[am_shell], 0, player->maxammo[am_shell]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "BoxOfShells") ||
      dmcp_str_equals_ci(give->item_class, "Box of Shells")) {
    player->ammo[am_shell] += 20 * amount;
    player->ammo[am_shell] = dmcp_clamp_int(player->ammo[am_shell], 0, player->maxammo[am_shell]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Rocket") ||
      dmcp_str_equals_ci(give->item_class, "Rockets") ||
      dmcp_str_equals_ci(give->item_class, "RocketAmmo") ||
      dmcp_str_equals_ci(give->item_class, "Rocket Ammo")) {
    player->ammo[am_misl] += amount;
    player->ammo[am_misl] = dmcp_clamp_int(player->ammo[am_misl], 0, player->maxammo[am_misl]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "BoxOfRockets") ||
      dmcp_str_equals_ci(give->item_class, "Box of Rockets")) {
    player->ammo[am_misl] += 5 * amount;
    player->ammo[am_misl] = dmcp_clamp_int(player->ammo[am_misl], 0, player->maxammo[am_misl]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "Cell") ||
      dmcp_str_equals_ci(give->item_class, "Cells")) {
    player->ammo[am_cell] += 20 * amount;
    player->ammo[am_cell] = dmcp_clamp_int(player->ammo[am_cell], 0, player->maxammo[am_cell]);
    return true;
  }
  if (dmcp_str_equals_ci(give->item_class, "CellPack") ||
      dmcp_str_equals_ci(give->item_class, "Cell Pack")) {
    player->ammo[am_cell] += 100 * amount;
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

    if ((mobj->flags & MF_COUNTKILL) == 0) {
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
  mobjtype_t        type;
  dmcp_spawn_kind_t kind;
  mobj_t*           spawned;
  fixed_t           x;
  fixed_t           y;

  if (!spawn || gamestate != GS_LEVEL) {
    return false;
  }
  if (!dmcp_resolve_spawn_type(spawn->entity_class, &type, &kind)) {
    return false;
  }

  if (kind == DMCP_SPAWN_KIND_ENEMY) {
    if (!dmcp_is_enemy_spawnable(spawn->entity_class, dmcp_to_gamemode(gamemode))) {
      return false;
    }
  } else {
    if (!dmcp_is_item_available(spawn->entity_class, dmcp_to_gamemode(gamemode))) {
      return false;
    }
  }

  if (!dmcp_spawn_assets_available(type)) {
    return false;
  }

  if (!dmcp_float_to_fixed_checked(spawn->position.x, &x) ||
      !dmcp_float_to_fixed_checked(spawn->position.y, &y)) {
    return false;
  }

  if (!dmcp_spawn_position_in_bounds(type, x, y)) {
    return false;
  }

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

    out[write_index++] = (char)tolower(ch);
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

bool dmcp_crispy_command_execute(dmcp_crispy_t* ctx, const dmcp_command_t* cmd) {
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
      // Doom simulation timing is fixed at 35 Hz in Crispy Doom.
      return false;

    case DMCP_CMD_DAMAGE_ENTITY:
      return dmcp_execute_damage_entity(player, &cmd->data.damage);

    case DMCP_CMD_KILL_ENTITY:
      return dmcp_execute_kill_entity(player, &cmd->data.kill);

    default:
      return false;
  }
}
