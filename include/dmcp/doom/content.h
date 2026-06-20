#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "dmcp/doom/constants.h"
#include "dmcp/doom/export.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Content Availability Checks
// ============================================================================

DMCP_API bool dmcp_is_weapon_available(const char* weapon_name, dmcp_gamemode_t mode);
DMCP_API bool dmcp_is_enemy_spawnable(const char* enemy_type, dmcp_gamemode_t mode);
DMCP_API bool dmcp_is_map_available(const char* map_name, dmcp_gamemode_t mode);
DMCP_API bool dmcp_is_item_available(const char* item_name, dmcp_gamemode_t mode);

// ============================================================================
// All Available Content (for discovery endpoints)
// ============================================================================

extern DMCP_API const char* const dmcp_all_weapons[];
extern DMCP_API const size_t      dmcp_all_weapons_count;

extern DMCP_API const char* const dmcp_all_items[];
extern DMCP_API const size_t      dmcp_all_items_count;

extern DMCP_API const char* const dmcp_all_enemies[];
extern DMCP_API const size_t      dmcp_all_enemies_count;

extern DMCP_API const char* const dmcp_all_maps_doom1[];
extern DMCP_API const size_t      dmcp_all_maps_doom1_count;

extern DMCP_API const char* const dmcp_all_maps_doom2[];
extern DMCP_API const size_t      dmcp_all_maps_doom2_count;

// ============================================================================
// Content Restriction Data (for adapter iteration)
// ============================================================================

extern DMCP_API const char* const dmcp_shareware_restricted_weapons[];
extern DMCP_API const size_t      dmcp_shareware_restricted_weapons_count;

extern DMCP_API const char* const dmcp_shareware_restricted_enemies[];
extern DMCP_API const size_t      dmcp_shareware_restricted_enemies_count;

extern DMCP_API const char* const dmcp_shareware_restricted_maps[];
extern DMCP_API const size_t      dmcp_shareware_restricted_maps_count;

extern DMCP_API const char* const dmcp_shareware_restricted_items[];
extern DMCP_API const size_t      dmcp_shareware_restricted_items_count;

// ============================================================================
// Error Message Builders
// ============================================================================

/**
 * Build a content-unavailable error into a caller-owned buffer.
 *
 * Returns the number of bytes written excluding the null terminator, or -1 when
 * the buffer is null/empty or too small.
 */
DMCP_API int dmcp_content_unavailable_message_copy(const char* content_type,
                                                   const char* content_name, dmcp_gamemode_t mode,
                                                   char* buffer, size_t buffer_size);

/**
 * Return a thread-local content-unavailable error string.
 *
 * The returned pointer is valid until the next call to this function on the same
 * thread. Prefer dmcp_content_unavailable_message_copy() for stable storage.
 */
DMCP_API const char* dmcp_content_unavailable_message(const char*     content_type,
                                                      const char*     content_name,
                                                      dmcp_gamemode_t mode);

#ifdef __cplusplus
}
#endif
