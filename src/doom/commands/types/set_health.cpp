#include "command_parsers.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "doom/commands/json_parsers.hpp"
#include "dmcp/doom/constants.h"

namespace dmcp {

bool parse_set_health_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_SET_PLAYER_HEALTH;

  if (!has_only_fields(params, {"health"})) {
    return false;
  }

  double health = 0.0;
  if (!read_required_number(params, {"health"}, &health)) {
    return false;
  }

  if (!std::isfinite(health) || health <= 0.0) {
    return false;
  }

  int32_t clamped_health;
  if (health > static_cast<double>(std::numeric_limits<int32_t>::max())) {
    clamped_health = DMCP_PLAYER_MAX_HEALTH;
  } else if (health < static_cast<double>(std::numeric_limits<int32_t>::min())) {
    clamped_health = 1;
  } else {
    clamped_health = std::clamp(static_cast<int32_t>(health), 1, DMCP_PLAYER_MAX_HEALTH);
  }

  out->data.set_health.health = clamped_health;
  return true;
}

}  // namespace dmcp
