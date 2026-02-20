#include "parsers.hpp"

#include <cmath>

#include "doom/commands/parsers.hpp"
#include "dmcp/doom/constants.h"

namespace dmcp {

bool parse_timescale_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_SET_TIMESCALE;

  double scale = 0.0;
  if (!read_required_number(params, {"scale"}, &scale) || !std::isfinite(scale) || scale <= 0.0) {
    return false;
  }

  out->data.timescale.scale =
      static_cast<float>(clamp_value(scale, DMCP_TIMESCALE_MIN, DMCP_TIMESCALE_MAX));

  return true;
}

}  // namespace dmcp
