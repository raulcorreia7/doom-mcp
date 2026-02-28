#include "dmcp_integration.h"
#include "dmcp_hooks.h"
#include "i_system.h"
#include "m_argv.h"
#include <stdint.h>
#include <stdlib.h>

dmcp_crispy_t* g_dmcp_ctx = NULL;

static dmcp_crispy_config_t dmcp_build_config(void) {
  dmcp_crispy_config_t cfg  = dmcp_crispy_config_default();
  int                  port = dmcp_engine_port_from_argv(myargc, myargv, "dmcp_port");

  if (port < 0) {
    I_Error("Invalid DMCP port (expected 1-65535)");
  }
  if (port > 0) {
    cfg.base.port = (uint16_t)port;
  }
  return cfg;
}

void DMCP_Init(void) {
  dmcp_crispy_config_t cfg = dmcp_build_config();
  I_AtExit(DMCP_Shutdown, true);
  g_dmcp_ctx = dmcp_crispy_create(&cfg);
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
