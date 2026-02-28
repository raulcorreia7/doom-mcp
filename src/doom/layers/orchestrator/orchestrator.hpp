#pragma once

#include "dmcp/doom/layer.h"

#ifdef __cplusplus
extern "C" {
#endif

DMCP_API dmcp_layer_t* dmcp_orchestrator_layer_create(void);
DMCP_API void          dmcp_orchestrator_layer_destroy(dmcp_layer_t* layer);

#ifdef __cplusplus
}
#endif
