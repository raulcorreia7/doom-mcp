#include "parsers.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

#include "doom/commands/parsers.hpp"

namespace dmcp {

bool parse_spawn_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_SPAWN_ENTITY;

  std::string_view entity_class;
  if (!read_required_string(params, {"entity_class", "entity", "class"}, &entity_class) ||
      !copy_checked_string(out->data.spawn.entity_class, sizeof(out->data.spawn.entity_class),
                           entity_class)) {
    return false;
  }

  json_value position     = params["position"];
  json_value coord_source = params;
  if (params.has_member("position")) {
    if (!position.is_object()) {
      return false;
    }
    coord_source = position;
  }

  double x     = 0.0;
  double y     = 0.0;
  double angle = 0.0;
  if (!read_required_number(coord_source, {"x"}, &x) ||
      !read_required_number(coord_source, {"y"}, &y) || !std::isfinite(x) || !std::isfinite(y)) {
    return false;
  }
  if (!read_optional_number(params, {"angle"}, 0.0, &angle) || !std::isfinite(angle)) {
    return false;
  }

  std::int64_t tid = 0;
  if (!read_optional_int(params, {"tid"}, 0, &tid) || tid < 0 ||
      tid > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }

  out->data.spawn.position.x = static_cast<float>(x);
  out->data.spawn.position.y = static_cast<float>(y);
  out->data.spawn.angle      = static_cast<float>(angle);
  out->data.spawn.tid        = static_cast<std::int32_t>(tid);

  return true;
}

}  // namespace dmcp
