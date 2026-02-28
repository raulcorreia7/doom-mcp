#include "dmcp_integration.h"
#include "i_system.h"
#include <stdlib.h>

static dmcp_crispy_t* g_dmcp_ctx = NULL;

void DMCP_Init(const dmcp_engine_config_t* config) {
  dmcp_engine_config_t cfg = config ? *config : dmcp_engine_config_default();

  dmcp_crispy_config_t adapter_cfg   = dmcp_crispy_config_default();
  adapter_cfg.base.port              = cfg.port;
  adapter_cfg.base.target_hz         = cfg.target_hz;
  adapter_cfg.base.screenshot.enable = cfg.screenshot_enabled;

  I_AtExit(DMCP_Shutdown, true);
  g_dmcp_ctx = dmcp_crispy_create(&adapter_cfg);
}

void DMCP_Shutdown(void) {
  if (g_dmcp_ctx) {
    dmcp_crispy_destroy(g_dmcp_ctx);
    g_dmcp_ctx = NULL;
  }
}

void DMCP_Tick(void) {
  if (g_dmcp_ctx) {
    dmcp_crispy_tick(g_dmcp_ctx);
    dmcp_crispy_commands_process(g_dmcp_ctx);
  }
}
