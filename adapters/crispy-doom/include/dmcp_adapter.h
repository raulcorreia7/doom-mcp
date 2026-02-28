// Crispy Doom Adapter for DMCP SDK
// Provides MCP protocol integration for Crispy Doom engine

#ifndef DMCP_ADAPTER_H
#define DMCP_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>

#include "dmcp/doom/dmcp.h"
#include "dmcp_ascii.h"

#ifdef __cplusplus
extern "C" {
#endif

// Crispy Doom DMCP context
typedef struct dmcp_crispy_s dmcp_crispy_t;

// Configuration
typedef struct {
  dmcp_config_t base;       // DMCP base config (set target_hz=35 for Doom)
  const char*   iwad_path;  // Path to IWAD file
  const char*   pwad_path;  // Path to PWAD file (optional)
} dmcp_crispy_config_t;

// Get default configuration
static inline dmcp_crispy_config_t dmcp_crispy_config_default(void) {
  dmcp_crispy_config_t cfg   = {0};
  cfg.base                   = dmcp_config_default();
  cfg.base.target_hz         = 35;     // Doom runs at 35 tics/sec
  cfg.base.screenshot.enable = false;  // Disable ASCII screenshot path by default
  cfg.iwad_path              = NULL;
  cfg.pwad_path              = NULL;
  return cfg;
}

// ============================================================================
// Lifecycle
// ============================================================================

// Create Crispy Doom context with DMCP support
dmcp_crispy_t* dmcp_crispy_create(const dmcp_crispy_config_t* config);

// Destroy context
void dmcp_crispy_destroy(dmcp_crispy_t* ctx);

// ============================================================================
// Game Loop Integration
// ============================================================================

// Call every game tic (35 Hz)
// This snapshots game state and broadcasts to MCP clients
void dmcp_crispy_tick(dmcp_crispy_t* ctx);

// Process pending commands from MCP clients
void dmcp_crispy_commands_process(dmcp_crispy_t* ctx);

// Process pending player inputs from MCP clients (one per tick)
void dmcp_crispy_inputs_process(dmcp_crispy_t* ctx);

// Get the underlying DMCP context
dmcp_context_t* dmcp_crispy_get_dmcp_context(dmcp_crispy_t* ctx);

// ============================================================================
// State Queries
// ============================================================================

// Check if DMCP server is running
bool dmcp_crispy_is_running(const dmcp_crispy_t* ctx);

// Get statistics
void dmcp_crispy_get_stats(dmcp_crispy_t* ctx, dmcp_stats_t* stats);

// ============================================================================
// Command Execution
// ============================================================================

// Execute a command received via MCP
// Returns true if command was handled
bool dmcp_crispy_command_execute(dmcp_crispy_t* ctx, const dmcp_command_t* cmd);

// ============================================================================
// Logging
// ============================================================================

// Log a message (uses DMCP logging if available, otherwise printf)
// Thread-safe: may be called from any thread
void dmcp_adapter_log(int level, const char* fmt, ...);

// ============================================================================
// Snapshot Population (called by adapter during tick)
// ============================================================================

// Populate player state from Crispy Doom's player_t
void dmcp_crispy_populate_player(dmcp_snapshot_t* snap);

// Populate level state from Crispy Doom globals
void dmcp_crispy_populate_level(dmcp_snapshot_t* snap);

// Populate enemies from Crispy Doom's thinker list
// Returns number of enemies added
int dmcp_crispy_populate_enemies(dmcp_snapshot_t* snap);

// Populate non-enemy interactive world entities (pickups, barrels)
// Returns number of entities added
int dmcp_crispy_populate_entities(dmcp_snapshot_t* snap);

// Get the current player struct (may be NULL if not in game)
struct player_s* dmcp_get_player(void);

#ifdef __cplusplus
}
#endif

#endif  // DMCP_ADAPTER_H
