#include "adapter.h"

#include <cstring>
#include <memory>
#include <vector>

#include "dmcp/common.hpp"
#include "dmcp/dmcp.h"
#include "dmcp/schema.hpp"

#include "common/utility/name.h"
#include "doomstat.h"
#include "g_levellocals.h"
#include "gamedata/a_weapons.h"
#include "gamedata/gametype.h"
#include "gamedata/gi.h"
#include "playsim/d_player.h"
#include "playsim/dthinker.h"
#include "playsim/p_local.h"

namespace {

struct AdapterContext {
  dmcp_context_t*     dmcp_ctx{nullptr};
  dmcp_zdoom_config_t user_cfg{};
  dmcp_config_t       dmcp_cfg{};
};

// Engine logging fallback; declared here to avoid pulling engine headers into dmcp.
extern "C" int Printf(int printlevel, const char* format, ...);

player_t& ConsolePlayer() { return players[consoleplayer]; }

const char* ItemLabel(AActor* item) {
  if (!item) return "";
  const char* label = item->GetTag();
  if (!label || label[0] == '\0') {
    label = item->GetClass()->TypeName.GetChars();
  }
  return label ? label : "";
}

// Game-specific inventory filtering
static bool ShouldFilterInventoryItem(AActor* item) {
  if (!item) return true;

  const char* className = item->GetClass()->TypeName.GetChars();

  if (strcmp(className, "BasicArmor") == 0) {
    return true;
  }

  if (strcmp(className, "HexenArmor") == 0) {
    return !(gameinfo.gametype & GAME_Hexen);
  }

  return false;
}

inline static void BindDefaults(dmcp::Snapshot& snapshot) {
  snapshot.level.tic = 0;
  snapshot.player.hp = 100.f;
}

inline static void BindPlayerHealth(dmcp::Snapshot& snapshot) {
  player_t& player   = ConsolePlayer();
  AActor*   pawn     = player.mo;
  snapshot.player.hp = pawn ? pawn->health : 0;
}

inline static void BindPlayerArmor(dmcp::Snapshot& snapshot) {
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (!pawn) {
    snapshot.player.armor = 0;
    return;
  }
  AActor* armor = pawn->FindInventory(NAME_BasicArmor, true);
  snapshot.player.armor =
      armor ? static_cast<float>(armor->IntVar(NAME_Amount)) : 0.f;
}

inline static void BindPlayerPosition(dmcp::Snapshot& snapshot) {
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (pawn) {
    snapshot.player.position.x = static_cast<float>(pawn->Pos().X);
    snapshot.player.position.y = static_cast<float>(pawn->Pos().Y);
  } else {
    snapshot.player.position.x = snapshot.player.position.y = 0.f;
  }
}

inline static void BindPlayerAmmo(dmcp::Snapshot& snapshot) {
  player_t& player = ConsolePlayer();
  if (player.ReadyWeapon == nullptr) {
    snapshot.player.ammo = 0;
    return;
  }
  auto* ammo           = player.ReadyWeapon->PointerVar<AActor>(NAME_Ammo1);
  snapshot.player.ammo = ammo ? ammo->IntVar(NAME_Amount) : 0;
}

inline static void BindLevel(dmcp::Snapshot& snapshot) {
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (pawn && pawn->Level) {
    snapshot.level.tic = pawn->Level->time;
    dmcp::CopyString(pawn->Level->MapName.GetChars(), snapshot.level.id);
    if (pawn->Level->LevelName.IsNotEmpty()) {
      dmcp::CopyString(pawn->Level->LevelName.GetChars(), snapshot.level.name);
    } else {
      dmcp::CopyString(pawn->Level->MapName.GetChars(), snapshot.level.name);
    }
  } else {
    snapshot.level.tic     = 0;
    snapshot.level.id[0]   = '\0';
    snapshot.level.name[0] = '\0';
  }
  snapshot.level.kill_count   = player.killcount;
  snapshot.level.item_count   = player.itemcount;
  snapshot.level.secret_count = player.secretcount;
}

inline static void BindEnemies(dmcp::Snapshot& snapshot) {
  snapshot.enemies.clear();
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (!pawn || !pawn->Level) {
    return;
  }
  TThinkerIterator<AActor> it(pawn->Level);
  while (auto* actor = it.Next()) {
    if (!(actor->flags & MF_COUNTKILL) || actor == pawn || actor->health <= 0) {
      continue;
    }
    if (snapshot.enemies.size() >= snapshot.enemies.capacity()) {
      break;
    }
    auto& entry = snapshot.enemies.emplace_back();
    entry.id    = static_cast<int32_t>(actor->tid);
    entry.hp    = static_cast<float>(actor->health);

    int maxHealth = actor->SpawnHealth();
    if (maxHealth <= 0) {
      maxHealth = actor->GetMaxHealth();
    }
    if (maxHealth <= 0) {
      maxHealth = 100;
    }
    entry.max_hp = static_cast<float>(maxHealth);

    entry.position.x = static_cast<float>(actor->Pos().X);
    entry.position.y = static_cast<float>(actor->Pos().Y);
    dmcp::CopyString(actor->GetClass()->TypeName.GetChars(), entry.type);
  }
}

inline static void BindInventory(dmcp::Snapshot& snapshot) {
  snapshot.player.inventory.clear();
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (!pawn) {
    return;
  }
  for (AActor* item = pawn->Inventory; item != nullptr;
       item         = item->Inventory) {
    if (ShouldFilterInventoryItem(item)) {
      continue;
    }

    int amount = item->IntVar(NAME_Amount);
    if (amount <= 0) {
      continue;
    }
    auto& dest = snapshot.player.inventory.emplace_back();
    dmcp::CopyString(ItemLabel(item), dest.name);
    dest.amount = amount;
  }
}

static void PopulateSnapshot(dmcp::Snapshot& snapshot) {
  BindDefaults(snapshot);
  BindPlayerHealth(snapshot);
  BindPlayerArmor(snapshot);
  BindPlayerPosition(snapshot);
  BindPlayerAmmo(snapshot);
  BindLevel(snapshot);
  BindEnemies(snapshot);
  BindInventory(snapshot);
}

static void ZdoomTick(void* /*user_data*/, void* snapshot_ptr) {
  if (!snapshot_ptr) return;
  auto* snapshot = static_cast<dmcp::Snapshot*>(snapshot_ptr);
  PopulateSnapshot(*snapshot);
}

static void ZdoomLog(void* user_data, int level, const char* message) {
  auto* ctx = static_cast<AdapterContext*>(user_data);
  if (!ctx || !message) return;

  if (ctx->user_cfg.log_fn) {
    ctx->user_cfg.log_fn(ctx->user_cfg.log_user, level, message);
    return;
  }
  int printLevel = (level >= DMCP_LOG_WARN) ? PRINT_HIGH : PRINT_LOG;
  Printf(printLevel, "[DMCP] %s\n", message);
}

}  // namespace

extern "C" dmcp_zdoom_t* dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg) {
  auto* ctx     = new AdapterContext();
  ctx->user_cfg = cfg ? *cfg : dmcp_zdoom_config_t{};
  ctx->dmcp_cfg = dmcp_default_config();

  if (ctx->user_cfg.dmcp_config) {
    ctx->dmcp_cfg = *ctx->user_cfg.dmcp_config;
  }

  ctx->dmcp_cfg.on_tick   = ZdoomTick;
  ctx->dmcp_cfg.on_log    = ZdoomLog;
  ctx->dmcp_cfg.user_data = ctx;

  ctx->dmcp_ctx = dmcp_create(&ctx->dmcp_cfg);
  if (!ctx->dmcp_ctx) {
    delete ctx;
    return nullptr;
  }
  return reinterpret_cast<dmcp_zdoom_t*>(ctx);
}

extern "C" void dmcp_zdoom_destroy(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx) return;
  if (ctx->dmcp_ctx) {
    dmcp_destroy(ctx->dmcp_ctx);
    ctx->dmcp_ctx = nullptr;
  }
  delete ctx;
}

extern "C" int dmcp_zdoom_tick(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx) return DMCP_ERROR_INVALID_ARGS;

  if (ctx->user_cfg.should_tick_fn &&
      !ctx->user_cfg.should_tick_fn(ctx->user_cfg.should_tick_user)) {
    return DMCP_OK;
  }

  dmcp_update(ctx->dmcp_ctx);
  return DMCP_OK;
}

extern "C" bool dmcp_zdoom_is_running(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx) return false;
  return dmcp_is_running(ctx->dmcp_ctx);
}

extern "C" void dmcp_zdoom_get_stats(dmcp_zdoom_t* ctx_handle,
                                     dmcp_stats_t* out_stats) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx || !out_stats) return;
  dmcp_get_stats(ctx->dmcp_ctx, out_stats);
}
