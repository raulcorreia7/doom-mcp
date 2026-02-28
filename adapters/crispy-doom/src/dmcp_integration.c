#include "dmcp_integration.h"
#include "dmcp_hooks.h"
#include "i_system.h"
#include "m_argv.h"
#include <stdint.h>
#include <stdlib.h>

dmcp_crispy_t* g_dmcp_ctx = NULL;

void DMCP_Init(void) {
  dmcp_crispy_config_t dmcp_cfg;
  int                  dmcp_port;

  dmcp_cfg  = dmcp_crispy_config_default();
  dmcp_port = dmcp_engine_port_from_argv(myargc, myargv, "dmcp_port");

  if (dmcp_port < 0) {
    I_Error("Invalid DMCP port (expected 1-65535)");
  }

  if (dmcp_port > 0) {
    dmcp_cfg.base.port = (uint16_t)dmcp_port;
  }

  I_AtExit(DMCP_Shutdown, true);
  g_dmcp_ctx = dmcp_crispy_create(&dmcp_cfg);
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
