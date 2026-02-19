#pragma once

#include "dmcp/doom/export.h"
#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Command Types - Agent -> Game communication
// ============================================================================

typedef enum {
  DMCP_CMD_UNKNOWN = 0,
  DMCP_CMD_SPAWN_ENTITY,
  DMCP_CMD_CHANGE_LEVEL,
  DMCP_CMD_GIVE_ITEM,
  DMCP_CMD_SET_PLAYER_HEALTH,
  DMCP_CMD_SET_PLAYER_POSITION,
  DMCP_CMD_EXECUTE_CONSOLE,
  DMCP_CMD_PAUSE_GAME,
  DMCP_CMD_SET_TIMESCALE,
  DMCP_CMD_DAMAGE_ENTITY,
  DMCP_CMD_KILL_ENTITY,
} dmcp_command_type_t;

// Command flags
#define DMCP_CMD_FLAG_IMMEDIATE (1 << 0)  // Execute immediately, don't queue
#define DMCP_CMD_FLAG_RELIABLE (1 << 1)   // Require acknowledgment

// ============================================================================
// Command Structures
// ============================================================================

typedef struct {
  char        entity_class[128];  // e.g., "DoomImp", "Clip"
  dmcp_vec2_t position;
  float       angle;  // Degrees
  int32_t     tid;    // Thing ID (0 = auto-assign)
} dmcp_cmd_spawn_t;

typedef struct {
  char    map_name[32];  // e.g., "MAP01", "E1M1"
  int32_t skill_level;   // 1-5
  bool    reset_inventory;
} dmcp_cmd_change_level_t;

typedef struct {
  char    item_class[128];
  int32_t amount;
} dmcp_cmd_give_item_t;

typedef struct {
  float health;
} dmcp_cmd_set_health_t;

typedef struct {
  dmcp_vec2_t position;
  float       angle;
} dmcp_cmd_set_position_t;

typedef struct {
  char command[256];  // Console command
} dmcp_cmd_console_t;

typedef struct {
  bool paused;
} dmcp_cmd_pause_t;

typedef struct {
  float scale;  // 1.0 = normal, 0.5 = half speed, 2.0 = double
} dmcp_cmd_timescale_t;

typedef struct {
  int32_t target_tid;  // Target entity ID
  float   damage;
  char    damage_type[32];  // e.g., "Normal", "Fire", "Ice"
} dmcp_cmd_damage_t;

typedef struct {
  int32_t target_tid;
} dmcp_cmd_kill_t;

// ============================================================================
// Command Union
// ============================================================================

typedef struct {
  dmcp_command_type_t type;
  uint32_t            flags;
  uint64_t            sequence;  // For ordering/acknowledgment

  union {
    dmcp_cmd_spawn_t        spawn;
    dmcp_cmd_change_level_t change_level;
    dmcp_cmd_give_item_t    give_item;
    dmcp_cmd_set_health_t   set_health;
    dmcp_cmd_set_position_t set_position;
    dmcp_cmd_console_t      console;
    dmcp_cmd_pause_t        pause;
    dmcp_cmd_timescale_t    timescale;
    dmcp_cmd_damage_t       damage;
    dmcp_cmd_kill_t         kill;
  } data;
  uint8_t _reserved[16];
} dmcp_command_t;

typedef struct {
  uint64_t            sequence;
  dmcp_command_type_t command_type;
  bool                completed;
  bool                success;
  char                message[128];
} dmcp_command_result_t;

// ============================================================================
// Command Queue API
// ============================================================================

// Push a command from agent to game
DMCP_API mcp_result_generic_t dmcp_push_command(dmcp_context_t* ctx, dmcp_command_t* cmd);

// Pop a command for execution (call from game thread)
DMCP_API bool dmcp_pop_command(dmcp_context_t* ctx, dmcp_command_t* out_cmd);

// Check if commands are pending
DMCP_API bool dmcp_has_commands(const dmcp_context_t* ctx);

// Get number of pending commands
DMCP_API uint32_t dmcp_command_count(const dmcp_context_t* ctx);

// Mark command as queued for asynchronous completion tracking.
DMCP_API mcp_result_generic_t dmcp_command_result_mark_queued(dmcp_context_t*       ctx,
                                                              const dmcp_command_t* cmd);

// Mark command as completed with success/failure status.
DMCP_API mcp_result_generic_t dmcp_command_result_complete(dmcp_context_t*       ctx,
                                                           const dmcp_command_t* cmd, bool success,
                                                           const char* message);

// Retrieve tracked command execution result by sequence id.
DMCP_API mcp_result_generic_t dmcp_command_result_get(const dmcp_context_t* ctx, uint64_t sequence,
                                                      dmcp_command_result_t* out_result);

// Clear all pending commands
DMCP_API void dmcp_clear_commands(dmcp_context_t* ctx);

// ============================================================================
// JSON Command Parsing
// ============================================================================

// Parse a JSON-RPC command request into a command structure
// JSON format: {"type": "spawn_entity", "params": {"entity_class": "DoomImp",
// ...}}
DMCP_API mcp_result_generic_t dmcp_parse_command_json(const char* json, dmcp_command_t* out_cmd);

// Extended version with context for custom parser lookup
DMCP_API mcp_result_generic_t dmcp_parse_command_json_ex(dmcp_context_t* ctx, const char* json,
                                                         dmcp_command_t* out_cmd);

// Serialize command result to JSON
DMCP_API mcp_result_generic_t dmcp_format_command_result(const dmcp_command_t* cmd, bool success,
                                                         const char* message, char* buffer,
                                                         size_t buffer_size);

// ============================================================================
// Command Registration (for custom commands)
// ============================================================================

typedef mcp_result_generic_t (*dmcp_custom_command_parser_t)(const char*     json_params,
                                                             dmcp_command_t* out_cmd);

// Register a custom command parser
// This allows engines to extend the command system
DMCP_API mcp_result_generic_t dmcp_register_command_parser(dmcp_context_t*              ctx,
                                                           const char*                  type_name,
                                                           dmcp_command_type_t          type,
                                                           dmcp_custom_command_parser_t parser);

#ifdef __cplusplus
}
#endif
