#include "dmcp_template.h"

#include <cstring>
#include <memory>
#include <mutex>
#include <new>

#include "dmcp/adapter/utils.h"
#include "dmcp_adapter_command_queue.h"
#include "mcp/core/memory.h"

struct dmcp_template_s {
  dmcp_context_t*        dmcp_ctx = nullptr;
  dmcp_template_config_t config{};
  mutable std::mutex     callback_mutex;
};

namespace {

constexpr const char* kFallbackMode      = "template";
constexpr const char* kFallbackVersion   = "template-0.1";
constexpr const char* kFallbackLevelId   = "TEMPLATE";
constexpr const char* kFallbackLevelName = "Adapter Template";
constexpr const char* kCommandMissing    = "Command callback not configured";
constexpr const char* kInputMissing      = "Input callback not configured";

dmcp_template_config_t CopyConfig(const dmcp_template_config_t* config) {
  dmcp_template_config_t effective = dmcp_template_config_default();
  if (config == nullptr) {
    return effective;
  }

  size_t copy_size = config->struct_size;
  if (copy_size == 0 || copy_size > sizeof(dmcp_template_config_t)) {
    copy_size = sizeof(dmcp_template_config_t);
  }

  mcp_memcpy_safe(&effective, sizeof(effective), config, copy_size);
  effective.struct_size = sizeof(dmcp_template_config_t);
  return effective;
}

void FillFallbackSnapshot(dmcp_snapshot_t* snapshot) {
  dmcp_snapshot_clear(snapshot);
  mcp_strcpy_safe(snapshot->game.mode, sizeof(snapshot->game.mode), kFallbackMode);
  mcp_strcpy_safe(snapshot->game.version, sizeof(snapshot->game.version), kFallbackVersion);
  mcp_strcpy_safe(snapshot->level.level_id, sizeof(snapshot->level.level_id), kFallbackLevelId);
  mcp_strcpy_safe(snapshot->level.level_name, sizeof(snapshot->level.level_name),
                  kFallbackLevelName);
  mcp_strcpy_safe(snapshot->level.gamestate, sizeof(snapshot->level.gamestate), "template");
  mcp_strcpy_safe(snapshot->player.playerstate, sizeof(snapshot->player.playerstate), "unknown");
}

void SnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
  if (user_data == nullptr || snapshot == nullptr) {
    return;
  }

  auto*                       adapter = static_cast<dmcp_template_t*>(user_data);
  std::lock_guard<std::mutex> lock(adapter->callback_mutex);

  if (adapter->config.fill_snapshot != nullptr &&
      adapter->config.fill_snapshot(adapter->config.engine_user, snapshot)) {
    return;
  }

  FillFallbackSnapshot(snapshot);
}

bool ExecuteCommandCallback(void* adapter_ctx, const dmcp_command_t* command, char* out_message,
                            size_t out_message_size) {
  auto* adapter = static_cast<dmcp_template_t*>(adapter_ctx);
  if (adapter == nullptr || command == nullptr) {
    mcp_strcpy_safe(out_message, out_message_size, "Invalid command");
    return false;
  }

  std::lock_guard<std::mutex> lock(adapter->callback_mutex);
  if (adapter->config.execute_command == nullptr) {
    mcp_strcpy_safe(out_message, out_message_size, kCommandMissing);
    return false;
  }

  return adapter->config.execute_command(adapter->config.engine_user, command, out_message,
                                         out_message_size);
}

bool ExecuteInputCallback(void* adapter_ctx, const dmcp_command_t* command, char* out_message,
                          size_t out_message_size) {
  auto* adapter = static_cast<dmcp_template_t*>(adapter_ctx);
  if (adapter == nullptr || command == nullptr || command->type != DMCP_CMD_PLAYER_INPUT) {
    mcp_strcpy_safe(out_message, out_message_size, "Invalid input");
    return false;
  }

  std::lock_guard<std::mutex> lock(adapter->callback_mutex);
  if (adapter->config.execute_input == nullptr) {
    mcp_strcpy_safe(out_message, out_message_size, kInputMissing);
    return false;
  }

  return adapter->config.execute_input(adapter->config.engine_user, command, out_message,
                                       out_message_size);
}

}  // namespace

extern "C" {

dmcp_template_t* dmcp_template_create(const dmcp_template_config_t* config) {
  std::unique_ptr<dmcp_template_t> adapter;
  try {
    adapter = std::make_unique<dmcp_template_t>();
  } catch (const std::bad_alloc&) {
    return nullptr;
  }

  adapter->config                  = CopyConfig(config);
  adapter->config.base.struct_size = sizeof(dmcp_config_t);
  adapter->config.base.on_snapshot = SnapshotCallback;
  adapter->config.base.user_data   = adapter.get();

  std::unique_ptr<dmcp_context_t, decltype(&dmcp_context_destroy)> dmcp_ctx(
      dmcp_context_create(&adapter->config.base), dmcp_context_destroy);
  if (!dmcp_ctx) {
    return nullptr;
  }

  adapter->dmcp_ctx = dmcp_ctx.release();
  return adapter.release();
}

void dmcp_template_destroy(dmcp_template_t* adapter) {
  if (adapter == nullptr) {
    return;
  }

  std::unique_ptr<dmcp_template_t> owned_adapter(adapter);
  if (owned_adapter->dmcp_ctx != nullptr) {
    dmcp_context_destroy(owned_adapter->dmcp_ctx);
    owned_adapter->dmcp_ctx = nullptr;
  }
}

mcp_status_t dmcp_template_tick(dmcp_template_t* adapter) {
  if (adapter == nullptr || adapter->dmcp_ctx == nullptr) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  if (adapter->config.base.start_transport && !dmcp_context_is_running(adapter->dmcp_ctx)) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_DISABLED, "Operation disabled");
  }

  dmcp_context_tick(adapter->dmcp_ctx);
  return MCP_STATUS_OK("Success");
}

void dmcp_template_commands_process(dmcp_template_t* adapter) {
  if (adapter == nullptr || adapter->dmcp_ctx == nullptr) {
    return;
  }

  dmcp_adapter_process_command_queue(adapter->dmcp_ctx, adapter, ExecuteCommandCallback, 0);
}

void dmcp_template_inputs_process(dmcp_template_t* adapter) {
  if (adapter == nullptr || adapter->dmcp_ctx == nullptr) {
    return;
  }

  dmcp_adapter_process_input_queue(adapter->dmcp_ctx, adapter, ExecuteInputCallback, 0);
}

bool dmcp_template_command_execute(dmcp_template_t* adapter, const dmcp_command_t* command) {
  char message[128] = {0};
  return ExecuteCommandCallback(adapter, command, message, sizeof(message));
}

mcp_status_t dmcp_template_capture_frame(dmcp_template_t* adapter) {
  if (adapter == nullptr || adapter->dmcp_ctx == nullptr) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_INVALID_ARGS, "Invalid arguments");
  }

  if (!dmcp_screenshot_is_requested(adapter->dmcp_ctx)) {
    return MCP_STATUS_OK("No screenshot requested");
  }

  if (adapter->config.capture_frame == nullptr) {
    return MCP_STATUS_ERROR(MCP_STATUS_CODE_DISABLED, "Screenshot callback not configured");
  }

  dmcp_screenshot_frame_t frame;
  std::memset(&frame, 0, sizeof(frame));

  {
    std::lock_guard<std::mutex> lock(adapter->callback_mutex);
    if (!adapter->config.capture_frame(adapter->config.engine_user, &frame)) {
      return MCP_STATUS_ERROR(MCP_STATUS_CODE_INTERNAL, "Screenshot capture failed");
    }
  }

  return dmcp_screenshot_submit(adapter->dmcp_ctx, &frame);
}

bool dmcp_template_is_running(const dmcp_template_t* adapter) {
  if (adapter == nullptr || adapter->dmcp_ctx == nullptr) {
    return false;
  }
  if (!adapter->config.base.start_transport) {
    return true;
  }
  return dmcp_context_is_running(adapter->dmcp_ctx);
}

dmcp_context_t* dmcp_template_get_context(dmcp_template_t* adapter) {
  if (adapter == nullptr) {
    return nullptr;
  }
  return adapter->dmcp_ctx;
}

void dmcp_template_get_stats(dmcp_template_t* adapter, dmcp_stats_t* out_stats) {
  if (adapter == nullptr || adapter->dmcp_ctx == nullptr || out_stats == nullptr) {
    return;
  }
  dmcp_stats_get(adapter->dmcp_ctx, out_stats);
}

}  // extern "C"
