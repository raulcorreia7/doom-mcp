#pragma once

#include "dmcp/doom/export.h"
#include "commands.h"
#include "config.h"
#include "types.h"
#include "mcp/core/string.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

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
DMCP_API dmcp_context_t* dmcp_context_create(const dmcp_config_t* config);

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
 * Example:
 * @code
 * dmcp_context_t* ctx = dmcp_context_create(&config);
 * // ... use context ...
 * dmcp_context_destroy(ctx);  // Clean up
 * ctx = NULL;  // Avoid dangling pointer
 * @endcode
 */
DMCP_API void dmcp_context_destroy(dmcp_context_t* ctx);

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
 * Example:
 * @code
 * if (dmcp_context_is_running(ctx)) {
 *     printf("Server is active\n");
 * } else {
 *     printf("Server is not running\n");
 * }
 * @endcode
 */
DMCP_API bool dmcp_context_is_running(const dmcp_context_t* ctx);

// ============================================================================
// Game Loop Integration
// ============================================================================

/**
 * @brief Process a game tick
 *
 * Call this function at your game's frame rate to update DMCP state.
 * This function:
 *   1. Calls the on_snapshot callback to fill snapshot data
 *   2. Publishes the latest sampled snapshot for MCP readers
 *
 * @param ctx Context handle
 *
 * @note Should be called from your game's main loop/thread
 * @note Snapshot sampling rate is controlled by config.target_hz
 * @note Command processing is adapter-owned so engines can apply changes on
 *       the correct game-thread boundary.
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
DMCP_API void dmcp_context_tick(dmcp_context_t* ctx);

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
DMCP_API bool dmcp_screenshot_is_requested(const dmcp_context_t* ctx);

/**
 * @brief Request a screenshot from the engine
 *
 * Increments the pending screenshot request count. Engine integrations should
 * poll dmcp_screenshot_is_requested() after rendering and submit a normalized
 * RGBA frame with dmcp_screenshot_submit().
 *
 * @param ctx Context handle
 * @return true if screenshots are enabled and a request was queued
 */
DMCP_API bool dmcp_screenshot_request(dmcp_context_t* ctx);

/**
 * @brief Submit a screenshot frame
 *
 * Submits screenshot pixel data to the context. The data is copied
 * immediately, so the caller can free the pixel buffer after this
 * function returns.
 *
 * @param ctx Context handle
 * @param frame Screenshot frame data (pixels, width, height, stride)
 * @return Status with code and message
 *
 * @note Thread-safe - can be called from any thread
 * @note The frame data is copied, caller retains ownership of pixels
 * @note If queue is full, returns MCP_STATUS_ERROR(-4, "Queue full")
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
 * mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);
 * if (result.code != 0) {
 *     fprintf(stderr, "Screenshot submit failed: %s\n", result.message);
 * }
 * @endcode
 */
DMCP_API mcp_status_t dmcp_screenshot_submit(dmcp_context_t*                ctx,
                                             const dmcp_screenshot_frame_t* frame);

/**
 * @brief Get ASCII representation of latest screenshot
 *
 * Returns the ASCII art representation of the most recently submitted
 * screenshot frame.
 *
 * @param ctx Context handle
 * @param target_width Target width in characters (default 160)
 * @return Pointer to ASCII string, or NULL if no screenshot available
 *
 * @note Returns NULL if context is NULL or no screenshot submitted
 * @note The returned pointer is thread-local and valid until the next
 *       dmcp_screenshot_get_ascii() call on the same thread
 * @note Thread-safe
 */
DMCP_API const char* dmcp_screenshot_get_ascii(dmcp_context_t* ctx, uint32_t target_width);

/**
 * @brief Copy ASCII representation of latest screenshot into a caller buffer
 *
 * @param ctx Context handle
 * @param buffer Output buffer for a null-terminated ASCII string
 * @param buffer_size Size of output buffer
 * @param target_width Target width in characters (default 160)
 * @return Number of bytes written excluding the null terminator, or -1 on error
 *
 * @note Prefer this function for stable C API integrations.
 * @note Returns -1 if no screenshot is available or the buffer is too small.
 */
DMCP_API int dmcp_screenshot_copy_ascii(dmcp_context_t* ctx, char* buffer, size_t buffer_size,
                                        uint32_t target_width);

/**
 * @brief Get screenshot as JSON with metadata
 *
 * Serializes the latest screenshot as JSON including metadata
 * (dimensions, format, ASCII art).
 *
 * @param ctx Context handle
 * @param buffer Output buffer for JSON string
 * @param buffer_size Size of output buffer
 * @param target_width Target width in characters (default 160)
 * @return Number of bytes written, or -1 on error
 *
 * @note Output format: {"width":N,"height":N,"ascii":"..."}
 * @note Returns -1 if buffer too small or context is NULL
 */
DMCP_API int dmcp_screenshot_to_json(dmcp_context_t* ctx, char* buffer, size_t buffer_size,
                                     uint32_t target_width);

// ============================================================================
// Utilities
// ============================================================================

/**
 * @brief Get current statistics
 *
 * Retrieves statistics about DMCP context operation, including screenshot
 * drops and client connections. Useful for monitoring and debugging.
 *
 * @param ctx Context handle
 * @param stats Output structure to receive statistics
 *
 * @note Thread-safe - can be called from any thread
 * @note Stats are cumulative since context creation
 * Example:
 * @code
 * dmcp_stats_t stats;
 * dmcp_stats_get(ctx, &stats);
 * printf("Dropped screenshots: %llu\n", stats.dropped_screenshots);
 * printf("Connected clients: %llu\n", stats.connected_clients);
 * @endcode
 */
DMCP_API void dmcp_stats_get(const dmcp_context_t* ctx, dmcp_stats_t* stats);

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
DMCP_API int dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot, char* buffer,
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
  memset(snapshot, 0, sizeof(dmcp_snapshot_t));
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
 *     mcp_strcpy_safe(enemy.type, sizeof(enemy.type), enemy->className);
 *
 *     if (!dmcp_snapshot_add_enemy(&snapshot, &enemy)) {
 *         fprintf(stderr, "Enemy array full\n");
 *         break;
 *     }
 * }
 * @endcode
 */
static inline bool dmcp_snapshot_add_enemy(dmcp_snapshot_t* snapshot, const dmcp_enemy_t* enemy) {
  if (!snapshot || !enemy) return false;
  if (snapshot->enemy_count >= DMCP_MAX_ENEMIES) return false;
  snapshot->enemies[snapshot->enemy_count++] = *enemy;
  return true;
}

/**
 * @brief Add world entity to snapshot
 *
 * Adds a non-enemy world entity (for example pickups or barrels)
 * to the snapshot entity array.
 *
 * @param snapshot Snapshot to add entity to
 * @param entity Entity data to add
 * @return true if added successfully, false if full or NULL inputs
 *
 * @note Fails silently if snapshot->entity_count >= DMCP_MAX_ENTITIES
 */
static inline bool dmcp_snapshot_add_entity(dmcp_snapshot_t*     snapshot,
                                            const dmcp_entity_t* entity) {
  if (!snapshot || !entity) return false;
  if (snapshot->entity_count >= DMCP_MAX_ENTITIES) return false;
  snapshot->entities[snapshot->entity_count++] = *entity;
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
 *     mcp_strcpy_safe(inv_item.name, sizeof(inv_item.name), item->name);
 *
 *     if (!dmcp_snapshot_add_item(&snapshot, &inv_item)) {
 *         fprintf(stderr, "Inventory full\n");
 *         break;
 *     }
 * }
 * @endcode
 */
static inline bool dmcp_snapshot_add_item(dmcp_snapshot_t* snapshot, const dmcp_item_t* item) {
  if (!snapshot || !item) return false;
  if (snapshot->inventory_count >= DMCP_MAX_INVENTORY) return false;
  snapshot->inventory[snapshot->inventory_count++] = *item;
  return true;
}

/**
 * @brief Add an available map name to the snapshot's map catalog.
 *
 * Adapters that know the loaded game's map catalog should add each canonical
 * map here. DMCP exposes this catalog through get_available_maps and uses it
 * to validate change_level commands. If the catalog is empty, DMCP falls back
 * to the base Doom map list for the current game mode.
 *
 * @param snapshot Snapshot to add map to
 * @param map_name Canonical Doom map name, e.g. E1M1 or MAP01
 * @return true if added successfully, false if full or NULL/empty inputs
 */
static inline bool dmcp_snapshot_add_map(dmcp_snapshot_t* snapshot, const char* map_name) {
  if (!snapshot || !map_name || map_name[0] == '\0') return false;
  if (snapshot->map_count >= DMCP_MAX_MAPS) return false;
  mcp_strcpy_safe(snapshot->maps[snapshot->map_count].name,
                  sizeof(snapshot->maps[snapshot->map_count].name), map_name);
  snapshot->map_count++;
  return true;
}

#ifdef __cplusplus
}
#endif
