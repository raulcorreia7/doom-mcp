#include <cstdint>
#include <string>

#include "internal/serialization.hpp"
#include "yyjson.h"

namespace dmcp {

static yyjson_mut_val* vec3_to_json(yyjson_mut_doc* doc, const dmcp_vec3_t& v) {
  yyjson_mut_val* obj = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_real(doc, obj, "x", v.x);
  yyjson_mut_obj_add_real(doc, obj, "y", v.y);
  yyjson_mut_obj_add_real(doc, obj, "z", v.z);
  return obj;
}

static yyjson_mut_val* int_array_to_json(yyjson_mut_doc* doc, const int32_t* arr, size_t count) {
  yyjson_mut_val* arr_val = yyjson_mut_arr(doc);
  for (size_t i = 0; i < count; i++) {
    yyjson_mut_arr_append(arr_val, yyjson_mut_sint(doc, arr[i]));
  }
  return arr_val;
}

static yyjson_mut_val* player_to_json(yyjson_mut_doc* doc, const dmcp_player_t& p) {
  yyjson_mut_val* obj = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_sint(doc, obj, "hp", static_cast<int64_t>(p.hp));
  yyjson_mut_obj_add_real(doc, obj, "armor", p.armor);
  yyjson_mut_obj_add_str(doc, obj, "armortype", p.armortype);
  yyjson_mut_obj_add_val(doc, obj, "position", vec3_to_json(doc, p.position));
  yyjson_mut_obj_add_real(doc, obj, "angle", p.angle);
  yyjson_mut_obj_add_str(doc, obj, "readyweapon", p.readyweapon);
  yyjson_mut_obj_add_str(doc, obj, "pendingweapon", p.pendingweapon);
  yyjson_mut_obj_add_val(doc, obj, "weaponowned",
                         int_array_to_json(doc, p.weaponowned, DMCP_MAX_WEAPONS));
  yyjson_mut_obj_add_val(doc, obj, "ammo", int_array_to_json(doc, p.ammo, DMCP_MAX_AMMO_TYPES));
  yyjson_mut_obj_add_val(doc, obj, "maxammo",
                         int_array_to_json(doc, p.maxammo, DMCP_MAX_AMMO_TYPES));
  yyjson_mut_obj_add_bool(doc, obj, "backpack", p.backpack);
  yyjson_mut_obj_add_val(doc, obj, "powers", int_array_to_json(doc, p.powers, DMCP_MAX_POWERUPS));
  yyjson_mut_obj_add_val(doc, obj, "cards", int_array_to_json(doc, p.cards, DMCP_MAX_KEYS));
  yyjson_mut_obj_add_str(doc, obj, "playerstate", p.playerstate);
  yyjson_mut_obj_add_sint(doc, obj, "cheats", p.cheats);
  yyjson_mut_obj_add_sint(doc, obj, "damagecount", p.damagecount);
  return obj;
}

static yyjson_mut_val* level_to_json(yyjson_mut_doc* doc, const dmcp_level_t& l) {
  yyjson_mut_val* obj = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_sint(doc, obj, "tic", l.tic);
  yyjson_mut_obj_add_sint(doc, obj, "leveltime", l.leveltime);
  yyjson_mut_obj_add_str(doc, obj, "level_id", l.level_id);
  yyjson_mut_obj_add_str(doc, obj, "level_name", l.level_name);
  yyjson_mut_obj_add_sint(doc, obj, "kill_count", l.kill_count);
  yyjson_mut_obj_add_sint(doc, obj, "item_count", l.item_count);
  yyjson_mut_obj_add_sint(doc, obj, "secret_count", l.secret_count);
  yyjson_mut_obj_add_sint(doc, obj, "totalkills", l.totalkills);
  yyjson_mut_obj_add_sint(doc, obj, "totalitems", l.totalitems);
  yyjson_mut_obj_add_sint(doc, obj, "totalsecrets", l.totalsecrets);
  yyjson_mut_obj_add_str(doc, obj, "skill", l.skill);
  yyjson_mut_obj_add_str(doc, obj, "gamestate", l.gamestate);
  yyjson_mut_obj_add_bool(doc, obj, "paused", l.paused != 0);
  return obj;
}

static yyjson_mut_val* game_to_json(yyjson_mut_doc* doc, const dmcp_game_t& g) {
  yyjson_mut_val* obj = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_str(doc, obj, "mode", g.mode);
  yyjson_mut_obj_add_bool(doc, obj, "respawnmonsters", g.respawnmonsters);
  yyjson_mut_obj_add_sint(doc, obj, "consoleplayer", g.consoleplayer);
  return obj;
}

static yyjson_mut_val* enemy_to_json(yyjson_mut_doc* doc, const dmcp_enemy_t& e) {
  yyjson_mut_val* obj = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_sint(doc, obj, "id", e.id);
  yyjson_mut_obj_add_sint(doc, obj, "hp", static_cast<int64_t>(e.hp));
  yyjson_mut_obj_add_sint(doc, obj, "max_hp", static_cast<int64_t>(e.max_hp));
  yyjson_mut_obj_add_val(doc, obj, "position", vec3_to_json(doc, e.position));
  yyjson_mut_obj_add_real(doc, obj, "angle", e.angle);
  yyjson_mut_obj_add_sint(doc, obj, "target_id", e.target_id);
  yyjson_mut_obj_add_str(doc, obj, "type", e.type);
  return obj;
}

static yyjson_mut_val* item_to_json(yyjson_mut_doc* doc, const dmcp_item_t& i) {
  yyjson_mut_val* obj = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_str(doc, obj, "name", i.name);
  yyjson_mut_obj_add_sint(doc, obj, "amount", i.amount);
  return obj;
}

std::string snapshot_to_json(const dmcp_snapshot_t& snapshot) {
  yyjson_mut_doc* doc  = yyjson_mut_doc_new(nullptr);
  yyjson_mut_val* root = yyjson_mut_obj(doc);
  yyjson_mut_doc_set_root(doc, root);

  yyjson_mut_obj_add_val(doc, root, "player", player_to_json(doc, snapshot.player));
  yyjson_mut_obj_add_val(doc, root, "level", level_to_json(doc, snapshot.level));
  yyjson_mut_obj_add_val(doc, root, "game", game_to_json(doc, snapshot.game));

  yyjson_mut_val* enemies = yyjson_mut_arr(doc);
  for (std::uint32_t i = 0; i < snapshot.enemy_count; i++) {
    yyjson_mut_arr_append(enemies, enemy_to_json(doc, snapshot.enemies[i]));
  }
  yyjson_mut_obj_add_val(doc, root, "enemies", enemies);
  yyjson_mut_obj_add_uint(doc, root, "enemy_count", snapshot.enemy_count);

  yyjson_mut_val* inventory = yyjson_mut_arr(doc);
  for (std::uint32_t i = 0; i < snapshot.inventory_count; i++) {
    yyjson_mut_arr_append(inventory, item_to_json(doc, snapshot.inventory[i]));
  }
  yyjson_mut_obj_add_val(doc, root, "inventory", inventory);
  yyjson_mut_obj_add_uint(doc, root, "inventory_count", snapshot.inventory_count);

  char*       json = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY_TWO_SPACES, nullptr);
  std::string result(json ? json : "{}");
  if (json) free(json);
  yyjson_mut_doc_free(doc);

  return result;
}

}  // namespace dmcp
