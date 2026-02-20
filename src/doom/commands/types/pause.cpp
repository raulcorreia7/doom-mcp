#include "parsers.hpp"

#include "commands/parsers.hpp"

namespace dmcp {

bool parse_pause_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_PAUSE_GAME;

  bool paused = false;
  if (!read_required_bool(params, {"paused", "pause"}, &paused)) {
    return false;
  }
  out->data.pause.paused = paused;

  return true;
}

}  // namespace dmcp
