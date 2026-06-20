#ifndef DMCP_TEMPLATE_ENGINE_HOOKS_H
#define DMCP_TEMPLATE_ENGINE_HOOKS_H

#include "dmcp_hooks.h"
#include "dmcp_template.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal engine-facing hook surface.
 *
 * A game should link exactly one real adapter that exports these names. Keep
 * engine code limited to parse/init/tick/frame/shutdown and put translation
 * work in the adapter implementation.
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
