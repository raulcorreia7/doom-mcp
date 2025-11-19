#include <cstring>
#include <vector>

#include "dmcp/binder.hpp"
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

dmcp::Binder g_binder;

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

void bind_defaults() {
  g_binder.Bind([](dmcp::Snapshot& snapshot) {
    snapshot.level.tic = 0;
    snapshot.player.health = 100.f;
  });
}

}  // namespace

extern "C" void dmcp_adapter_setup() {
#if defined(DMCP_WITH_UZDOOM)
  g_binder.Reset();
  bind_defaults();

  g_binder.Bind([](dmcp::Snapshot& snapshot) {
    player_t& player = ConsolePlayer();
    AActor* pawn = player.mo;
    snapshot.player.health = pawn ? pawn->health : 0;
  });

  g_binder.Bind([](dmcp::Snapshot& snapshot) {
    player_t& player = ConsolePlayer();
    AActor* pawn = player.mo;
    if (!pawn) {
      snapshot.player.armor = 0;
      return;
    }
    AActor* armor = pawn->FindInventory(NAME_BasicArmor, true);
    snapshot.player.armor =
        armor ? static_cast<float>(armor->IntVar(NAME_Amount)) : 0.f;
  });

  g_binder.Bind([](dmcp::Snapshot& snapshot) {
    player_t& player = ConsolePlayer();
    AActor* pawn = player.mo;
    if (pawn) {
      snapshot.player.position.x = static_cast<float>(pawn->Pos().X);
      snapshot.player.position.y = static_cast<float>(pawn->Pos().Y);
    } else {
      snapshot.player.position.x = snapshot.player.position.y = 0.f;
    }
  });

  g_binder.Bind([](dmcp::Snapshot& snapshot) {
    player_t& player = ConsolePlayer();
    if (player.ReadyWeapon == nullptr) {
      snapshot.player.ammo = 0;
      return;
    }
    auto* ammo = player.ReadyWeapon->PointerVar<AActor>(NAME_Ammo1);
    snapshot.player.ammo = ammo ? ammo->IntVar(NAME_Amount) : 0;
  });

  g_binder.Bind([](dmcp::Snapshot& snapshot) {
    player_t& player = ConsolePlayer();
    AActor* pawn = player.mo;
    if (pawn && pawn->Level) {
      snapshot.level.tic = pawn->Level->time;
      dmcp::CopyString(pawn->Level->MapName.GetChars(), snapshot.level.name);
    } else {
      snapshot.level.tic = 0;
      snapshot.level.name[0] = '\0';
    }
    snapshot.level.kill_count = player.killcount;
    snapshot.level.item_count = player.itemcount;
    snapshot.level.secret_count = player.secretcount;
  });

  g_binder.Bind([](dmcp::Snapshot& snapshot) {
    snapshot.enemies.clear();
    player_t& player = ConsolePlayer();
    AActor* pawn = player.mo;
    if (!pawn || !pawn->Level) {
      return;
    }
    TThinkerIterator<AActor> it(pawn->Level);
    while (auto* actor = it.Next()) {
      if (!(actor->flags & MF_COUNTKILL) || actor == pawn ||
          actor->health <= 0) {
        continue;
      }
      auto& entry = snapshot.enemies.emplace_back();
      entry.id = static_cast<int32_t>(actor->tid);
      entry.health = static_cast<float>(actor->health);
      entry.position.x = static_cast<float>(actor->Pos().X);
      entry.position.y = static_cast<float>(actor->Pos().Y);
      dmcp::CopyString(actor->GetClass()->TypeName.GetChars(), entry.type);
    }
  });

  g_binder.Bind([](dmcp::Snapshot& snapshot) {
    snapshot.player.inventory.clear();
    player_t& player = ConsolePlayer();
    AActor* pawn = player.mo;
    if (!pawn) {
      return;
    }
    for (AActor* item = pawn->Inventory; item != nullptr;
         item = item->Inventory) {
      int amount = item->IntVar(NAME_Amount);
      if (amount <= 0) {
        continue;
      }
      auto& dest = snapshot.player.inventory.emplace_back();
      dmcp::CopyString(ItemLabel(item), dest.name);
      dest.amount = amount;
    }
  });
#else
  bind_defaults();
#endif
}

extern "C" void DMCP_Process_Tick_Impl(dmcp::Snapshot* snapshot) {
  g_binder.Execute(snapshot);
}
