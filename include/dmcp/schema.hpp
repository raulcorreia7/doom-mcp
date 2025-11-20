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
  float                 hp     = 0.f;
  float                 max_hp = 0.f;
  Vector2D              position{};
  std::array<char, 128> type{};
};

struct InventoryItem {
  std::array<char, 64> name{};
  int32_t              amount = 0;
};

struct PlayerState {
  float                      hp    = 0.f;
  float                      armor = 0.f;
  Vector2D                   position{};
  int32_t                    ammo = 0;
  std::vector<InventoryItem> inventory;
};

struct LevelState {
  int32_t              tic = 0;
  std::array<char, 32> id{};    // e.g., MAP01/E1M1
  std::array<char, 96> name{};  // human-friendly name/title
  int32_t              kill_count   = 0;
  int32_t              item_count   = 0;
  int32_t              secret_count = 0;
};

struct Snapshot {
  PlayerState        player;
  LevelState         level;
  std::vector<Enemy> enemies;

  void Clear() {
    player = PlayerState{};
    level  = LevelState{};
    enemies.clear();
  }
};

}  // namespace dmcp
