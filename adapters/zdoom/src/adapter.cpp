#include "dmcp_zdoom.h"
#include "internal.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

// ZDoom headers - these come from the engine build
#include "common/engine/printf.h"
#include "common/utility/name.h"
#include "doomstat.h"
#include "g_levellocals.h"
#include "gamedata/a_weapons.h"
#include "gamedata/gametype.h"
#include "gamedata/gi.h"
#include "playsim/d_player.h"
#include "playsim/dthinker.h"
#include "playsim/p_local.h"

using namespace dmcp::zdoom;

namespace {

struct PlayerView {
  player_t*     player;
  AActor*       pawn;
  FLevelLocals* level;

  bool HasPawn() const { return pawn != nullptr; }
  bool HasLevel() const { return level != nullptr; }
};

}  // namespace

// ============================================================================
// ZDoom State Extraction
// ============================================================================

static PlayerView MakePlayerView() {
  PlayerView view = {};
  view.player     = &players[consoleplayer];
  view.pawn       = view.player ? view.player->mo : nullptr;
  view.level      = (view.pawn && view.pawn->Level) ? view.pawn->Level : nullptr;
  return view;
}

static uint16_t EnvPortOverride() {
  const char* env_port = std::getenv("DMCP_PORT");
  if (env_port) {
    int port = std::atoi(env_port);
    if (port > 0 && port < 65536) {
      return static_cast<uint16_t>(port);
    }
  }
  return 0;
}

static dmcp_zdoom_config_t CopyConfig(const dmcp_zdoom_config_t* config) {
  dmcp_zdoom_config_t effective = dmcp_zdoom_config_default();
  if (!config) {
    return effective;
  }

  size_t copy_size = config->struct_size;
  if (copy_size == 0 || copy_size > sizeof(dmcp_zdoom_config_t)) {
    copy_size = sizeof(dmcp_zdoom_config_t);
  }
  std::memcpy(&effective, config, copy_size);
  effective.struct_size = sizeof(dmcp_zdoom_config_t);
  return effective;
}

static const char* ItemLabel(AActor* item) {
  if (!item) return "";

  const char* label = item->GetTag();
  if (!label || label[0] == '\0') {
    label = item->GetClass()->TypeName.GetChars();
  }
  return label ? label : "";
}

static bool ShouldFilterInventoryItem(AActor* item) {
  if (!item) return true;

  const char* className = item->GetClass()->TypeName.GetChars();

  if (std::strcmp(className, "BasicArmor") == 0) {
    return true;
  }

  if (std::strcmp(className, "HexenArmor") == 0) {
    return !(gameinfo.gametype & GAME_Hexen);
  }

  return false;
}

// ============================================================================
// Snapshot Population
// ============================================================================

static void BindDefaults(dmcp_snapshot_t* snapshot) {
  snapshot->level.tic         = 0;
  snapshot->player.hp         = DMCP_PLAYER_INITIAL_HEALTH;
  snapshot->player.armor      = 0;
  snapshot->player.position.x = 0.f;
  snapshot->player.position.y = 0.f;
  for (int i = 0; i < DMCP_MAX_AMMO_TYPES; ++i) {
    snapshot->player.ammo[i]    = 0;
    snapshot->player.maxammo[i] = 0;
  }
}

static void BindPlayerHealth(dmcp_snapshot_t* snapshot, const PlayerView& view) {
  snapshot->player.hp = view.HasPawn() ? view.pawn->health : 0;
}

static void BindPlayerArmor(dmcp_snapshot_t* snapshot, const PlayerView& view) {
  if (!view.HasPawn()) {
    snapshot->player.armor = 0;
    return;
  }

  AActor* armor          = view.pawn->FindInventory(NAME_BasicArmor, true);
  snapshot->player.armor = armor ? armor->IntVar(NAME_Amount) : 0;
}

static void BindPlayerPosition(dmcp_snapshot_t* snapshot, const PlayerView& view) {
  if (!view.HasPawn()) return;

  snapshot->player.position.x = static_cast<float>(view.pawn->Pos().X);
  snapshot->player.position.y = static_cast<float>(view.pawn->Pos().Y);
}

static void BindPlayerAmmo(dmcp_snapshot_t* snapshot, const PlayerView& view) {
  if (!view.player || view.player->ReadyWeapon == nullptr) {
    return;
  }

  auto* ammo = view.player->ReadyWeapon->PointerVar<AActor>(NAME_Ammo1);
  // ZDoom exposes ammo as inventory classes. Until this adapter maps each class
  // to Doom's four canonical pools, report the ready weapon's primary ammo in
  // slot 0 and leave the other pools unknown/zero.
  snapshot->player.ammo[0] = ammo ? ammo->IntVar(NAME_Amount) : 0;
}

static void BindLevel(dmcp_snapshot_t* snapshot, const PlayerView& view) {
  if (view.HasLevel()) {
    snapshot->level.tic = view.level->time;
    dmcp_strcpy(snapshot->level.level_id, view.level->MapName.GetChars(),
                sizeof(snapshot->level.level_id));

    if (view.level->LevelName.IsNotEmpty()) {
      dmcp_strcpy(snapshot->level.level_name, view.level->LevelName.GetChars(),
                  sizeof(snapshot->level.level_name));
    } else {
      dmcp_strcpy(snapshot->level.level_name, view.level->MapName.GetChars(),
                  sizeof(snapshot->level.level_name));
    }
  } else {
    snapshot->level.tic           = 0;
    snapshot->level.level_id[0]   = '\0';
    snapshot->level.level_name[0] = '\0';
  }

  snapshot->level.kill_count   = view.player ? view.player->killcount : 0;
  snapshot->level.item_count   = view.player ? view.player->itemcount : 0;
  snapshot->level.secret_count = view.player ? view.player->secretcount : 0;
}

static void BindEnemies(dmcp_snapshot_t* snapshot, const PlayerView& view) {
  if (!view.HasLevel() || !view.HasPawn()) return;

  TThinkerIterator<AActor> it(view.level);
  while (auto* actor = it.Next()) {
    if (!(actor->flags & MF_COUNTKILL) || actor == view.pawn || actor->health <= 0) {
      continue;
    }

    if (snapshot->enemy_count >= DMCP_MAX_ENEMIES) break;

    dmcp_enemy_t enemy = {};
    enemy.id           = static_cast<int32_t>(actor->tid);
    enemy.hp           = actor->health;

    int maxHealth = actor->SpawnHealth();
    if (maxHealth <= 0) {
      maxHealth = actor->GetMaxHealth();
    }
    if (maxHealth <= 0) {
      maxHealth = DMCP_PLAYER_INITIAL_HEALTH;
    }
    enemy.max_hp = maxHealth;

    enemy.position.x = static_cast<float>(actor->Pos().X);
    enemy.position.y = static_cast<float>(actor->Pos().Y);
    dmcp_strcpy(enemy.type, actor->GetClass()->TypeName.GetChars(), sizeof(enemy.type));

    dmcp_snapshot_add_enemy(snapshot, &enemy);
  }
}

static void BindInventory(dmcp_snapshot_t* snapshot, const PlayerView& view) {
  if (!view.HasPawn()) return;

  for (AActor* item = view.pawn->Inventory; item != nullptr; item = item->Inventory) {
    if (ShouldFilterInventoryItem(item)) continue;

    int amount = item->IntVar(NAME_Amount);
    if (amount <= 0) continue;

    dmcp_item_t dest = {};
    dmcp_strcpy(dest.name, ItemLabel(item), sizeof(dest.name));
    dest.amount = amount;

    dmcp_snapshot_add_item(snapshot, &dest);
  }
}

static void PopulateSnapshot(dmcp_snapshot_t* snapshot) {
  dmcp_snapshot_clear(snapshot);
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

// ============================================================================
// Callbacks
// ============================================================================

static void ZdoomSnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
  if (!snapshot) return;
  PopulateSnapshot(snapshot);
}

static void ZdoomLogCallback(void* user_data, int level, const char* message) {
  auto* ctx = static_cast<AdapterContext*>(user_data);
  if (!ctx || !message) return;

  if (ctx->user_cfg.log_fn) {
    ctx->user_cfg.log_fn(ctx->user_cfg.log_user, level, message);
    return;
  }

  int printLevel = (level >= MCP_LOG_WARN) ? PRINT_HIGH : PRINT_LOG;
  Printf(printLevel, "[DMCP] %s\n", message);
}

}  // namespace

namespace dmcp::zdoom {

void Log(AdapterContext* ctx, int level, const char* fmt, ...) {
  if (!fmt) return;

  char    buf[kLogBufferSize];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  ZdoomLogCallback(ctx, level, buf);
}

}  // namespace dmcp::zdoom

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

dmcp_zdoom_t* dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg) {
  auto ctx                     = std::make_unique<AdapterContext>();
  ctx->user_cfg                = CopyConfig(cfg);
  ctx->dmcp_cfg                = ctx->user_cfg.base;
  ctx->log_not_running_emitted = false;

  // Apply user config
  if (uint16_t env_port = EnvPortOverride()) {
    ctx->dmcp_cfg.port = env_port;
  }

  // Setup callbacks
  ctx->dmcp_cfg.on_snapshot = ZdoomSnapshotCallback;
  ctx->dmcp_cfg.on_log      = ZdoomLogCallback;
  ctx->dmcp_cfg.user_data   = ctx.get();

  // Create DMCP context
  std::unique_ptr<dmcp_context_t, decltype(&dmcp_context_destroy)> dmcp_ctx(
      dmcp_context_create(&ctx->dmcp_cfg), dmcp_context_destroy);
  if (!dmcp_ctx) {
    Log(ctx.get(), MCP_LOG_ERROR, "Failed to start DMCP server on port %u", ctx->dmcp_cfg.port);
    return nullptr;
  }

  ctx->dmcp_ctx = dmcp_ctx.release();
  Log(ctx.get(), MCP_LOG_INFO, "DMCP server started on port %u (target %u Hz)", ctx->dmcp_cfg.port,
      ctx->dmcp_cfg.target_hz);

  return reinterpret_cast<dmcp_zdoom_t*>(ctx.release());
}

void dmcp_zdoom_destroy(dmcp_zdoom_t* ctx_handle) {
  std::unique_ptr<AdapterContext> ctx(reinterpret_cast<AdapterContext*>(ctx_handle));
  if (!ctx) return;

  if (ctx->dmcp_ctx) {
    dmcp_context_destroy(ctx->dmcp_ctx);
    ctx->dmcp_ctx = nullptr;
  }

  Log(ctx.get(), MCP_LOG_INFO, "DMCP server stopped");
}

mcp_status_t dmcp_zdoom_tick(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx) {
    mcp_status_t result = {};
    result.code         = MCP_STATUS_CODE_INVALID_ARGS;
    return result;
  }

  // Check should_tick gate
  if (ctx->user_cfg.should_tick_fn &&
      !ctx->user_cfg.should_tick_fn(ctx->user_cfg.should_tick_user)) {
    return MCP_STATUS_OK("Success");
  }

  // Check server health
  if (!dmcp_context_is_running(ctx->dmcp_ctx)) {
    if (!ctx->log_not_running_emitted) {
      Log(ctx, MCP_LOG_WARN, "DMCP server not running; skipping tick");
      ctx->log_not_running_emitted = true;
    }
    mcp_status_t result = {};
    result.code         = MCP_STATUS_CODE_DISABLED;
    return result;
  }
  ctx->log_not_running_emitted = false;

  // Process tick
  dmcp_context_tick(ctx->dmcp_ctx);

  return MCP_STATUS_OK("Success");
}

bool dmcp_zdoom_is_running(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx) return false;
  return dmcp_context_is_running(ctx->dmcp_ctx);
}

void dmcp_zdoom_get_stats(dmcp_zdoom_t* ctx_handle, dmcp_stats_t* out_stats) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx || !ctx->dmcp_ctx || !out_stats) return;

  dmcp_stats_get(ctx->dmcp_ctx, out_stats);
}

dmcp_context_t* dmcp_zdoom_get_context(dmcp_zdoom_t* ctx_handle) {
  auto* ctx = reinterpret_cast<AdapterContext*>(ctx_handle);
  if (!ctx) {
    return nullptr;
  }
  return ctx->dmcp_ctx;
}

}  // extern "C"
