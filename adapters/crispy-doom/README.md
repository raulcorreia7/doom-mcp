# crispy-doom Adapter

This adapter provides a narrow C hook layer for a crispy-doom engine fork. The
adapter source lives in DMCP; the consuming crispy-doom repository owns the
actual game build, packaging, release flow, and runtime assets.

## Boundary

```text
crispy-doom engine hooks
  <-> DMCP_* hook API (`include/engine_hooks.h`)
  <-> dmcp_crispy_* adapter internals
  <-> Doom MCP context
```

Engine code should call the `DMCP_*` hook API. The lower-level
`dmcp_crispy_*` lifecycle is adapter implementation detail for tests and
advanced embedding.

## Files

| Path | Purpose |
|------|---------|
| `include/engine_hooks.h` | Engine-facing hook API |
| `include/dmcp_crispy.h` | Lower-level adapter lifecycle |
| `src/engine_hooks.c` | Argument parsing, init/tick/frame/shutdown glue |
| `src/adapter_lifecycle.c` | DMCP context lifecycle and queue processing |
| `src/state_player.c` | Player snapshot extraction |
| `src/state_level.c` | Level/game metadata extraction |
| `src/state_enemies.c` | Enemy and world entity enumeration |
| `src/command_exec.c` | Command execution bridge |
| `src/input.c` | Tick-level input bridge |
| `src/dmcp_ascii.c` | Framebuffer-to-ASCII conversion |

## Build

Enable this adapter from an engine build that provides the required source and
generated-header include directories:

```bash
cmake -S /path/to/doom-mcp -B build/dmcp-crispy \
  -DDMCP_BUILD_SHARED=ON \
  -DDMCP_BUILD_SINGLE_DLL=ON \
  -DDMCP_BUILD_ADAPTER_CRISPY=ON \
  -DDMCP_CRISPY_SRC_DIR=/path/to/crispy/src \
  -DDMCP_CRISPY_DOOM_DIR=/path/to/crispy/src/doom \
  -DDMCP_CRISPY_GEN_INCLUDE_DIR=/path/to/crispy/build
cmake --build build/dmcp-crispy --parallel
```

## Engine Hook Example

Keep the engine-side changes minimal:

```c
#include "engine_hooks.h"

void D_DoomMain(void) {
  dmcp_engine_config_t config = DMCP_ParseArgs(myargc, myargv);
  DMCP_Init(config);

  /* continue normal engine startup */
}

void G_Ticker(void) {
  DMCP_Tick();

  /* continue normal game tick */
}

void D_Display(void) {
  DMCP_CaptureFrame();

  /* continue normal frame presentation */
}

void I_Quit(void) {
  DMCP_Shutdown();
}
```

`DMCP_ParseArgs()` returns a value-owned config, so the engine can inspect or
override it before calling `DMCP_Init(config)` without heap ownership or parse
cleanup.

## Supported Runtime Switches

The hook parser currently understands:

| Switch | Purpose |
|--------|---------|
| `-dmcp_port <port>` | Override the MCP HTTP port |
| `-dmcp_allow_cheats` | Enable cheat-gated tools |
| `-dmcp_allow_console` | Enable console command execution |

Console commands are enabled automatically when cheats are enabled.

## Hook Responsibilities

- `DMCP_Init(config)` creates the adapter and registers shutdown cleanup.
- `DMCP_Tick()` publishes snapshots and drains queued commands/input.
- `DMCP_CaptureFrame()` submits a screenshot only when the MCP layer requested
  one.
- `DMCP_Shutdown()` destroys the adapter context and is safe to call during
  engine shutdown.

## Notes

- This SDK repository does not launch crispy-doom or download game data.
- Adapter compile checks need crispy-doom headers from the consuming engine
  build.
- The adapter exposes canonical DMCP tool/content names rather than engine-local
  aliases.
