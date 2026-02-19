#pragma once

#include "dmcp/doom/export.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Game-Specific Array Limits
// ============================================================================

/**
 * @brief Maximum number of enemies in snapshot
 *
 * Maximum count of enemy entries that can be included in a
 * single dmcp_snapshot_t structure.
 */
#define DMCP_MAX_ENEMIES 1024

/**
 * @brief Maximum inventory item count
 *
 * Maximum number of items that can be stored in snapshot inventory.
 */
#define DMCP_MAX_INVENTORY 64

/**
 * @brief Maximum item name length
 *
 * Maximum string length for item names (including null terminator).
 */
#define DMCP_MAX_ITEM_NAME 64

/**
 * @brief Maximum enemy type string length
 *
 * Maximum string length for enemy type/class names.
 */
#define DMCP_MAX_ENEMY_TYPE 128

/**
 * @brief Maximum level ID length
 *
 * Maximum string length for level/map identifiers.
 */
#define DMCP_MAX_LEVEL_ID 32

/**
 * @brief Maximum level name length
 *
 * Maximum string length for level/map display names.
 */
#define DMCP_MAX_LEVEL_NAME 96

#ifdef __cplusplus
}
#endif
