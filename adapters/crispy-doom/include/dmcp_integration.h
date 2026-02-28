#ifndef DMCP_INTEGRATION_H
#define DMCP_INTEGRATION_H

#include "dmcp_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

extern dmcp_crispy_t* g_dmcp_ctx;

void DMCP_Init(void);
void DMCP_Shutdown(void);
void DMCP_Tick(void);

#ifdef __cplusplus
}
#endif

#endif
