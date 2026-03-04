#ifndef DMCP_INTEGRATION_H
#define DMCP_INTEGRATION_H

#include "dmcp_crispy.h"
#include "dmcp_hooks.h"

#ifdef __cplusplus
extern "C" {
#endif

void DMCP_Init(const dmcp_engine_config_t* config);
void DMCP_Shutdown(void);
void DMCP_Tick(void);

#ifdef __cplusplus
}
#endif

#endif
