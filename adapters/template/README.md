# Adapter Template

This directory is a copyable adapter skeleton. It is intentionally not included
by the top-level DMCP build. Copy it to `adapters/<engine>`, rename the symbols,
then add the new adapter to the SDK build only when it becomes a real adapter.

## What It Provides

```text
engine hooks
  <-> DMCP_ParseArgs / DMCP_Init / DMCP_Tick / DMCP_CaptureFrame / DMCP_Shutdown
  <-> dmcp_template_* adapter API
  <-> dmcp_context_t
```

Files:

```text
adapters/template/
  CMakeLists.txt              # standalone/copyable adapter build
  dmcp_template.version       # Linux symbol exports for shared builds
  include/
    dmcp_template.h           # example stable C adapter API
    engine_hooks.h            # small engine-facing hook API
  src/
    adapter.cpp               # C API over private C++ implementation
    engine_hooks.cpp          # simple DMCP_* hook wrapper
```

## Try The Template Build

From the SDK root:

```sh
cmake -S adapters/template -B build/adapter-template \
  -DDMCP_SDK_ROOT="$PWD" \
  -DDMCP_BUILD_SHARED=ON \
  -DDMCP_BUILD_SINGLE_DLL=ON
cmake --build build/adapter-template --parallel
```

This validates the template in isolation. It does not add `adapters/template` to
the main SDK adapter list.

## Create A Real Adapter

```sh
cp -R adapters/template adapters/my-engine
```

Then rename:

- `dmcp_template_*` to `dmcp_my_engine_*`
- `dmcp_template_t` to `dmcp_my_engine_t`
- `dmcp_template_config_t` to `dmcp_my_engine_config_t`
- `DMCP_TEMPLATE_*` include guards or constants to your adapter prefix
- `dmcp_adapter_template` CMake target to `adapter_my_engine`
- `dmcp_template.version` exports to your adapter prefix
- `include/dmcp_template.h` to `include/dmcp/adapters/my_engine.h` when the
  adapter is promoted into the SDK public headers

After the rename, add the new adapter option and `add_subdirectory()` entry in
the SDK root build. Keep the template itself out of the root build.

## Engine Hook Usage

The engine integration should stay small:

```c
#include "engine_hooks.h"

void Engine_Init(int argc, char** argv) {
  dmcp_engine_config_t dmcp_config = DMCP_ParseArgs(argc, argv);
  DMCP_Init(dmcp_config);
}

void Engine_Tick(void) {
  DMCP_Tick();
}

void Engine_AfterRender(void) {
  DMCP_CaptureFrame();
}

void Engine_Shutdown(void) {
  DMCP_Shutdown();
}
```

The adapter owns translation between engine state and DMCP structs.

## Fill Engine State

Replace the template callbacks with engine-specific code:

```c
static bool FillSnapshot(void* engine_user, dmcp_snapshot_t* out) {
  const my_engine_t* engine = (const my_engine_t*)engine_user;

  dmcp_snapshot_clear(out);
  mcp_strcpy_safe(out->level.level_id, sizeof(out->level.level_id),
                  engine->map_name);
  mcp_strcpy_safe(out->player.readyweapon, sizeof(out->player.readyweapon),
                  engine->weapon_name);
  out->player.hp = engine->player_health;
  out->player.position.x = engine->player_x;
  out->player.position.y = engine->player_y;
  out->player.angle = engine->player_angle;
  return true;
}

static bool ExecuteCommand(void* engine_user, const dmcp_command_t* command,
                           char* out_message, size_t out_message_size) {
  my_engine_t* engine = (my_engine_t*)engine_user;

  switch (command->type) {
    case DMCP_CMD_CHANGE_LEVEL:
      return MyEngine_ChangeLevel(engine, command->data.change_level.map_name,
                                  out_message, out_message_size);
    case DMCP_CMD_SET_PLAYER_POSITION:
      return MyEngine_SetPlayerPosition(engine, command->data.set_position.position.x,
                                        command->data.set_position.position.y,
                                        command->data.set_position.angle);
    default:
      mcp_strcpy_safe(out_message, out_message_size, "Unsupported command");
      return false;
  }
}

dmcp_template_config_t cfg = dmcp_template_config_default();
cfg.engine_user = engine;
cfg.fill_snapshot = FillSnapshot;
cfg.execute_command = ExecuteCommand;
cfg.execute_input = ExecuteCommand;
dmcp_template_t* dmcp = dmcp_template_create(&cfg);
```

Use `dmcp_template_capture_frame()` only after rendering and only when the
engine can provide a contiguous RGBA frame. Screenshot tools are disabled by
default; set `cfg.base.screenshot.enable = true` only when `capture_frame` is
implemented. DMCP copies the pixels immediately.

## Rules

- Keep serialization in Doom MCP, not adapters.
- Keep engine state reads and command translation in the adapter.
- Return canonical DMCP names, not engine-local aliases.
- Keep engine hooks to parse/init/tick/frame/shutdown.
- Do not launch engines or download game data from SDK tests.
- Promote helpers to `adapters/common/` only after at least two real adapters
  need the same behavior.
