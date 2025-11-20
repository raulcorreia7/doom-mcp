#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <random>
#include <thread>
#include <vector>

#include "dmcp/dmcp.h"

static volatile bool g_running = true;

void signal_handler(int) { g_running = false; }

// Simple mock game state
struct GameState {
  float player_x      = 0.0f;
  float player_y      = 0.0f;
  float player_health = 100.0f;
  int   player_ammo   = 50;
  int   tic           = 0;

  struct MockEnemy {
    int   id;
    float x, y;
    float health;
  };
  std::vector<MockEnemy> enemies;
};

void UpdateGame(GameState& state) {
  state.tic++;

  // Move player in a circle
  state.player_x = 100.0f * std::cos(state.tic * 0.01f);
  state.player_y = 100.0f * std::sin(state.tic * 0.01f);

  // Randomly damage/heal player
  if (rand() % 100 < 2) state.player_health -= 5.0f;
  if (state.player_health < 0) state.player_health = 100.0f;

  // Update enemies
  for (auto& enemy : state.enemies) {
    enemy.x += (rand() % 3 - 1) * 2.0f;
    enemy.y += (rand() % 3 - 1) * 2.0f;
  }
}

// Callback to bind game state to DMCP snapshot
void BindSnapshot(void* user_data, void* snapshot_ptr) {
  auto* state = static_cast<GameState*>(user_data);

  // We need to manually cast to the C++ type if we were in C++,
  // but since we are linking against the library, we can't easily access the
  // C++ Snapshot struct from this "user" code unless we include the C++ schema
  // header. Ideally, the C API should provide helper functions to set fields,
  // OR we assume the user is writing C++ and includes `dmcp/schema.hpp`.

  // For this example, let's assume we are a C++ client and can include the
  // schema. BUT, `dmcp.h` exposes `dmcp_tick_callback_t` taking `void*
  // snapshot`. We need to cast it to `dmcp::Snapshot*`.

  // Wait, `dmcp/schema.hpp` is a public header. So we can include it.
}

// We need to include schema to actually set data
#include "dmcp/schema.hpp"

void BindSnapshotCpp(void* user_data, void* snapshot_ptr) {
  auto* state    = static_cast<GameState*>(user_data);
  auto* snapshot = static_cast<dmcp::Snapshot*>(snapshot_ptr);

  snapshot->level.tic = state->tic;
  // Safe string copy
  std::strncpy(snapshot->level.name.data(), "E1M1: Hangar",
               snapshot->level.name.size() - 1);

  snapshot->player.hp     = state->player_health;
  snapshot->player.ammo       = state->player_ammo;
  snapshot->player.position.x = state->player_x;
  snapshot->player.position.y = state->player_y;

  snapshot->enemies.clear();
  for (const auto& e : state->enemies) {
    auto& dest      = snapshot->enemies.emplace_back();
    dest.id         = e.id;
    dest.hp     = e.health;
    dest.position.x = e.x;
    dest.position.y = e.y;
    std::strncpy(dest.type.data(), "Imp", dest.type.size() - 1);
  }
}

int main() {
  std::signal(SIGINT, signal_handler);
  std::srand(std::time(nullptr));

  GameState game;
  game.enemies.push_back({1, 200, 200, 60});
  game.enemies.push_back({2, -200, 200, 60});

  dmcp_config_t config = dmcp_default_config();
  config.port          = 9090;
  config.target_hz     = 35;  // Doom runs at 35Hz
  config.on_tick       = BindSnapshotCpp;
  config.user_data     = &game;

  dmcp_context_t* ctx = dmcp_create(&config);
  if (!ctx) {
    std::fprintf(stderr, "Failed to create DMCP context\n");
    return 1;
  }

  std::printf("Dummy server running on port %d. Press Ctrl+C to stop.\n",
              config.port);

  // Generate a dummy screenshot (red/blue checkerboard)
  std::vector<uint8_t> pixels(640 * 480 * 4);

  while (g_running) {
    auto start = std::chrono::steady_clock::now();

    UpdateGame(game);

    // Process DMCP updates
    dmcp_update(ctx);

    // Handle screenshot requests
    if (dmcp_has_screenshot_request(ctx)) {
      std::printf("Screenshot requested!\n");

      // Fill with dynamic pattern
      for (int y = 0; y < 480; ++y) {
	for (int x = 0; x < 640; ++x) {
	  int i         = (y * 640 + x) * 4;
	  pixels[i + 0] = (x + game.tic) % 255;  // R
	  pixels[i + 1] = (y + game.tic) % 255;  // G
	  pixels[i + 2] = 0;                     // B
	  pixels[i + 3] = 255;                   // A
	}
      }

      dmcp_screenshot_frame_t frame;
      frame.pixels = pixels.data();
      frame.width  = 640;
      frame.height = 480;
      frame.stride = 640 * 4;

      dmcp_result_t res = dmcp_submit_screenshot(ctx, &frame);
      if (res != DMCP_OK) {
	std::fprintf(stderr, "Failed to submit screenshot: %d\n", res);
      }
    }

    // Log stats every ~1 second (35 ticks)
    if (game.tic % 35 == 0) {
      dmcp_stats_t stats;
      dmcp_get_stats(ctx, &stats);
      std::printf("Stats: dropped=%lu, clients=%lu\n", stats.dropped_snapshots,
                  stats.connected_clients);
      std::fflush(stdout);
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed =
	std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    if (elapsed.count() < 28) {  // ~35Hz
      std::this_thread::sleep_for(std::chrono::milliseconds(28) - elapsed);
    }
  }

  dmcp_destroy(ctx);
  return 0;
}
