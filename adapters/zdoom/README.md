# zdoom Adapter

This adapter provides a C++ bridge between zdoom-family engine internals and the
DMCP SDK. The consuming engine repository owns the final game build, runtime
assets, packaging, and release flow.

## Boundary

```text
zdoom-family engine hooks
  <-> dmcp_zdoom_* adapter API (`include/dmcp/adapters/zdoom.h`)
  <-> Doom MCP context
```

## Files

| Path | Purpose |
|------|---------|
| `include/engine_hooks.h` | Engine-facing easy hook API |
| `include/dmcp_zdoom.h` | Adapter-local include over the public C API |
| `include/internal.h` | Internal adapter context and logging helpers |
| `src/adapter.cpp` | Lifecycle, snapshot extraction, stats |
| `src/commands.cpp` | Command execution and queue processing |
| `src/engine_hooks.cpp` | Parse/init/tick/frame/shutdown glue |

## Build

Enable the adapter from a build that can provide zdoom-family include paths:

```bash
cmake -S /path/to/doom-mcp -B build/dmcp-zdoom \
  -DDMCP_BUILD_SHARED=ON \
  -DDMCP_BUILD_SINGLE_DLL=ON \
  -DDMCP_BUILD_ADAPTER_ZDOOM=ON \
  -DDMCP_ZDOOM_INCLUDE_DIRS=/path/to/zdoom/src
cmake --build build/dmcp-zdoom --parallel
```

For multiple include roots, pass a CMake semicolon-separated list:

```bash
-DDMCP_ZDOOM_INCLUDE_DIRS="/path/to/src;/path/to/generated"
```

## Engine Hook Example

Prefer the easy hook layer for engine-side integration:

```cpp
#include "engine_hooks.h"

void D_DoomMain() {
  dmcp_engine_config_t config = DMCP_ParseArgs(myargc, myargv);
  DMCP_Init(config);

  // continue normal engine startup
}

void G_Ticker() {
  DMCP_Tick();

  // continue normal game tick
}

void D_Display() {
  DMCP_CaptureFrame();

  // continue normal frame presentation
}

void I_Quit() {
  DMCP_Shutdown();
}
```

`DMCP_CaptureFrame()` is currently a no-op in this adapter until screenshot
capture is wired to zdoom-family rendering APIs.

## Lower-Level API

Use this only when the engine needs direct ownership of the adapter handle:

```cpp
#include "dmcp/adapters/zdoom.h"

static dmcp_zdoom_t* g_dmcp = nullptr;

void EngineStartup() {
  dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
  config.base.port = 6060;

  g_dmcp = dmcp_zdoom_create(&config);
}

void G_Ticker() {
  if (g_dmcp == nullptr) {
    return;
  }

  (void)dmcp_zdoom_tick(g_dmcp);
  dmcp_zdoom_commands_process(g_dmcp);
  dmcp_zdoom_inputs_process(g_dmcp);
}

void EngineShutdown() {
  dmcp_zdoom_destroy(g_dmcp);
  g_dmcp = nullptr;
}
```

## Configuration

`dmcp_zdoom_config_t` embeds `dmcp_config_t` as `base`, so the generic DMCP
settings are configured the same way as the core API:

```cpp
dmcp_zdoom_config_t config = dmcp_zdoom_config_default();
config.base.port = 6060;
config.base.target_hz = 35;
config.base.screenshot.enable = false;
```

Set `config.base.screenshot.enable = true` only after wiring
`DMCP_CaptureFrame()` to a real renderer frame source.
Console and cheat-style tools are enabled by default; set
`config.base.permissions.allow_cheats` or
`config.base.permissions.allow_console_commands` to `false` only for restricted
embeds.

## Adapter Responsibilities

- `dmcp_zdoom_tick()` updates the DMCP context and publishes current state.
- `dmcp_zdoom_commands_process()` drains queued mutating commands in engine
  context.
- `dmcp_zdoom_inputs_process()` is present for API symmetry; unsupported input
  actions should fail cleanly rather than being silently accepted.
- `dmcp_zdoom_context_get()` exposes the underlying `dmcp_context_t` for advanced
  engine integration.

## Notes

- This SDK repository does not launch an engine or download game data.
- Engine-header validation belongs in the consuming engine build or explicit
  local adapter builds.
- The adapter should return canonical DMCP content/tool names, not engine-local
  aliases.
