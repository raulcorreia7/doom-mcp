#include "parsers.hpp"

#include <cmath>

#include "commands/parsers.hpp"

namespace dmcp {

bool parse_set_health_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_SET_PLAYER_HEALTH;

  double health = 0.0;
  if (!read_required_number(params, {"health", "value"}, &health) || !std::isfinite(health) ||
      health <= 0.0 || health > 200.0) {
    return false;
  }
  out->data.set_health.health = static_cast<float>(health);

  return true;
}

}  // namespace dmcp
