#include "parsers.hpp"

#include <cstdint>
#include <limits>
#include <string_view>

#include "commands/parsers.hpp"

namespace dmcp {

bool parse_change_level_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_CHANGE_LEVEL;

  std::string_view map_name;
  if (!read_required_string(params, {"map_name", "level"}, &map_name) ||
      !is_valid_map_name(map_name) ||
      !copy_checked_string(out->data.change_level.map_name, sizeof(out->data.change_level.map_name),
                           map_name)) {
    return false;
  }

  std::int64_t skill_level = 3;
  if (!read_optional_int(params, {"skill_level"}, 3, &skill_level) || skill_level < 1 ||
      skill_level > 5) {
    return false;
  }

  bool reset_inventory = false;
  if (!read_optional_bool(params, {"reset_inventory"}, false, &reset_inventory)) {
    return false;
  }

  out->data.change_level.skill_level     = static_cast<std::int32_t>(skill_level);
  out->data.change_level.reset_inventory = reset_inventory;

  return true;
}

}  // namespace dmcp
