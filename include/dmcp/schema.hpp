#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace dmcp {

struct Vector2D {
  float x = 0.f;
  float y = 0.f;
};

struct Enemy {
  int32_t               id     = 0;
  float                 health = 0.f;
  Vector2D              position{};
  std::array<char, 128> type{};
};

struct InventoryItem {
  std::array<char, 64> name{};
  int32_t              amount = 0;
};

struct PlayerState {
  float                      health = 0.f;
  float                      armor  = 0.f;
  Vector2D                   position{};
  int32_t                    ammo = 0;
  std::vector<InventoryItem> inventory;
};

struct LevelState {
  int32_t              tic = 0;
  std::array<char, 64> name{};
  int32_t              kill_count   = 0;
  int32_t              item_count   = 0;
  int32_t              secret_count = 0;
};

struct Snapshot {
  PlayerState        player;
  LevelState         level;
  std::vector<Enemy> enemies;

  void Clear() {
    player.health   = 0.f;
    player.armor    = 0.f;
    player.position = {};
    player.ammo     = 0;
    player.inventory.clear();

    level.tic = 0;
    level.name.fill(0);
    level.kill_count   = 0;
    level.item_count   = 0;
    level.secret_count = 0;

    enemies.clear();
  }
};

}  // namespace dmcp
