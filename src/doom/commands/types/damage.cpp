#include "parsers.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

#include "doom/commands/parsers.hpp"

namespace dmcp {

bool parse_damage_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_DAMAGE_ENTITY;

  std::int64_t target_tid = 0;
  if (!read_required_int(params, {"target_tid"}, &target_tid) || target_tid < 0 ||
      target_tid > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }

  double damage = 0.0;
  if (!read_required_number(params, {"damage"}, &damage) || !std::isfinite(damage) ||
      damage <= 0.0 || damage > 10000.0) {
    return false;
  }

  std::string_view damage_type;
  if (!read_optional_string(params, {"damage_type"}, "Normal", &damage_type) ||
      !copy_checked_string(out->data.damage.damage_type, sizeof(out->data.damage.damage_type),
                           damage_type)) {
    return false;
  }

  out->data.damage.target_tid = static_cast<std::int32_t>(target_tid);
  out->data.damage.damage     = static_cast<float>(damage);

  return true;
}

}  // namespace dmcp
