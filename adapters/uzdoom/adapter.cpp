#include <cstring>
#include <memory>
#include <vector>

#include "dmcp/common.hpp"
#include "dmcp/dmcp.h"
#include "dmcp/schema.hpp"

#if defined(DMCP_WITH_UZDOOM)
#include "d_player.h"
#include "doomstat.h"
#include "dthinker.h"
#include "gamedata/a_weapons.h"
#include "name.h"
#include "p_local.h"
#endif

namespace {

static dmcp_context_t* g_ctx = nullptr;

#if defined(DMCP_WITH_UZDOOM)
player_t& ConsolePlayer() { return players[consoleplayer]; }

const char* ItemLabel(AActor* item) {
  if (!item) {
    return "";
  }
  const char* label = item->GetTag();
  if (!label || label[0] == '\0') {
    label = item->GetClass()->TypeName.GetChars();
  }
  return label ? label : "";
}
#endif

static void BindDefaults(dmcp::Snapshot& snapshot) {
  snapshot.level.tic     = 0;
  snapshot.player.health = 100.f;
}

static void BindPlayerHealth(dmcp::Snapshot& snapshot) {
#if defined(DMCP_WITH_UZDOOM)
  player_t& player       = ConsolePlayer();
  AActor*   pawn         = player.mo;
  snapshot.player.health = pawn ? pawn->health : 0;
#endif
}

static void BindPlayerArmor(dmcp::Snapshot& snapshot) {
#if defined(DMCP_WITH_UZDOOM)
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (!pawn) {
    snapshot.player.armor = 0;
    return;
  }
  AActor* armor = pawn->FindInventory(NAME_BasicArmor, true);
  snapshot.player.armor =
      armor ? static_cast<float>(armor->IntVar(NAME_Amount)) : 0.f;
#endif
}

static void BindPlayerPosition(dmcp::Snapshot& snapshot) {
#if defined(DMCP_WITH_UZDOOM)
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (pawn) {
    snapshot.player.position.x = static_cast<float>(pawn->Pos().X);
    snapshot.player.position.y = static_cast<float>(pawn->Pos().Y);
  } else {
    snapshot.player.position.x = snapshot.player.position.y = 0.f;
  }
#endif
}

static void BindPlayerAmmo(dmcp::Snapshot& snapshot) {
#if defined(DMCP_WITH_UZDOOM)
  player_t& player = ConsolePlayer();
  if (player.ReadyWeapon == nullptr) {
    snapshot.player.ammo = 0;
    return;
  }
  auto* ammo           = player.ReadyWeapon->PointerVar<AActor>(NAME_Ammo1);
  snapshot.player.ammo = ammo ? ammo->IntVar(NAME_Amount) : 0;
#endif
}

static void BindLevel(dmcp::Snapshot& snapshot) {
#if defined(DMCP_WITH_UZDOOM)
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (pawn && pawn->Level) {
    snapshot.level.tic = pawn->Level->time;
    dmcp::CopyString(pawn->Level->MapName.GetChars(), snapshot.level.name);
  } else {
    snapshot.level.tic     = 0;
    snapshot.level.name[0] = '\0';
  }
  snapshot.level.kill_count   = player.killcount;
  snapshot.level.item_count   = player.itemcount;
  snapshot.level.secret_count = player.secretcount;
#endif
}

static void BindEnemies(dmcp::Snapshot& snapshot) {
#if defined(DMCP_WITH_UZDOOM)
  snapshot.enemies.clear();
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (!pawn || !pawn->Level) {
    return;
  }
  TThinkerIterator<AActor> it(pawn->Level);
  while (auto* actor = it.Next()) {
    if (!(actor->flags & MF_COUNTKILL) || actor == pawn ||
        actor->health <= 0) {
      continue;
    }
    if (snapshot.enemies.size() >= snapshot.enemies.capacity()) {
      break;
    }
    auto& entry      = snapshot.enemies.emplace_back();
    entry.id         = static_cast<int32_t>(actor->tid);
    entry.health     = static_cast<float>(actor->health);
    entry.position.x = static_cast<float>(actor->Pos().X);
    entry.position.y = static_cast<float>(actor->Pos().Y);
    dmcp::CopyString(actor->GetClass()->TypeName.GetChars(), entry.type);
  }
#endif
}

static void BindInventory(dmcp::Snapshot& snapshot) {
#if defined(DMCP_WITH_UZDOOM)
  snapshot.player.inventory.clear();
  player_t& player = ConsolePlayer();
  AActor*   pawn   = player.mo;
  if (!pawn) {
    return;
  }
  for (AActor* item = pawn->Inventory; item != nullptr;
       item         = item->Inventory) {
    int amount = item->IntVar(NAME_Amount);
    if (amount <= 0) {
      continue;
    }
    auto& dest = snapshot.player.inventory.emplace_back();
    dmcp::CopyString(ItemLabel(item), dest.name);
    dest.amount = amount;
  }
#endif
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

static void AdapterTick(void* /*user_data*/, void* snapshot_ptr) {
  if (!snapshot_ptr) return;
  auto* snapshot = static_cast<dmcp::Snapshot*>(snapshot_ptr);
  PopulateSnapshot(*snapshot);
}

}  // namespace

extern "C" void dmcp_adapter_setup() {
  if (g_ctx) {
    return;  // Already initialized
  }

  // Initialize the library context
  dmcp_config_t cfg = dmcp_default_config();
  cfg.on_tick       = AdapterTick;
  cfg.user_data     = nullptr;

  g_ctx = dmcp_create(&cfg);
}

extern "C" void dmcp_adapter_shutdown() {
  if (g_ctx) {
    dmcp_destroy(g_ctx);
    g_ctx = nullptr;
  }
}

extern "C" void dmcp_adapter_update() {
  if (g_ctx) {
    dmcp_update(g_ctx);
  }
}
