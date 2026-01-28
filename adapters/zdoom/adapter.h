#pragma once

#include "dmcp/doom/api.h"
#include "dmcp/doom/commands.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ZDoom Adapter
//
// This adapter provides zero-configuration integration with ZDoom/GZDoom.
// It automatically extracts game state from the ZDoom engine.
//
// Usage:
//   1. Include this header in your ZDoom build
//   2. Call dmcp_zdoom_create() at startup
//   3. Call dmcp_zdoom_tick() every game tic
//   4. Call dmcp_zdoom_destroy() at shutdown
// ============================================================================

typedef struct dmcp_zdoom_s dmcp_zdoom_t;

// ============================================================================
// Configuration
// ============================================================================
typedef struct {
  uint32_t struct_size;

  // Optional: Override base DMCP config. If NULL, defaults are used.
  const dmcp_config_t* dmcp_config;

  // Optional: Custom logging hook
  void (*log_fn)(void* user, int level, const char* message);
  void* log_user;

  // Optional: Gate to skip ticking (e.g., during pause/menu)
  bool (*should_tick_fn)(void* user);
  void* should_tick_user;

  // Optional: Custom port override (overrides DMCP_PORT env var)
  uint16_t port_override;

} dmcp_zdoom_config_t;

static inline dmcp_zdoom_config_t dmcp_zdoom_config_default(void) {
  dmcp_zdoom_config_t cfg = {};
  cfg.struct_size         = sizeof(dmcp_zdoom_config_t);
  cfg.dmcp_config         = NULL;
  cfg.log_fn              = NULL;
  cfg.log_user            = NULL;
  cfg.should_tick_fn      = NULL;
  cfg.should_tick_user    = NULL;
  cfg.port_override       = 0;
  return cfg;
}

// ============================================================================
// Lifecycle
// ============================================================================

// Create ZDoom adapter. Returns NULL on failure.
dmcp_zdoom_t* dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg);

// Destroy adapter and shutdown server
void dmcp_zdoom_destroy(dmcp_zdoom_t* ctx);

// ============================================================================
// Game Loop
// ============================================================================

// Call once per game tic (from G_Ticker)
// Returns DMCP_OK on success
int dmcp_zdoom_tick(dmcp_zdoom_t* ctx);

// ============================================================================
// State
// ============================================================================

bool dmcp_zdoom_is_running(dmcp_zdoom_t* ctx);
void dmcp_zdoom_get_stats(dmcp_zdoom_t* ctx, dmcp_stats_t* out_stats);

// ============================================================================
// Command Processing (Agent -> Game)
// ============================================================================

// Execute a single command
bool dmcp_zdoom_command_execute(dmcp_zdoom_t* ctx, const dmcp_command_t* cmd);

// Process all pending commands (call from game loop)
void dmcp_zdoom_commands_process(dmcp_zdoom_t* ctx);

#ifdef __cplusplus
}
#endif
