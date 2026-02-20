#include "parsers.hpp"

#include <cmath>

#include "commands/parsers.hpp"

namespace dmcp {

bool parse_timescale_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_SET_TIMESCALE;

  double scale = 0.0;
  if (!read_required_number(params, {"scale"}, &scale) || !std::isfinite(scale) || scale <= 0.0 ||
      scale > 10.0) {
    return false;
  }
  out->data.timescale.scale = static_cast<float>(scale);

  return true;
}

}  // namespace dmcp
