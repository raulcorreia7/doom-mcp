#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Doom MCP Public API
// 
// This is the public interface for integrating Doom engines with the
// Model Context Protocol. All functions are thread-safe unless noted.
// ============================================================================

// ============================================================================
// Lifecycle
// ============================================================================

// Create a new DMCP context
// Returns NULL on failure (e.g., server failed to start)
dmcp_context_t* dmcp_create(const dmcp_config_t* config);

// Destroy context and free all resources
void dmcp_destroy(dmcp_context_t* ctx);

// Check if server is running
bool dmcp_is_running(const dmcp_context_t* ctx);

// ============================================================================
// Game Loop Integration
// ============================================================================

// Process a game tick - should be called at the game's frame rate
// This will:
//   1. Call the on_snapshot callback to fill snapshot data
//   2. Queue the snapshot for broadcasting (rate-limited by target_hz)
void dmcp_tick(dmcp_context_t* ctx);

// ============================================================================
// Screenshot
// ============================================================================

// Check if a screenshot has been requested by a client
bool dmcp_screenshot_requested(const dmcp_context_t* ctx);

// Submit a screenshot frame
// The frame data is copied immediately, so pixels can be freed after return
dmcp_result_t dmcp_submit_screenshot(dmcp_context_t* ctx,
                                     const dmcp_screenshot_frame_t* frame);

// ============================================================================
// Utilities
// ============================================================================

// Get current statistics
void dmcp_get_stats(const dmcp_context_t* ctx, dmcp_stats_t* stats);

// Helper: Convert snapshot to JSON string
// Returns number of bytes written (excluding null terminator), or -1 on error
int dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot,
                          char* buffer, size_t buffer_size);

// Helper: Clear a snapshot structure
static inline void dmcp_snapshot_clear(dmcp_snapshot_t* snapshot) {
  if (!snapshot) return;
  snapshot->player.hp = 0;
  snapshot->player.armor = 0;
  snapshot->player.position.x = 0;
  snapshot->player.position.y = 0;
  snapshot->player.ammo = 0;
  snapshot->level.tic = 0;
  snapshot->level.level_id[0] = '\0';
  snapshot->level.level_name[0] = '\0';
  snapshot->level.kill_count = 0;
  snapshot->level.item_count = 0;
  snapshot->level.secret_count = 0;
  snapshot->enemy_count = 0;
  snapshot->inventory_count = 0;
}

// Helper: Add enemy to snapshot
static inline bool dmcp_snapshot_add_enemy(dmcp_snapshot_t* snapshot,
                                           const dmcp_enemy_t* enemy) {
  if (!snapshot || !enemy) return false;
  if (snapshot->enemy_count >= DMCP_MAX_ENEMIES) return false;
  snapshot->enemies[snapshot->enemy_count++] = *enemy;
  return true;
}

// Helper: Add item to inventory
static inline bool dmcp_snapshot_add_item(dmcp_snapshot_t* snapshot,
                                          const dmcp_item_t* item) {
  if (!snapshot || !item) return false;
  if (snapshot->inventory_count >= DMCP_MAX_INVENTORY) return false;
  snapshot->inventory[snapshot->inventory_count++] = *item;
  return true;
}

// Helper: Safe string copy
static inline void dmcp_strcpy(char* dest, const char* src, size_t dest_size) {
  if (!dest || !src || dest_size == 0) return;
  size_t i = 0;
  while (i < dest_size - 1 && src[i] != '\0') {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

#ifdef __cplusplus
}
#endif
