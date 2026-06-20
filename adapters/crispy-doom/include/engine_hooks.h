#ifndef DMCP_CRISPY_ENGINE_HOOKS_H
#define DMCP_CRISPY_ENGINE_HOOKS_H

#include "dmcp_crispy.h"
#include "dmcp_hooks.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Engine-facing convenience hooks.
 *
 * These functions intentionally have simple DMCP_* names because an engine
 * binary should link exactly one real DMCP engine adapter. Adapter internals use
 * dmcp_crispy_* directly; engine hook points should prefer this small surface.
 */
dmcp_engine_config_t DMCP_ParseArgs(int argc, char** argv);
void                 DMCP_Init(dmcp_engine_config_t config);
void                 DMCP_Shutdown(void);
void                 DMCP_Tick(void);
void                 DMCP_CaptureFrame(void);

#ifdef __cplusplus
}
#endif

#endif
