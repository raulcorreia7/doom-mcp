#include "command_parsers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

#include "doom/commands/json_parsers.hpp"
#include "dmcp/doom/constants.h"

namespace dmcp {

bool parse_damage_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_DAMAGE_ENTITY;

  std::int64_t target_tid = 0;
  if (!read_required_int(params, {"target_tid"}, &target_tid)) {
    return false;
  }

  if (target_tid < DMCP_TID_MIN) {
    return false;
  }
  out->data.damage.target_tid =
      std::clamp(static_cast<std::int32_t>(target_tid), DMCP_TID_MIN, DMCP_TID_MAX);

  double damage_dbl = 0.0;
  if (!read_required_number(params, {"damage"}, &damage_dbl)) {
    return false;
  }

  if (!std::isfinite(damage_dbl) || damage_dbl < static_cast<double>(DMCP_DAMAGE_MIN)) {
    return false;
  }

  int32_t damage;
  if (damage_dbl > static_cast<double>(std::numeric_limits<int32_t>::max())) {
    damage = DMCP_DAMAGE_MAX;
  } else {
    damage = std::clamp(static_cast<int32_t>(damage_dbl), DMCP_DAMAGE_MIN, DMCP_DAMAGE_MAX);
  }
  out->data.damage.damage = damage;

  std::string_view damage_type;
  if (!read_optional_string(params, {"damage_type"}, "Normal", &damage_type) ||
      !copy_checked_string(out->data.damage.damage_type, sizeof(out->data.damage.damage_type),
                           damage_type)) {
    return false;
  }

  return true;
}

}  // namespace dmcp
