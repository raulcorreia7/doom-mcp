#include "engine_hooks.h"

namespace {

dmcp_zdoom_t* g_dmcp_ctx = nullptr;

dmcp_zdoom_config_t ToAdapterConfig(dmcp_engine_config_t config) {
  dmcp_zdoom_config_t adapter_config = dmcp_zdoom_config_default();
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
  dmcp_zdoom_config_t adapter_config = ToAdapterConfig(config);

  DMCP_Shutdown();
  g_dmcp_ctx = dmcp_zdoom_create(&adapter_config);
}

extern "C" void DMCP_Shutdown(void) {
  if (g_dmcp_ctx != nullptr) {
    dmcp_zdoom_destroy(g_dmcp_ctx);
    g_dmcp_ctx = nullptr;
  }
}

extern "C" void DMCP_Tick(void) {
  if (g_dmcp_ctx == nullptr) {
    return;
  }

  (void)dmcp_zdoom_tick(g_dmcp_ctx);
  dmcp_zdoom_commands_process(g_dmcp_ctx);
  dmcp_zdoom_inputs_process(g_dmcp_ctx);
}

extern "C" void DMCP_CaptureFrame(void) {
  // zdoom screenshot capture is not wired through this adapter yet.
}
