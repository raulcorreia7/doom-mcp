// Crispy Doom Adapter for DMCP SDK
// Provides MCP protocol integration for Crispy Doom engine

#ifndef DMCP_ADAPTER_H
#define DMCP_ADAPTER_H

#include "dmcp/adapters/crispy.h"
#include "dmcp_ascii.h"

#ifdef __cplusplus
extern "C" {
#endif

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

// Execute one queued DMCP command against Crispy Doom and return a concise result message.
bool dmcp_crispy_command_execute(dmcp_crispy_t* ctx, const dmcp_command_t* cmd, char* out_message,
                                 size_t out_message_size);

#ifdef __cplusplus
}
#endif

#endif  // DMCP_ADAPTER_H
