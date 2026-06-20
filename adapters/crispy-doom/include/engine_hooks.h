#ifndef DMCP_CRISPY_ENGINE_HOOKS_H
#define DMCP_CRISPY_ENGINE_HOOKS_H

#include "dmcp_crispy.h"
#include "dmcp_hooks.h"

#ifdef __cplusplus
extern "C" {
#endif

dmcp_engine_config_t DMCP_ParseArgs(int argc, char** argv);
void                 DMCP_Init(dmcp_engine_config_t config);
void                 DMCP_Shutdown(void);
void                 DMCP_Tick(void);
void                 DMCP_CaptureFrame(void);

#ifdef __cplusplus
}
#endif

#endif
