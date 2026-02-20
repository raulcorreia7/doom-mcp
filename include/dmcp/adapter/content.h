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

DMCP_API const char* dmcp_content_unavailable_message(const char*     content_type,
                                                      const char*     content_name,
                                                      dmcp_gamemode_t mode);

#ifdef __cplusplus
}
#endif
