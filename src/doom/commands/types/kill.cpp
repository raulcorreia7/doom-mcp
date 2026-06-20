#include "command_parsers.hpp"

#include <cstdint>
#include <limits>

#include "doom/commands/json_parsers.hpp"

namespace dmcp {

bool parse_kill_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_KILL_ENTITY;

  if (!has_only_fields(params, {"target_tid"})) {
    return false;
  }

  std::int64_t target_tid = 0;
  if (!read_required_int(params, {"target_tid"}, &target_tid) || target_tid < 0 ||
      target_tid > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  out->data.kill.target_tid = static_cast<std::int32_t>(target_tid);

  return true;
}

}  // namespace dmcp
