#pragma once

#include "dmcp/doom/commands.h"
#include "doom/internal/json_types.hpp"

namespace dmcp {

// Command type parsers - each returns true on success, false on failure
bool parse_spawn_command(const json_value& params, dmcp_command_t* out);
bool parse_change_level_command(const json_value& params, dmcp_command_t* out);
bool parse_give_item_command(const json_value& params, dmcp_command_t* out);
bool parse_set_health_command(const json_value& params, dmcp_command_t* out);
bool parse_set_position_command(const json_value& params, dmcp_command_t* out);
bool parse_pause_command(const json_value& params, dmcp_command_t* out);
bool parse_timescale_command(const json_value& params, dmcp_command_t* out);
bool parse_damage_command(const json_value& params, dmcp_command_t* out);
bool parse_kill_command(const json_value& params, dmcp_command_t* out);
bool parse_console_command(const json_value& params, dmcp_command_t* out);
bool parse_player_input_command(const json_value& params, dmcp_command_t* out);

}  // namespace dmcp
