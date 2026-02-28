#include "dmcp_integration.h"
#include "i_system.h"
#include "m_argv.h"
#include <stdlib.h>

dmcp_crispy_t* g_dmcp_ctx = NULL;

static int DMCP_PortOverride(void) {
  int   p;
  char* end;
  long  port;

  p = M_CheckParmWithArgs("-dmcp_port", 1);
  if (!p) {
    p = M_CheckParmWithArgs("-dmcp-port", 1);
  }

  if (!p) {
    return 0;
  }

  end  = NULL;
  port = strtol(myargv[p + 1], &end, 10);
  if (end == myargv[p + 1] || *end != '\0' || port < 1 || port > 65535) {
    I_Error("Invalid DMCP port '%s' (expected 1-65535)", myargv[p + 1]);
  }

  return (int)port;
}

void DMCP_Init(void) {
  dmcp_crispy_config_t dmcp_cfg;
  int                  dmcp_port;

  dmcp_cfg  = dmcp_crispy_config_default();
  dmcp_port = DMCP_PortOverride();

  if (dmcp_port > 0) {
    dmcp_cfg.base.port = dmcp_port;
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
