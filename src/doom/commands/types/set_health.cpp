#include "parsers.hpp"

#include "doom/commands/parsers.hpp"
#include "dmcp/doom/constants.h"

namespace dmcp {

bool parse_set_health_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_SET_PLAYER_HEALTH;

  double health = 0.0;
  if (!read_required_number(params, {"health", "value"}, &health)) {
    return false;
  }

  if (health <= 0.0) {
    return false;
  }

  out->data.set_health.health =
      clamp_value(static_cast<int32_t>(health), 1, DMCP_PLAYER_MAX_HEALTH);

  return true;
}

}  // namespace dmcp
