#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Initialize the DMCP adapter for UZDoom
void dmcp_adapter_setup();

/// Shutdown the DMCP adapter
void dmcp_adapter_shutdown();

/// Update the DMCP adapter (call this every game tick)
void dmcp_adapter_update();

#ifdef __cplusplus
}
#endif