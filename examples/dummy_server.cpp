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

constexpr int k_stats_log_interval_ticks = 35;

void signal_handler(int) { g_running = false; }

enum class ParseResult {
  run,
  exit_success,
  exit_error,
};

void print_usage(const char* argv0) {
  const char* program = argv0 && argv0[0] ? argv0 : "dummy_server";
  std::printf("Usage: %s [--port PORT] [--target-hz HZ] [--help] [--examples]\n", program);
  std::printf("\nOptions:\n");
  std::printf("  --port PORT       HTTP/SSE port (default: %d)\n", MCP_DEFAULT_PORT);
  std::printf("  --target-hz HZ    DMCP snapshot sampling rate (default: %d)\n",
              DMCP_DEFAULT_TARGET_HZ);
  std::printf("  --examples        Print canonical MCP tool examples\n");
  std::printf("  --help            Print this help\n");
}

void print_examples() {
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

bool parse_positive_int(const char* text, int* out_value) {
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

ParseResult parse_args(int argc, char** argv, int* port, int* target_hz) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return ParseResult::exit_success;
    }
    if (std::strcmp(argv[i], "--examples") == 0) {
      print_examples();
      return ParseResult::exit_success;
    }
    if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
      if (!parse_positive_int(argv[++i], port)) {
        std::fprintf(stderr, "Invalid --port value\n");
        return ParseResult::exit_error;
      }
      continue;
    }
    if (std::strcmp(argv[i], "--target-hz") == 0 && i + 1 < argc) {
      if (!parse_positive_int(argv[++i], target_hz)) {
        std::fprintf(stderr, "Invalid --target-hz value\n");
        return ParseResult::exit_error;
      }
      continue;
    }

    std::fprintf(stderr, "Unknown option: %s\n", argv[i]);
    print_usage(argv[0]);
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

void update_game(GameState& state) {
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
void snapshot_callback(void* user_data, dmcp_snapshot_t* snapshot) {
  auto* state = static_cast<GameState*>(user_data);

  // Level info
  snapshot->level.tic = state->tic;
  mcp_strcpy_safe(snapshot->level.level_id, sizeof(snapshot->level.level_id), "E1M1");
  mcp_strcpy_safe(snapshot->level.level_name, sizeof(snapshot->level.level_name), "Hangar");
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
  mcp_strcpy_safe(shells.name, sizeof(shells.name), "Shells");
  shells.amount = 24;
  dmcp_snapshot_add_item(snapshot, &shells);

  dmcp_item_t stimpack = {};
  mcp_strcpy_safe(stimpack.name, sizeof(stimpack.name), "Stimpack");
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
    mcp_strcpy_safe(enemy.type, sizeof(enemy.type), "DoomImp");
    dmcp_snapshot_add_enemy(snapshot, &enemy);
  }
}

void log_callback(void* /*user_data*/, int level, const char* message) {
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

  int               port         = MCP_DEFAULT_PORT;
  int               target_hz    = DMCP_DEFAULT_TARGET_HZ;
  const ParseResult parse_result = parse_args(argc, argv, &port, &target_hz);
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
  config.on_snapshot   = snapshot_callback;
  config.on_log        = log_callback;
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
  printf("  GET /health     - Health check\n\n");
  printf("  GET /game/state - Current game snapshot\n");
  printf("  GET /game/screenshot - Screenshot JSON (if enabled)\n\n");

  // Main loop
  while (g_running) {
    auto start = std::chrono::steady_clock::now();

    update_game(game);
    dmcp_context_tick(ctx);

    // Log stats periodically
    if (game.tic % k_stats_log_interval_ticks == 0) {
      dmcp_stats_t stats;
      dmcp_stats_get(ctx, &stats);
      printf("Stats: clients=%lu\n", static_cast<unsigned long>(stats.connected_clients));
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
