#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Initialize the DMCP bridge for UZDoom
void dmcp_bridge_setup();

/// Shutdown the DMCP bridge
void dmcp_bridge_shutdown();

/// Update the DMCP bridge (call this every game tick)
void dmcp_bridge_update();

#ifdef __cplusplus
}
#endif