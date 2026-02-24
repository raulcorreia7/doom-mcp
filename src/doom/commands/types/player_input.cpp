#include "command_parsers.hpp"

#include <cmath>
#include <cstdint>
#include <string_view>

#include "doom/commands/json_parsers.hpp"

namespace dmcp {

static dmcp_player_input_action_t parse_action_string(std::string_view action_str) {
  if (action_str == "fwd" || action_str == "forward") return DMCP_INPUT_FORWARD;
  if (action_str == "back" || action_str == "backward") return DMCP_INPUT_BACKWARD;
  if (action_str == "left" || action_str == "strafe_left") return DMCP_INPUT_STRAFE_LEFT;
  if (action_str == "right" || action_str == "strafe_right") return DMCP_INPUT_STRAFE_RIGHT;
  if (action_str == "tleft" || action_str == "turn_left") return DMCP_INPUT_TURN_LEFT;
  if (action_str == "tright" || action_str == "turn_right") return DMCP_INPUT_TURN_RIGHT;
  if (action_str == "aim") return DMCP_INPUT_AIM;
  if (action_str == "atk" || action_str == "attack") return DMCP_INPUT_ATTACK;
  if (action_str == "use") return DMCP_INPUT_USE;
  if (action_str == "wpn" || action_str == "weapon") return DMCP_INPUT_WEAPON;
  return DMCP_INPUT_NONE;
}

static float normalize_angle_degrees(double angle) {
  if (!std::isfinite(angle)) return 0.0f;
  angle = std::fmod(angle, 360.0);
  if (angle < 0.0) angle += 360.0;
  return static_cast<float>(angle);
}

bool parse_player_input_command(const json_value& params, dmcp_command_t* out) {
  out->type                   = DMCP_CMD_PLAYER_INPUT;
  out->data.input.action      = DMCP_INPUT_NONE;
  out->data.input.aim_angle   = 0.0f;
  out->data.input.weapon_slot = 0;

  std::string_view action_str;
  if (read_optional_string(params, {"a", "action"}, "", &action_str)) {
    out->data.input.action = parse_action_string(action_str);
  }

  if (out->data.input.action == DMCP_INPUT_NONE) {
    return false;
  }

  if (out->data.input.action == DMCP_INPUT_AIM) {
    double angle = 0.0;
    if (!read_required_number(params, {"v", "angle", "degrees"}, &angle)) {
      return false;
    }
    out->data.input.aim_angle = normalize_angle_degrees(angle);
  }

  if (out->data.input.action == DMCP_INPUT_WEAPON) {
    std::int64_t slot = 0;
    if (!read_optional_int(params, {"v", "slot", "weapon"}, 1, &slot)) {
      return false;
    }
    if (slot < 1 || slot > 7) {
      return false;
    }
    out->data.input.weapon_slot = static_cast<int32_t>(slot);
  }

  return true;
}

}  // namespace dmcp
