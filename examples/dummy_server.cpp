#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <thread>
#include <vector>

#include "dmcp/doom/api.h"

static volatile bool g_running = true;

void signal_handler(int) { g_running = false; }

enum class ParseResult {
  run,
  exit_success,
  exit_error,
};

void PrintUsage(const char* argv0) {
  const char* program = argv0 && argv0[0] ? argv0 : "dummy_server";
  std::printf("Usage: %s [--port PORT] [--target-hz HZ] [--help] [--examples]\n", program);
  std::printf("\nOptions:\n");
  std::printf("  --port PORT       HTTP/SSE port (default: 6060)\n");
  std::printf("  --target-hz HZ    Snapshot rate (default: 10)\n");
  std::printf("  --examples        Print canonical MCP tool examples\n");
  std::printf("  --help            Print this help\n");
}

void PrintExamples() {
  std::printf("Canonical MCP tool examples:\n");
  std::printf("  get_player:          {\"name\":\"get_player\",\"arguments\":{}}\n");
  std::printf(
      "  get_enemies:         "
      "{\"name\":\"get_enemies\",\"arguments\":{\"status\":\"alive\",\"limit\":8}}\n");
  std::printf(
      "  get_entities:        "
      "{\"name\":\"get_entities\",\"arguments\":{\"kind\":\"enemy\",\"status\":\"all\","
      "\"limit\":8}}\n");
  std::printf(
      "  get_items:           "
      "{\"name\":\"get_items\",\"arguments\":{\"kind\":\"armor\",\"limit\":8}}\n");
  std::printf(
      "  spawn_entity:        "
      "{\"name\":\"spawn_entity\",\"arguments\":{\"entity_class\":\"DoomImp\",\"x\":160,"
      "\"y\":96,\"angle\":0}}\n");
  std::printf(
      "  give_item:           "
      "{\"name\":\"give_item\",\"arguments\":{\"item_class\":\"Shotgun\",\"amount\":1}}\n");
  std::printf(
      "  set_player_position: "
      "{\"name\":\"set_player_position\",\"arguments\":{\"x\":160,\"y\":96,\"angle\":90}}\n");
  std::printf(
      "  player_input:        "
      "{\"name\":\"player_input\",\"arguments\":{\"action\":\"weapon\",\"value\":3}}\n");
}

bool ParsePositiveInt(const char* text, int* out_value) {
  if (!text || !out_value) {
    return false;
  }

  char* end   = nullptr;
  long  value = std::strtol(text, &end, 10);
  if (!end || *end != '\0' || value <= 0 || value > 65535) {
    return false;
  }

  *out_value = static_cast<int>(value);
  return true;
}

ParseResult ParseArgs(int argc, char** argv, int* port, int* target_hz) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--help") == 0) {
      PrintUsage(argv[0]);
      return ParseResult::exit_success;
    }
    if (std::strcmp(argv[i], "--examples") == 0) {
      PrintExamples();
      return ParseResult::exit_success;
    }
    if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
      if (!ParsePositiveInt(argv[++i], port)) {
        std::fprintf(stderr, "Invalid --port value\n");
        return ParseResult::exit_error;
      }
      continue;
    }
    if (std::strcmp(argv[i], "--target-hz") == 0 && i + 1 < argc) {
      if (!ParsePositiveInt(argv[++i], target_hz)) {
        std::fprintf(stderr, "Invalid --target-hz value\n");
        return ParseResult::exit_error;
      }
      continue;
    }

    std::fprintf(stderr, "Unknown option: %s\n", argv[i]);
    PrintUsage(argv[0]);
    return ParseResult::exit_error;
  }

  return ParseResult::run;
}

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

// DMCP callback to fill snapshot
void SnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
  auto* state = static_cast<GameState*>(user_data);

  // Level info
  snapshot->level.tic = state->tic;
  dmcp_strcpy(snapshot->level.level_id, "E1M1", sizeof(snapshot->level.level_id));
  dmcp_strcpy(snapshot->level.level_name, "Hangar", sizeof(snapshot->level.level_name));
  snapshot->level.kill_count   = 5;
  snapshot->level.item_count   = 2;
  snapshot->level.secret_count = 1;

  // Player
  snapshot->player.hp         = state->player_health;
  snapshot->player.armor      = 25.0f;
  snapshot->player.ammo[0]    = state->player_ammo;
  snapshot->player.position.x = state->player_x;
  snapshot->player.position.y = state->player_y;

  // Inventory
  dmcp_item_t shells = {};
  dmcp_strcpy(shells.name, "Shells", sizeof(shells.name));
  shells.amount = 24;
  dmcp_snapshot_add_item(snapshot, &shells);

  dmcp_item_t stimpack = {};
  dmcp_strcpy(stimpack.name, "Stimpack", sizeof(stimpack.name));
  stimpack.amount = 2;
  dmcp_snapshot_add_item(snapshot, &stimpack);

  // Enemies
  for (const auto& e : state->enemies) {
    dmcp_enemy_t enemy = {};
    enemy.id           = e.id;
    enemy.hp           = e.health;
    enemy.max_hp       = 60.0f;
    enemy.position.x   = e.x;
    enemy.position.y   = e.y;
    dmcp_strcpy(enemy.type, "DoomImp", sizeof(enemy.type));
    dmcp_snapshot_add_enemy(snapshot, &enemy);
  }
}

void LogCallback(void* /*user_data*/, int level, const char* message) {
  const char* prefix = "[DMCP]";
  switch (level) {
    case MCP_LOG_DEBUG:
      prefix = "[DMCP:D]";
      break;
    case MCP_LOG_INFO:
      prefix = "[DMCP:I]";
      break;
    case MCP_LOG_WARN:
      prefix = "[DMCP:W]";
      break;
    case MCP_LOG_ERROR:
      prefix = "[DMCP:E]";
      break;
  }
  printf("%s %s\n", prefix, message);
}

int main(int argc, char** argv) {
  std::signal(SIGINT, signal_handler);
  srand(static_cast<unsigned>(time(nullptr)));

  int               port         = 6060;
  int               target_hz    = 10;
  const ParseResult parse_result = ParseArgs(argc, argv, &port, &target_hz);
  if (parse_result == ParseResult::exit_success) {
    return 0;
  }
  if (parse_result == ParseResult::exit_error) {
    return 1;
  }

  GameState game;
  game.enemies.push_back({1, 200, 200, 60});
  game.enemies.push_back({2, -200, 200, 60});

  dmcp_config_t config = dmcp_config_default();
  config.port          = static_cast<uint16_t>(port);
  config.target_hz     = static_cast<uint32_t>(target_hz);
  config.on_snapshot   = SnapshotCallback;
  config.on_log        = LogCallback;
  config.user_data     = &game;

  // Create context
  dmcp_context_t* ctx = dmcp_context_create(&config);
  if (!ctx) {
    fprintf(stderr, "Failed to create DMCP context\n");
    return 1;
  }

  printf("Dummy server running on port %d. Press Ctrl+C to stop.\n", config.port);
  printf("Endpoints:\n");
  printf("  POST /mcp       - MCP JSON-RPC endpoint\n");
  printf("  GET /mcp        - Server-Sent Events stream\n");
  printf("  GET /health     - Health check\n\n");
  printf("  GET /game/state - Current game snapshot\n");
  printf("  GET /game/screenshot - Screenshot JSON (if enabled)\n\n");

  // Main loop
  while (g_running) {
    auto start = std::chrono::steady_clock::now();

    UpdateGame(game);
    dmcp_context_tick(ctx);

    // Log stats periodically
    if (game.tic % 35 == 0) {
      dmcp_stats_t stats;
      dmcp_stats_get(ctx, &stats);
      printf("Stats: clients=%lu, dropped=%lu\n",
             static_cast<unsigned long>(stats.connected_clients),
             static_cast<unsigned long>(stats.dropped_snapshots));
    }

    // Screenshot integration copies submitted pixels, so engine-owned frame
    // memory can be released immediately after dmcp_screenshot_submit returns.
    //
    // if (dmcp_screenshot_is_requested(ctx)) {
    //   uint8_t* pixels = CaptureScreenshot();
    //   dmcp_screenshot_frame_t frame = {
    //       .pixels = pixels,
    //       .width = width,
    //       .height = height,
    //       .stride = width * 4  // RGBA = 4 bytes per pixel
    //   };
    //
    //   mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);
    //   if (result.code != MCP_STATUS_CODE_OK) {
    //     fprintf(stderr, "Screenshot failed: %s\n", result.message);
    //   }
    //   free(pixels);  // Safe to free after submit (data is copied)
    // }

    // Maintain ~35Hz
    auto end     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    if (elapsed.count() < 28) {
      std::this_thread::sleep_for(std::chrono::milliseconds(28) - elapsed);
    }
  }

  printf("\nShutting down...\n");
  dmcp_context_destroy(ctx);
  return 0;
}
