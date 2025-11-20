#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Initialize the DMCP adapter for ZDoom
void dmcp_zdoom_init();

/// Shutdown the DMCP adapter for ZDoom
void dmcp_zdoom_shutdown();

/// Update the DMCP adapter for ZDoom (call this every game tick)
void dmcp_zdoom_tick();

#ifdef __cplusplus
}
#endif
