#include "parsers.hpp"

#include <cmath>

#include "doom/commands/parsers.hpp"

namespace dmcp {

bool parse_set_position_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_SET_PLAYER_POSITION;

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

  out->data.set_position.position.x = static_cast<float>(x);
  out->data.set_position.position.y = static_cast<float>(y);
  out->data.set_position.angle      = static_cast<float>(angle);

  return true;
}

}  // namespace dmcp
