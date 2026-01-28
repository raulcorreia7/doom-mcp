#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Doom MCP Public API
//
// This is public interface for integrating Doom engines with the
// Model Context Protocol. All functions are thread-safe unless noted.
// ============================================================================

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * @brief Create a new DMCP context
 *
 * Creates and initializes a Doom MCP context with the specified
 * configuration. The context includes an MCP server, screenshot
 * handler, and command queue.
 *
 * @param config Configuration for context (port, callbacks, rate limiting)
 * @return Context handle on success, NULL on failure
 *
 * @note The caller owns the returned handle and must destroy it with
 *       dmcp_context_destroy() when done.
 * @warning Returns NULL if server fails to start (e.g., port in use)
 *
 * Breaking Changes:
 * - Function renamed from dmcp_create to dmcp_context_create (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * dmcp_context_t* ctx = dmcp_create(&config);
 *
 * // After
 * dmcp_context_t* ctx = dmcp_context_create(&config);
 * @endcode
 *
 * Example:
 * @code
 * void OnSnapshot(void* user, dmcp_snapshot_t* snapshot) {
 *     // Fill snapshot with game state...
 * }
 *
 * dmcp_config_t config = dmcp_config_default();
 * config.on_snapshot = OnSnapshot;
 *
 * dmcp_context_t* ctx = dmcp_context_create(&config);
 * if (!ctx) {
 *     fprintf(stderr, "Failed to create DMCP context\n");
 *     return -1;
 * }
 * // ... use context ...
 * dmcp_context_destroy(ctx);
 * @endcode
 */
dmcp_context_t* dmcp_context_create(const dmcp_config_t* config);

/**
 * @brief Destroy context and free all resources
 *
 * Stops the MCP server, closes all client connections, frees the
 * command queue, and releases all allocated resources. After this
 * call, the context handle becomes invalid.
 *
 * @param ctx Context handle (may be NULL, safe to call)
 *
 * @note Thread-safe - can be called from any thread
 * @note After this call, do not use the context handle
 *
 * Breaking Changes:
 * - Function renamed from dmcp_destroy to dmcp_context_destroy (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * dmcp_destroy(ctx);
 *
 * // After (same pattern, just renamed)
 * dmcp_context_destroy(ctx);
 * @endcode
 *
 * Example:
 * @code
 * dmcp_context_t* ctx = dmcp_context_create(&config);
 * // ... use context ...
 * dmcp_context_destroy(ctx);  // Clean up
 * ctx = NULL;  // Avoid dangling pointer
 * @endcode
 */
void dmcp_context_destroy(dmcp_context_t* ctx);

/**
 * @brief Check if server is running
 *
 * Returns true if the DMCP server (embedded in context) is
 * currently accepting connections and processing requests. Returns false
 * if server has been stopped or is in process of shutting down.
 *
 * @param ctx Context handle
 * @return true if server is running, false otherwise
 *
 * @note Returns false if context handle is NULL
 *
 * Breaking Changes:
 * - Function renamed from dmcp_is_running to dmcp_context_is_running (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * if (dmcp_is_running(ctx)) { ... }
 *
 * // After (same pattern, just renamed)
 * if (dmcp_context_is_running(ctx)) { ... }
 * @endcode
 *
 * Example:
 * @code
 * if (dmcp_context_is_running(ctx)) {
 *     printf("Server is active\n");
 * } else {
 *     printf("Server is not running\n");
 * }
 * @endcode
 */
bool dmcp_context_is_running(const dmcp_context_t* ctx);

// ============================================================================
// Game Loop Integration
// ============================================================================

/**
 * @brief Process a game tick
 *
 * Call this function at your game's frame rate to update DMCP state.
 * This function:
 *   1. Calls the on_snapshot callback to fill snapshot data
 *   2. Queues the snapshot for broadcasting (rate-limited by target_hz)
 *   3. Processes any pending commands from agents
 *
 * @param ctx Context handle
 *
 * @note Should be called from your game's main loop/thread
 * @note Snapshot rate is controlled by config.target_hz
 * @note Commands from agents are processed during this call
 *
 * Breaking Changes:
 * - Function renamed from dmcp_tick to dmcp_context_tick (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * dmcp_tick(ctx);
 *
 * // After (same pattern, just renamed)
 * dmcp_context_tick(ctx);
 * @endcode
 *
 * Example:
 * @code
 * // In your game loop
 * void GameLoop() {
 *     while (game_running) {
 *         GameTick();
 *         dmcp_context_tick(g_dmcp_ctx);  // Update DMCP
 *         Sleep(frametime);
 *     }
 * }
 * @endcode
 */
void dmcp_context_tick(dmcp_context_t* ctx);

// ============================================================================
// Screenshot
// ============================================================================

/**
 * @brief Check if a screenshot has been requested by a client
 *
 * Returns true if a client has requested a screenshot via the MCP
 * protocol. After taking a screenshot, submit it using
 * dmcp_screenshot_submit().
 *
 * @param ctx Context handle
 * @return true if screenshot requested, false otherwise
 *
 * @note After submission, the "requested" flag is cleared
 * @note Returns false if context handle is NULL or screenshots disabled
 *
 * Breaking Changes:
 * - Function renamed from dmcp_screenshot_requested to
 * dmcp_screenshot_is_requested (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * if (dmcp_screenshot_requested(ctx)) {
 *
 * // After (same pattern, just renamed)
 * if (dmcp_screenshot_is_requested(ctx)) {
 * @endcode
 *
 * Example:
 * @code
 * // In your render loop or frame tick
 * if (dmcp_screenshot_is_requested(ctx)) {
 *     uint8_t* pixels = CaptureScreenshot();
 *     dmcp_screenshot_frame_t frame = {
 *         .pixels = pixels,
 *         .width = width,
 *         .height = height,
 *         .stride = width * 4
 *     };
 *     dmcp_screenshot_submit(ctx, &frame);
 *     free(pixels);  // Safe to free after submit
 * }
 * @endcode
 */
bool dmcp_screenshot_is_requested(const dmcp_context_t* ctx);

/**
 * @brief Submit a screenshot frame
 *
 * Submits screenshot pixel data to the context. The data is copied
 * immediately, so the caller can free the pixel buffer after this
 * function returns.
 *
 * @param ctx Context handle
 * @param frame Screenshot frame data (pixels, width, height, stride)
 * @return Result struct with code and message
 *
 * @note Thread-safe - can be called from any thread
 * @note The frame data is copied, caller retains ownership of pixels
 * @note If queue is full, returns DMCP_ERROR_QUEUE_FULL
 *
 * Breaking Changes:
 * - Function renamed from dmcp_submit_screenshot to dmcp_screenshot_submit
 * (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * dmcp_submit_screenshot(ctx, &frame);
 *
 * // After (same pattern, just renamed)
 * dmcp_screenshot_submit(ctx, &frame);
 * @endcode
 *
 * Example:
 * @code
 * // After capturing screenshot
 * dmcp_screenshot_frame_t frame = {
 *     .pixels = screenshot_pixels,
 *     .width = width,
 *     .height = height,
 *     .stride = width * 4  // RGBA = 4 bytes per pixel
 * };
 *
 * dmcp_result_t result = dmcp_screenshot_submit(ctx, &frame);
 * if (result.code != DMCP_RESULT_CODE_OK) {
 *     fprintf(stderr, "Screenshot submit failed: %s\n", result.message);
 * }
 * @endcode
 */
dmcp_result_t dmcp_screenshot_submit(dmcp_context_t*                ctx,
                                     const dmcp_screenshot_frame_t* frame);

// ============================================================================
// Utilities
// ============================================================================

/**
 * @brief Get current statistics
 *
 * Retrieves statistics about DMCP context operation, including
 * dropped snapshots, dropped screenshots, and client connections.
 * Useful for monitoring and debugging.
 *
 * @param ctx Context handle
 * @param stats Output structure to receive statistics
 *
 * @note Thread-safe - can be called from any thread
 * @note Stats are cumulative since context creation
 *
 * Breaking Changes:
 * - Function renamed from dmcp_get_stats to dmcp_stats_get (v0.5.0)
 *
 * Migration:
 * @code
 * // Before
 * dmcp_stats_t stats;
 * dmcp_get_stats(ctx, &stats);
 *
 * // After (same pattern, just renamed)
 * dmcp_stats_t stats;
 * dmcp_stats_get(ctx, &stats);
 * @endcode
 *
 * Example:
 * @code
 * dmcp_stats_t stats;
 * dmcp_stats_get(ctx, &stats);
 * printf("Dropped snapshots: %llu\n", stats.dropped_snapshots);
 * printf("Dropped screenshots: %llu\n", stats.dropped_screenshots);
 * printf("Connected clients: %llu\n", stats.connected_clients);
 * @endcode
 */
void dmcp_stats_get(const dmcp_context_t* ctx, dmcp_stats_t* stats);

/**
 * @brief Convert snapshot to JSON string
 *
 * Serializes a dmcp_snapshot_t structure into a JSON string.
 * Useful for logging, debugging, or custom transport implementations.
 *
 * @param snapshot Snapshot to serialize
 * @param buffer Output buffer for JSON string
 * @param buffer_size Size of output buffer
 * @return Number of bytes written (excluding null terminator), or -1 on error
 *
 * @note Output is null-terminated
 * @note Returns -1 if buffer too small or snapshot is NULL
 *
 * Example:
 * @code
 * char json[4096];
 * dmcp_snapshot_t* snapshot = ...;
 *
 * int written = dmcp_snapshot_to_json(snapshot, json, sizeof(json));
 * if (written < 0) {
 *     fprintf(stderr, "JSON serialization failed\n");
 * } else {
 *     printf("Snapshot JSON: %s\n", json);
 * }
 * @endcode
 */
int dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot, char* buffer,
                          size_t buffer_size);

/**
 * @brief Clear a snapshot structure
 *
 * Resets all fields in a snapshot structure to zero/empty values.
 * This is useful when reusing snapshot structures.
 *
 * @param snapshot Snapshot to clear (may be NULL, safe)
 *
 * Example:
 * @code
 * dmcp_snapshot_t snapshot;
 * // ... use snapshot ...
 * dmcp_snapshot_clear(&snapshot);  // Reset for reuse
 * @endcode
 */
static inline void dmcp_snapshot_clear(dmcp_snapshot_t* snapshot) {
  if (!snapshot) return;
  snapshot->player.hp           = 0;
  snapshot->player.armor        = 0;
  snapshot->player.position.x   = 0;
  snapshot->player.position.y   = 0;
  snapshot->player.ammo         = 0;
  snapshot->level.tic           = 0;
  snapshot->level.level_id[0]   = '\0';
  snapshot->level.level_name[0] = '\0';
  snapshot->level.kill_count    = 0;
  snapshot->level.item_count    = 0;
  snapshot->level.secret_count  = 0;
  snapshot->enemy_count         = 0;
  snapshot->inventory_count     = 0;
}

/**
 * @brief Add enemy to snapshot
 *
 * Adds an enemy to the snapshot's enemy array. Checks array bounds
 * and increments enemy count.
 *
 * @param snapshot Snapshot to add enemy to
 * @param enemy Enemy data to add
 * @return true if added successfully, false if full or NULL inputs
 *
 * @note Fails silently if snapshot->enemy_count >= DMCP_MAX_ENEMIES
 * @note Thread-safe (atomic increment if used correctly)
 *
 * Example:
 * @code
 * dmcp_snapshot_t snapshot;
 * dmcp_snapshot_clear(&snapshot);
 *
 * for (each enemy in game) {
 *     dmcp_enemy_t enemy = {
 *         .id = enemy->id,
 *         .hp = enemy->health,
 *         .position = {enemy->x, enemy->y},
 *         .max_hp = enemy->max_health,
 *     };
 *     strcpy(enemy.type, enemy->className, sizeof(enemy.type));
 *
 *     if (!dmcp_snapshot_add_enemy(&snapshot, &enemy)) {
 *         fprintf(stderr, "Enemy array full\n");
 *         break;
 *     }
 * }
 * @endcode
 */
static inline bool dmcp_snapshot_add_enemy(dmcp_snapshot_t*    snapshot,
                                           const dmcp_enemy_t* enemy) {
  if (!snapshot || !enemy) return false;
  if (snapshot->enemy_count >= DMCP_MAX_ENEMIES) return false;
  snapshot->enemies[snapshot->enemy_count++] = *enemy;
  return true;
}

/**
 * @brief Add item to inventory
 *
 * Adds an item to the snapshot's inventory array. Checks array bounds
 * and increments inventory count.
 *
 * @param snapshot Snapshot to add item to
 * @param item Item data to add
 * @return true if added successfully, false if full or NULL inputs
 *
 * @note Fails silently if snapshot->inventory_count >= DMCP_MAX_INVENTORY
 *
 * Example:
 * @code
 * dmcp_snapshot_t snapshot;
 * dmcp_snapshot_clear(&snapshot);
 *
 * for (each item in player->inventory) {
 *     dmcp_item_t inv_item = {
 *         .amount = item->amount,
 *     };
 *     strcpy(inv_item.name, item->name, sizeof(inv_item.name));
 *
 *     if (!dmcp_snapshot_add_item(&snapshot, &inv_item)) {
 *         fprintf(stderr, "Inventory full\n");
 *         break;
 *     }
 * }
 * @endcode
 */
static inline bool dmcp_snapshot_add_item(dmcp_snapshot_t*   snapshot,
                                          const dmcp_item_t* item) {
  if (!snapshot || !item) return false;
  if (snapshot->inventory_count >= DMCP_MAX_INVENTORY) return false;
  snapshot->inventory[snapshot->inventory_count++] = *item;
  return true;
}

/**
 * @brief Safe string copy
 *
 * Copies a source string to destination with bounds checking.
 * Unlike strcpy(), this function ensures the destination is always
 * null-terminated and never overflows the buffer.
 *
 * @param dest Destination buffer
 * @param src Source string
 * @param dest_size Size of destination buffer
 *
 * @note Safe to call with NULL pointers (no-op)
 * @note Always null-terminates dest
 *
 * Example:
 * @code
 * char level_name[DMCP_MAX_LEVEL_NAME];
 * dmcp_strcpy(level_name, "E1M1: Hangar", sizeof(level_name));
 * printf("Level: %s\n", level_name);
 * @endcode
 */
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
