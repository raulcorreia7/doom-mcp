#include "parsers.hpp"

#include <cstdint>
#include <string_view>

#include "doom/commands/parsers.hpp"
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
      clamp_value(static_cast<std::int32_t>(target_tid), DMCP_TID_MIN, DMCP_TID_MAX);

  std::int64_t damage = 0;
  if (!read_required_int(params, {"damage"}, &damage)) {
    return false;
  }

  if (damage < DMCP_DAMAGE_MIN) {
    return false;
  }
  out->data.damage.damage =
      clamp_value(static_cast<std::int32_t>(damage), DMCP_DAMAGE_MIN, DMCP_DAMAGE_MAX);

  std::string_view damage_type;
  if (!read_optional_string(params, {"damage_type"}, "Normal", &damage_type) ||
      !copy_checked_string(out->data.damage.damage_type, sizeof(out->data.damage.damage_type),
                           damage_type)) {
    return false;
  }

  return true;
}

}  // namespace dmcp
