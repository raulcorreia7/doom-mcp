#include "adapter.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include "common/engine/printf.h"
#include "common/utility/name.h"
#include "dmcp/common.hpp"
#include "dmcp/dmcp.h"
#include "dmcp/schema.hpp"
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
  bool                log_not_running_emitted{false};
};

constexpr size_t kLogBufferSize = 256;

struct PlayerView {
  player_t*     player{nullptr};
  AActor*       pawn{nullptr};
  FLevelLocals*  level{nullptr};
  bool          HasPawn() const { return pawn != nullptr; }
  bool          HasLevel() const { return level != nullptr; }
};

static PlayerView MakePlayerView() {
  PlayerView view{};
  view.player = &players[consoleplayer];
  view.pawn   = view.player ? view.player->mo : nullptr;
  view.level  = (view.pawn && view.pawn->Level) ? view.pawn->Level : nullptr;
  return view;
}

static uint16_t EnvPortOverride() {
  if (const char* env_port = std::getenv("DMCP_PORT")) {
    const int port = std::atoi(env_port);
    if (port > 0 && port < 65536) {
      return static_cast<uint16_t>(port);
    }
  }
  return 0;
}

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

static void BindDefaults(dmcp::Snapshot& snapshot) {
  snapshot.level.tic   = 0;
  snapshot.player.hp   = 100.f;
  snapshot.player.armor = 0.f;
  snapshot.player.ammo  = 0;
  snapshot.player.position.x = 0.f;
  snapshot.player.position.y = 0.f;
}

static void BindPlayerHealth(dmcp::Snapshot& snapshot,
                             const PlayerView& view) {
  snapshot.player.hp = view.HasPawn() ? view.pawn->health : 0.f;
}

static void BindPlayerArmor(dmcp::Snapshot& snapshot,
                            const PlayerView& view) {
  if (!view.HasPawn()) {
    snapshot.player.armor = 0.f;
    return;
  }
  AActor* armor = view.pawn->FindInventory(NAME_BasicArmor, true);
  snapshot.player.armor =
      armor ? static_cast<float>(armor->IntVar(NAME_Amount)) : 0.f;
}

static void BindPlayerPosition(dmcp::Snapshot& snapshot,
                               const PlayerView& view) {
  if (!view.HasPawn()) {
    return;
  }
  snapshot.player.position.x = static_cast<float>(view.pawn->Pos().X);
  snapshot.player.position.y = static_cast<float>(view.pawn->Pos().Y);
}

static void BindPlayerAmmo(dmcp::Snapshot& snapshot,
                           const PlayerView& view) {
  if (!view.player || view.player->ReadyWeapon == nullptr) {
    snapshot.player.ammo = 0;
    return;
  }
  auto* ammo           = view.player->ReadyWeapon->PointerVar<AActor>(NAME_Ammo1);
  snapshot.player.ammo = ammo ? ammo->IntVar(NAME_Amount) : 0;
}

static void BindLevel(dmcp::Snapshot& snapshot, const PlayerView& view) {
  if (view.HasLevel()) {
    snapshot.level.tic = view.level->time;
    dmcp::CopyString(view.level->MapName.GetChars(), snapshot.level.id);
    if (view.level->LevelName.IsNotEmpty()) {
      dmcp::CopyString(view.level->LevelName.GetChars(), snapshot.level.name);
    } else {
      dmcp::CopyString(view.level->MapName.GetChars(), snapshot.level.name);
    }
  } else {
    snapshot.level.tic     = 0;
    snapshot.level.id[0]   = '\0';
    snapshot.level.name[0] = '\0';
  }
  snapshot.level.kill_count   = view.player ? view.player->killcount : 0;
  snapshot.level.item_count   = view.player ? view.player->itemcount : 0;
  snapshot.level.secret_count = view.player ? view.player->secretcount : 0;
}

static void BindEnemies(dmcp::Snapshot& snapshot, const PlayerView& view) {
  if (!view.HasLevel() || !view.HasPawn()) {
    return;
  }
  TThinkerIterator<AActor> it(view.level);
  while (auto* actor = it.Next()) {
    if (!(actor->flags & MF_COUNTKILL) || actor == view.pawn ||
        actor->health <= 0) {
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

static void BindInventory(dmcp::Snapshot& snapshot, const PlayerView& view) {
  if (!view.HasPawn()) {
    return;
  }
  for (AActor* item = view.pawn->Inventory; item != nullptr;
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
  snapshot.enemies.clear();
  snapshot.player.inventory.clear();
  BindDefaults(snapshot);
  const PlayerView view = MakePlayerView();
  BindPlayerHealth(snapshot, view);
  BindPlayerArmor(snapshot, view);
  BindPlayerPosition(snapshot, view);
  BindPlayerAmmo(snapshot, view);
  BindLevel(snapshot, view);
  BindEnemies(snapshot, view);
  BindInventory(snapshot, view);
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
  std::fprintf((level >= DMCP_LOG_ERROR) ? stderr : stdout, "[DMCP] %s\n",
               message);
}

static void Log(AdapterContext* ctx, int level, const char* fmt, ...) {
  if (!fmt) return;
  char    buf[kLogBufferSize];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  ZdoomLog(ctx, level, buf);
}

}  // namespace

extern "C" dmcp_zdoom_t* dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg) {
  auto* ctx     = new AdapterContext();
  ctx->user_cfg = cfg ? *cfg : dmcp_zdoom_config_t{};
  ctx->dmcp_cfg = dmcp_default_config();

  if (ctx->user_cfg.dmcp_config) {
    ctx->dmcp_cfg = *ctx->user_cfg.dmcp_config;
  } else if (uint16_t env_port = EnvPortOverride()) {
    ctx->dmcp_cfg.port = env_port;
  }

  ctx->dmcp_cfg.on_tick   = ZdoomTick;
  ctx->dmcp_cfg.on_log    = ZdoomLog;
  ctx->dmcp_cfg.user_data = ctx;

  ctx->dmcp_ctx = dmcp_create(&ctx->dmcp_cfg);
  if (!ctx->dmcp_ctx) {
    Log(ctx, DMCP_LOG_ERROR, "Failed to start DMCP server (port %u)",
        ctx->dmcp_cfg.port);
    delete ctx;
    return nullptr;
  }
  Log(ctx, DMCP_LOG_INFO, "DMCP server started on port %u (target %u Hz)",
      ctx->dmcp_cfg.port, ctx->dmcp_cfg.target_hz);
  return reinterpret_cast<dmcp_zdoom_t*>(ctx);
}

extern "C" void dmcp_zdoom_destroy(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx) return;
  if (ctx->dmcp_ctx) {
    dmcp_destroy(ctx->dmcp_ctx);
    ctx->dmcp_ctx = nullptr;
  }
  Log(ctx, DMCP_LOG_INFO, "DMCP server stopped");
  delete ctx;
}

extern "C" int dmcp_zdoom_tick(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx) return DMCP_ERROR_INVALID_ARGS;

  if (ctx->user_cfg.should_tick_fn &&
      !ctx->user_cfg.should_tick_fn(ctx->user_cfg.should_tick_user)) {
    return DMCP_OK;
  }

  if (!dmcp_is_running(ctx->dmcp_ctx)) {
    if (!ctx->log_not_running_emitted) {
      Log(ctx, DMCP_LOG_WARN, "DMCP server not running; skipping tick");
      ctx->log_not_running_emitted = true;
    }
    return DMCP_ERROR_DISABLED;
  }
  ctx->log_not_running_emitted = false;

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
