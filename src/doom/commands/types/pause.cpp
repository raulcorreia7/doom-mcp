#include "command_parsers.hpp"

#include "doom/commands/json_parsers.hpp"

namespace dmcp {

bool parse_pause_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_PAUSE_GAME;

  if (!has_only_fields(params, {"paused"})) {
    return false;
  }

  bool paused = false;
  if (!read_required_bool(params, {"paused"}, &paused)) {
    return false;
  }
  out->data.pause.paused = paused;

  return true;
}

}  // namespace dmcp
