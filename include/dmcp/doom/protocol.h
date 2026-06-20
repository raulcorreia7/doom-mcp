#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MCP Tool Names
// ============================================================================

#define DMCP_TOOL_GET_PLAYER "get_player"
#define DMCP_TOOL_GET_ENEMIES "get_enemies"
#define DMCP_TOOL_GET_ENTITIES "get_entities"
#define DMCP_TOOL_GET_ITEMS "get_items"
#define DMCP_TOOL_GET_MAP "get_map"
#define DMCP_TOOL_GET_INVENTORY "get_inventory"
#define DMCP_TOOL_GET_GAME_INFO "get_game_info"
#define DMCP_TOOL_GET_STATE "get_state"
#define DMCP_TOOL_GET_STATE_BATCH "get_state_batch"
#define DMCP_TOOL_GET_SCREENSHOT "get_screenshot"
#define DMCP_TOOL_GET_COMMAND_RESULT "get_command_result"
#define DMCP_TOOL_GET_AVAILABLE_CONTENT "get_available_content"
#define DMCP_TOOL_GET_AVAILABLE_ENEMIES "get_available_enemies"
#define DMCP_TOOL_GET_AVAILABLE_ENTITIES "get_available_entities"
#define DMCP_TOOL_GET_AVAILABLE_ITEMS "get_available_items"
#define DMCP_TOOL_GET_AVAILABLE_WEAPONS "get_available_weapons"
#define DMCP_TOOL_GET_AVAILABLE_AMMO "get_available_ammo"
#define DMCP_TOOL_GET_AVAILABLE_KEYS "get_available_keys"
#define DMCP_TOOL_GET_AVAILABLE_MAPS "get_available_maps"
#define DMCP_TOOL_GET_AVAILABLE_GIVEABLE "get_available_giveable"
#define DMCP_TOOL_EXECUTE_BATCH "execute_batch"
#define DMCP_TOOL_GET_COMMAND_EXAMPLES "get_command_examples"
#define DMCP_TOOL_PLAYER_INPUT "player_input"

// Direct command tool names. They are invoked through MCP tools/call with
// structured arguments and are mapped to internal command queue types by DMCP.
#define DMCP_TOOL_SPAWN_ENTITY "spawn_entity"
#define DMCP_TOOL_CHANGE_LEVEL "change_level"
#define DMCP_TOOL_GIVE_ITEM "give_item"
#define DMCP_TOOL_SET_PLAYER_HEALTH "set_player_health"
#define DMCP_TOOL_SET_PLAYER_POSITION "set_player_position"
#define DMCP_TOOL_EXECUTE_CONSOLE "execute_console"
#define DMCP_TOOL_PAUSE_GAME "pause_game"
#define DMCP_TOOL_DAMAGE_ENTITY "damage_entity"
#define DMCP_TOOL_KILL_ENTITY "kill_entity"

// ============================================================================
// Command Type Names (JSON "type" field values)
// ============================================================================

#define DMCP_CMD_NAME_SPAWN_ENTITY "spawn_entity"
#define DMCP_CMD_NAME_CHANGE_LEVEL "change_level"
#define DMCP_CMD_NAME_GIVE_ITEM "give_item"
#define DMCP_CMD_NAME_SET_PLAYER_HEALTH "set_player_health"
#define DMCP_CMD_NAME_SET_PLAYER_POSITION "set_player_position"
#define DMCP_CMD_NAME_EXECUTE_CONSOLE "execute_console"
#define DMCP_CMD_NAME_PAUSE_GAME "pause_game"
#define DMCP_CMD_NAME_DAMAGE_ENTITY "damage_entity"
#define DMCP_CMD_NAME_KILL_ENTITY "kill_entity"
#define DMCP_CMD_NAME_PLAYER_INPUT "player_input"

// ============================================================================
// Input Action Names (JSON "action" field values)
// ============================================================================

#define DMCP_INPUT_NAME_FORWARD "forward"
#define DMCP_INPUT_NAME_BACKWARD "backward"
#define DMCP_INPUT_NAME_STRAFE_LEFT "strafe_left"
#define DMCP_INPUT_NAME_STRAFE_RIGHT "strafe_right"
#define DMCP_INPUT_NAME_TURN_LEFT "turn_left"
#define DMCP_INPUT_NAME_TURN_RIGHT "turn_right"
#define DMCP_INPUT_NAME_AIM "aim"
#define DMCP_INPUT_NAME_ATTACK "attack"
#define DMCP_INPUT_NAME_USE "use"
#define DMCP_INPUT_NAME_WEAPON "weapon"

// ============================================================================
// JSON Field Names
// ============================================================================

#define DMCP_FIELD_ACTION "action"
#define DMCP_FIELD_VALUE "value"
#define DMCP_FIELD_TYPE "type"
#define DMCP_FIELD_PARAMS "params"
#define DMCP_FIELD_NAME "name"
#define DMCP_FIELD_ARGUMENTS "arguments"

// ============================================================================
// State Section Names
// ============================================================================

#define DMCP_SECTION_PLAYER "player"
#define DMCP_SECTION_ENEMIES "enemies"
#define DMCP_SECTION_ENTITIES "entities"
#define DMCP_SECTION_ITEMS "items"
#define DMCP_SECTION_MAP "map"
#define DMCP_SECTION_INVENTORY "inventory"
#define DMCP_SECTION_GAME "game"

// ============================================================================
// Entity Status Filter Values
// ============================================================================

#define DMCP_STATUS_ALIVE "alive"
#define DMCP_STATUS_DEAD "dead"
#define DMCP_STATUS_ALL "all"

// ============================================================================
// Entity Kind Filter Values
// ============================================================================

#define DMCP_KIND_ALL "all"
#define DMCP_KIND_ENEMY "enemy"
#define DMCP_KIND_ITEM "item"
#define DMCP_KIND_WEAPON "weapon"
#define DMCP_KIND_AMMO "ammo"
#define DMCP_KIND_KEY "key"
#define DMCP_KIND_HEALTH "health"
#define DMCP_KIND_ARMOR "armor"
#define DMCP_KIND_POWERUP "powerup"
#define DMCP_KIND_WORLD "world"

// ============================================================================
// Game Mode Names
// ============================================================================

#define DMCP_MODE_SHAREWARE "shareware"
#define DMCP_MODE_REGISTERED "registered"
#define DMCP_MODE_COMMERCIAL "commercial"
#define DMCP_MODE_DOOM2 "doom2"
#define DMCP_MODE_RETAIL "retail"
#define DMCP_MODE_ULTIMATE "ultimate"

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <string_view>

namespace dmcp {
namespace tools {

constexpr std::string_view get_player             = DMCP_TOOL_GET_PLAYER;
constexpr std::string_view get_enemies            = DMCP_TOOL_GET_ENEMIES;
constexpr std::string_view get_entities           = DMCP_TOOL_GET_ENTITIES;
constexpr std::string_view get_items              = DMCP_TOOL_GET_ITEMS;
constexpr std::string_view get_map                = DMCP_TOOL_GET_MAP;
constexpr std::string_view get_inventory          = DMCP_TOOL_GET_INVENTORY;
constexpr std::string_view get_game_info          = DMCP_TOOL_GET_GAME_INFO;
constexpr std::string_view get_state              = DMCP_TOOL_GET_STATE;
constexpr std::string_view get_state_batch        = DMCP_TOOL_GET_STATE_BATCH;
constexpr std::string_view get_screenshot         = DMCP_TOOL_GET_SCREENSHOT;
constexpr std::string_view get_command_result     = DMCP_TOOL_GET_COMMAND_RESULT;
constexpr std::string_view get_available_content  = DMCP_TOOL_GET_AVAILABLE_CONTENT;
constexpr std::string_view get_available_enemies  = DMCP_TOOL_GET_AVAILABLE_ENEMIES;
constexpr std::string_view get_available_entities = DMCP_TOOL_GET_AVAILABLE_ENTITIES;
constexpr std::string_view get_available_items    = DMCP_TOOL_GET_AVAILABLE_ITEMS;
constexpr std::string_view get_available_weapons  = DMCP_TOOL_GET_AVAILABLE_WEAPONS;
constexpr std::string_view get_available_ammo     = DMCP_TOOL_GET_AVAILABLE_AMMO;
constexpr std::string_view get_available_keys     = DMCP_TOOL_GET_AVAILABLE_KEYS;
constexpr std::string_view get_available_maps     = DMCP_TOOL_GET_AVAILABLE_MAPS;
constexpr std::string_view get_available_giveable = DMCP_TOOL_GET_AVAILABLE_GIVEABLE;
constexpr std::string_view execute_batch          = DMCP_TOOL_EXECUTE_BATCH;
constexpr std::string_view get_command_examples   = DMCP_TOOL_GET_COMMAND_EXAMPLES;
constexpr std::string_view player_input           = DMCP_TOOL_PLAYER_INPUT;

constexpr std::string_view spawn_entity        = DMCP_TOOL_SPAWN_ENTITY;
constexpr std::string_view change_level        = DMCP_TOOL_CHANGE_LEVEL;
constexpr std::string_view give_item           = DMCP_TOOL_GIVE_ITEM;
constexpr std::string_view set_player_health   = DMCP_TOOL_SET_PLAYER_HEALTH;
constexpr std::string_view set_player_position = DMCP_TOOL_SET_PLAYER_POSITION;
constexpr std::string_view execute_console     = DMCP_TOOL_EXECUTE_CONSOLE;
constexpr std::string_view pause_game          = DMCP_TOOL_PAUSE_GAME;
constexpr std::string_view damage_entity       = DMCP_TOOL_DAMAGE_ENTITY;
constexpr std::string_view kill_entity         = DMCP_TOOL_KILL_ENTITY;

}  // namespace tools

namespace cmds {

constexpr std::string_view spawn_entity        = DMCP_CMD_NAME_SPAWN_ENTITY;
constexpr std::string_view change_level        = DMCP_CMD_NAME_CHANGE_LEVEL;
constexpr std::string_view give_item           = DMCP_CMD_NAME_GIVE_ITEM;
constexpr std::string_view set_player_health   = DMCP_CMD_NAME_SET_PLAYER_HEALTH;
constexpr std::string_view set_player_position = DMCP_CMD_NAME_SET_PLAYER_POSITION;
constexpr std::string_view execute_console     = DMCP_CMD_NAME_EXECUTE_CONSOLE;
constexpr std::string_view pause_game          = DMCP_CMD_NAME_PAUSE_GAME;
constexpr std::string_view damage_entity       = DMCP_CMD_NAME_DAMAGE_ENTITY;
constexpr std::string_view kill_entity         = DMCP_CMD_NAME_KILL_ENTITY;
constexpr std::string_view player_input        = DMCP_CMD_NAME_PLAYER_INPUT;

}  // namespace cmds

namespace input {

constexpr std::string_view forward      = DMCP_INPUT_NAME_FORWARD;
constexpr std::string_view backward     = DMCP_INPUT_NAME_BACKWARD;
constexpr std::string_view strafe_left  = DMCP_INPUT_NAME_STRAFE_LEFT;
constexpr std::string_view strafe_right = DMCP_INPUT_NAME_STRAFE_RIGHT;
constexpr std::string_view turn_left    = DMCP_INPUT_NAME_TURN_LEFT;
constexpr std::string_view turn_right   = DMCP_INPUT_NAME_TURN_RIGHT;
constexpr std::string_view aim          = DMCP_INPUT_NAME_AIM;
constexpr std::string_view attack       = DMCP_INPUT_NAME_ATTACK;
constexpr std::string_view use          = DMCP_INPUT_NAME_USE;
constexpr std::string_view weapon       = DMCP_INPUT_NAME_WEAPON;

}  // namespace input

namespace section {

constexpr std::string_view player    = DMCP_SECTION_PLAYER;
constexpr std::string_view enemies   = DMCP_SECTION_ENEMIES;
constexpr std::string_view entities  = DMCP_SECTION_ENTITIES;
constexpr std::string_view items     = DMCP_SECTION_ITEMS;
constexpr std::string_view map       = DMCP_SECTION_MAP;
constexpr std::string_view inventory = DMCP_SECTION_INVENTORY;
constexpr std::string_view game      = DMCP_SECTION_GAME;

}  // namespace section

namespace status {

constexpr std::string_view alive = DMCP_STATUS_ALIVE;
constexpr std::string_view dead  = DMCP_STATUS_DEAD;
constexpr std::string_view all   = DMCP_STATUS_ALL;

}  // namespace status

namespace kind {

constexpr std::string_view all     = DMCP_KIND_ALL;
constexpr std::string_view enemy   = DMCP_KIND_ENEMY;
constexpr std::string_view item    = DMCP_KIND_ITEM;
constexpr std::string_view weapon  = DMCP_KIND_WEAPON;
constexpr std::string_view ammo    = DMCP_KIND_AMMO;
constexpr std::string_view key     = DMCP_KIND_KEY;
constexpr std::string_view health  = DMCP_KIND_HEALTH;
constexpr std::string_view armor   = DMCP_KIND_ARMOR;
constexpr std::string_view powerup = DMCP_KIND_POWERUP;
constexpr std::string_view world   = DMCP_KIND_WORLD;

}  // namespace kind

namespace mode {

constexpr std::string_view shareware  = DMCP_MODE_SHAREWARE;
constexpr std::string_view registered = DMCP_MODE_REGISTERED;
constexpr std::string_view commercial = DMCP_MODE_COMMERCIAL;
constexpr std::string_view doom2      = DMCP_MODE_DOOM2;
constexpr std::string_view retail     = DMCP_MODE_RETAIL;
constexpr std::string_view ultimate   = DMCP_MODE_ULTIMATE;

}  // namespace mode

}  // namespace dmcp

#endif  // __cplusplus
