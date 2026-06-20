#include "engine_hooks.h"

namespace {

dmcp_template_t* g_dmcp_ctx = nullptr;

dmcp_template_config_t to_adapter_config(dmcp_engine_config_t config) {
  dmcp_template_config_t adapter_config = dmcp_template_config_default();
  adapter_config.base.port =
      static_cast<uint16_t>(config.port > 0 ? config.port : adapter_config.base.port);
  adapter_config.base.target_hz = static_cast<uint32_t>(
      config.target_hz > 0 ? config.target_hz : adapter_config.base.target_hz);
  adapter_config.base.screenshot.enable                  = config.screenshot_enabled;
  adapter_config.base.permissions.allow_console_commands = config.allow_console_commands;
  adapter_config.base.permissions.allow_cheats           = config.allow_cheats;
  return adapter_config;
}

}  // namespace

extern "C" dmcp_engine_config_t DMCP_ParseArgs(int argc, char** argv) {
  return dmcp_engine_config_from_argv(argc, argv);
}

extern "C" void DMCP_Init(dmcp_engine_config_t config) {
  dmcp_template_config_t adapter_config = to_adapter_config(config);

  DMCP_Shutdown();
  g_dmcp_ctx = dmcp_template_create(&adapter_config);
}

extern "C" void DMCP_Shutdown(void) {
  if (g_dmcp_ctx != nullptr) {
    dmcp_template_destroy(g_dmcp_ctx);
    g_dmcp_ctx = nullptr;
  }
}

extern "C" void DMCP_Tick(void) {
  if (g_dmcp_ctx == nullptr) {
    return;
  }

  (void)dmcp_template_tick(g_dmcp_ctx);
  dmcp_template_commands_process(g_dmcp_ctx);
  dmcp_template_inputs_process(g_dmcp_ctx);
}

extern "C" void DMCP_CaptureFrame(void) {
  if (g_dmcp_ctx != nullptr) {
    (void)dmcp_template_capture_frame(g_dmcp_ctx);
  }
}
