#include "command_parsers.hpp"

#include <string_view>

#include "doom/commands/json_parsers.hpp"

namespace dmcp {

bool parse_console_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_EXECUTE_CONSOLE;

  std::string_view command;
  if (!read_required_string(params, {"command"}, &command) ||
      !copy_checked_string(out->data.console.command, sizeof(out->data.console.command), command)) {
    return false;
  }

  return true;
}

}  // namespace dmcp
