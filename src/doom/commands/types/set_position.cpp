#include "command_parsers.hpp"

#include "doom/commands/json_parsers.hpp"

namespace dmcp {

bool parse_set_position_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_SET_PLAYER_POSITION;

  position_coords pos;
  if (!parse_position_coords(params, &pos)) {
    return false;
  }

  out->data.set_position.position.x = static_cast<float>(pos.x);
  out->data.set_position.position.y = static_cast<float>(pos.y);
  out->data.set_position.angle      = static_cast<float>(pos.angle);

  return true;
}

}  // namespace dmcp
