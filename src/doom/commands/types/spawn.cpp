#include "command_parsers.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

#include "doom/commands/json_parsers.hpp"

namespace dmcp {

bool parse_spawn_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_SPAWN_ENTITY;

  if (!has_only_fields(params, {"entity_class", "x", "y", "angle", "tid"})) {
    return false;
  }

  std::string_view entity_class;
  if (!read_required_string(params, {"entity_class"}, &entity_class) ||
      !copy_checked_string(out->data.spawn.entity_class, sizeof(out->data.spawn.entity_class),
                           entity_class)) {
    return false;
  }

  position_coords pos;
  if (!parse_position_coords(params, &pos)) {
    return false;
  }

  std::int64_t tid = 0;
  if (!read_optional_int(params, {"tid"}, 0, &tid) || tid < 0 ||
      tid > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }

  out->data.spawn.position.x = static_cast<float>(pos.x);
  out->data.spawn.position.y = static_cast<float>(pos.y);
  out->data.spawn.angle      = static_cast<float>(pos.angle);
  out->data.spawn.tid        = static_cast<std::int32_t>(tid);

  return true;
}

}  // namespace dmcp
