#include "dmcp_adapter.h"

#include <limits.h>
#include <math.h>

#include "dmcp_adapter_command_queue.h"
#include "dmcp/adapter/utils.h"
#include "dmcp/doom/commands.h"

#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_fixed.h"

extern fixed_t forwardmove[2];
extern fixed_t sidemove[2];
extern fixed_t angleturn[3];

#define DMCP_SCHAR_MAX 127
#define DMCP_SCHAR_MIN (-128)
#define DMCP_SHORT_MAX 32767
#define DMCP_SHORT_MIN (-32768)

static float dmcp_angle_to_degrees(angle_t angle) { return (float)angle * (360.0f / 65536.0f); }

static signed char dmcp_clamp_to_schar(fixed_t value) {
  if (value > DMCP_SCHAR_MAX) return DMCP_SCHAR_MAX;
  if (value < DMCP_SCHAR_MIN) return DMCP_SCHAR_MIN;
  return (signed char)value;
}

static short dmcp_clamp_to_short(fixed_t value) {
  if (value > DMCP_SHORT_MAX) return DMCP_SHORT_MAX;
  if (value < DMCP_SHORT_MIN) return DMCP_SHORT_MIN;
  return (short)value;
}

static short dmcp_degrees_to_angleturn_clamped(float degrees) {
  if (!isfinite(degrees)) return 0;
  float result = degrees * (65536.0f / 360.0f);
  if (result > (float)DMCP_SHORT_MAX) return DMCP_SHORT_MAX;
  if (result < (float)DMCP_SHORT_MIN) return DMCP_SHORT_MIN;
  return (short)result;
}

static float normalize_angle_delta(float delta) {
  if (!isfinite(delta)) return 0.0f;
  delta = fmodf(delta, 360.0f);
  if (delta > 180.0f) delta -= 360.0f;
  if (delta < -180.0f) delta += 360.0f;
  return delta;
}

bool dmcp_crispy_input_execute(dmcp_crispy_t* ctx, const dmcp_command_t* cmd) {
  player_t* player;
  ticcmd_t* ticcmd;
  float     target_angle;
  float     current_angle;
  float     delta;
  float     turn_speed;

  if (!ctx || !cmd || cmd->type != DMCP_CMD_PLAYER_INPUT) {
    return false;
  }

  player = dmcp_get_player();
  if (!player || !player->mo) {
    return false;
  }

  ticcmd = &player->cmd;

  switch (cmd->data.input.action) {
    case DMCP_INPUT_FORWARD:
      ticcmd->forwardmove = dmcp_clamp_to_schar(forwardmove[0]);
      break;

    case DMCP_INPUT_BACKWARD:
      ticcmd->forwardmove = dmcp_clamp_to_schar(-forwardmove[0]);
      break;

    case DMCP_INPUT_STRAFE_LEFT:
      ticcmd->sidemove = dmcp_clamp_to_schar(-sidemove[0]);
      break;

    case DMCP_INPUT_STRAFE_RIGHT:
      ticcmd->sidemove = dmcp_clamp_to_schar(sidemove[0]);
      break;

    case DMCP_INPUT_TURN_LEFT:
      ticcmd->angleturn = dmcp_clamp_to_short(angleturn[2]);
      break;

    case DMCP_INPUT_TURN_RIGHT:
      ticcmd->angleturn = dmcp_clamp_to_short(-angleturn[2]);
      break;

    case DMCP_INPUT_AIM:
      target_angle  = cmd->data.input.aim_angle;
      current_angle = dmcp_angle_to_degrees(player->mo->angle);
      delta         = normalize_angle_delta(target_angle - current_angle);
      turn_speed    = dmcp_angle_to_degrees((angle_t)angleturn[2] << 16);
      if (delta > turn_speed) {
        ticcmd->angleturn = dmcp_clamp_to_short(-angleturn[2]);
      } else if (delta < -turn_speed) {
        ticcmd->angleturn = dmcp_clamp_to_short(angleturn[2]);
      } else {
        ticcmd->angleturn = dmcp_degrees_to_angleturn_clamped(delta);
      }
      break;

    case DMCP_INPUT_ATTACK:
      ticcmd->buttons |= BT_ATTACK;
      break;

    case DMCP_INPUT_USE:
      ticcmd->buttons |= BT_USE;
      break;

    case DMCP_INPUT_WEAPON:
      if (cmd->data.input.weapon_slot >= 1 && cmd->data.input.weapon_slot <= 7) {
        ticcmd->buttons |= BT_CHANGE;
        ticcmd->buttons |= (byte)((cmd->data.input.weapon_slot - 1) << BT_WEAPONSHIFT);
      }
      break;

    default:
      return false;
  }

  return true;
}

static bool crispy_execute_input_for_queue(void* adapter_ctx, const dmcp_command_t* cmd,
                                           char* out_message, size_t out_message_size) {
  dmcp_crispy_t* ctx;
  bool           success;

  if (!adapter_ctx || !cmd || cmd->type != DMCP_CMD_PLAYER_INPUT) {
    dmcp_strcpy_safe(out_message, "Invalid player input", out_message_size);
    return false;
  }

  ctx     = (dmcp_crispy_t*)adapter_ctx;
  success = dmcp_crispy_input_execute(ctx, cmd);
  if (!success) {
    dmcp_strcpy_safe(out_message, "Input failed in engine", out_message_size);
  }
  return success;
}

void dmcp_crispy_inputs_process(dmcp_crispy_t* ctx) {
  if (!ctx || !dmcp_crispy_get_context(ctx)) {
    return;
  }

  // Process one input per tick to preserve current Crispy control semantics.
  dmcp_adapter_process_input_queue(dmcp_crispy_get_context(ctx), ctx, crispy_execute_input_for_queue,
                                   1);
}
