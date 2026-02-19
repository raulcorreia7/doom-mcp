// Chocolate Doom Adapter for DMCP SDK
// Provides MCP protocol integration for Chocolate Doom engine

#ifndef DMCP_ADAPTER_H
#define DMCP_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>

#include "dmcp/doom/dmcp.h"
#include "dmcp_ascii.h"

#ifdef __cplusplus
extern "C" {
#endif

// Chocolate Doom DMCP context
typedef struct dmcp_chocolate_s dmcp_chocolate_t;

// Configuration
typedef struct {
  dmcp_config_t base;       // DMCP base config (set target_hz=35 for Doom)
  const char*   iwad_path;  // Path to IWAD file
  const char*   pwad_path;  // Path to PWAD file (optional)
} dmcp_chocolate_config_t;

// Get default configuration
static inline dmcp_chocolate_config_t dmcp_chocolate_config_default(void) {
  dmcp_chocolate_config_t cfg = {0};
  cfg.base                    = dmcp_config_default();
  cfg.base.target_hz          = 35;     // Doom runs at 35 tics/sec
  cfg.base.screenshot.enable  = false;  // Disable ASCII screenshot path by default
  cfg.iwad_path               = NULL;
  cfg.pwad_path               = NULL;
  return cfg;
}

// ============================================================================
// Lifecycle
// ============================================================================

// Create Chocolate Doom context with DMCP support
dmcp_chocolate_t* dmcp_chocolate_create(const dmcp_chocolate_config_t* config);

// Destroy context
void dmcp_chocolate_destroy(dmcp_chocolate_t* ctx);

// ============================================================================
// Game Loop Integration
// ============================================================================

// Call every game tic (35 Hz)
// This snapshots game state and broadcasts to MCP clients
void dmcp_chocolate_tick(dmcp_chocolate_t* ctx);

// Process pending commands from MCP clients
void dmcp_chocolate_commands_process(dmcp_chocolate_t* ctx);

// ============================================================================
// State Queries
// ============================================================================

// Check if DMCP server is running
bool dmcp_chocolate_is_running(const dmcp_chocolate_t* ctx);

// Get statistics
void dmcp_chocolate_get_stats(dmcp_chocolate_t* ctx, dmcp_stats_t* stats);

// ============================================================================
// Command Execution
// ============================================================================

// Execute a command received via MCP
// Returns true if command was handled
bool dmcp_chocolate_command_execute(dmcp_chocolate_t* ctx, const dmcp_command_t* cmd);

// ============================================================================
// Snapshot Population (called by adapter during tick)
// ============================================================================

// Populate player state from Chocolate Doom's player_t
void dmcp_chocolate_populate_player(dmcp_snapshot_t* snap);

// Populate level state from Chocolate Doom globals
void dmcp_chocolate_populate_level(dmcp_snapshot_t* snap);

// Populate enemies from Chocolate Doom's thinker list
// Returns number of enemies added
int dmcp_chocolate_populate_enemies(dmcp_snapshot_t* snap);

#ifdef __cplusplus
}
#endif

#endif  // DMCP_ADAPTER_H
