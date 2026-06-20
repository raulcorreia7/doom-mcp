# DMCP Adapters

Adapters bridge DMCP core (`dmcp::single` / `dmcp::core`) to concrete engine internals.
Their public surface is a small C99-compatible API; engine-specific hook code
stays in the engine or adapter, not in the generic MCP layer.

## Scope And Boundaries

Keep a strict balance between shared and engine-specific code:

- Move to `adapters/common/` only code that is truly cross-engine (queue draining, tiny glue helpers).
- Keep in each engine adapter all game-specific state extraction, command mapping, and runtime hooks.
- Do not move engine-specific include dependencies, enums, or command semantics into common code.

## Layout

```text
adapters/
  common/                 # Shared adapter-only helpers
  crispy-doom/            # crispy-doom adapter
  zdoom/                  # zdoom adapter
  fake/                   # Deterministic test adapter
  template/               # New-adapter guide
```

Public adapter headers live in:

- `include/dmcp/adapters/crispy.h`
- `include/dmcp/adapters/zdoom.h`
- `include/dmcp/adapters/fake.h`

## Naming Conventions

### C API

- Prefix: `dmcp_<engine>_...`
- Return status type: `mcp_status_t` from `include/mcp/core/status.h`
- Configuration structs include `struct_size` so adapters can add fields while
  preserving source-compatible initialization.
- Common lifecycle surface per adapter:
  - `*_config_default()`
  - `*_create()` / `*_destroy()`
  - `*_tick()`
  - `*_commands_process()`
  - `*_inputs_process()`
  - `*_is_running()`
  - `*_get_stats()`
  - `*_get_context()`

### CMake Targets

- Real targets: `adapter_<engine>`
- Preferred aliases: `dmcp::adapter_<engine>`

## Shared Utilities

- `adapters/common/include/dmcp_adapter_command_queue.h`
  - `dmcp_adapter_process_command_queue()`
  - `dmcp_adapter_process_input_queue()`
- `adapters/common/include/dmcp_hooks.h`
  - `dmcp_engine_config_default()`
  - `dmcp_engine_config_from_argv()`
  - `dmcp_engine_port_from_argv()`
  - `dmcp_engine_flag_from_argv()`
- `include/dmcp/adapter/utils.h`
- `include/dmcp/adapter/validation.h`

## Engine Loop Pattern

Prefer a small engine-facing hook layer for each adapter:

```c
void Engine_Init(int argc, char** argv) {
  adapter_config_t config = Adapter_ParseArgs(argc, argv);
  Adapter_Init(config);
}

void Engine_Tick(void) {
  Adapter_Tick();
}

void Engine_AfterRender(void) {
  Adapter_CaptureFrame();
}

void Engine_Shutdown(void) {
  Adapter_Shutdown();
}
```

The concrete `crispy-doom` and `zdoom` adapters both expose this as `DMCP_*`
hooks through adapter-local `engine_hooks.h` headers. Lower-level
`dmcp_<engine>_*` APIs remain available for tests and advanced embedders.

Link only one real engine adapter into a game binary when using the `DMCP_*`
hook names. The symbols are intentionally simple at the engine boundary and are
not a multi-adapter plugin ABI.

## Integration Note

For engine repos consuming this project as a submodule:

- Link core from this repo (`dmcp::single` preferred).
- Keep the final engine hook points in the engine repo/fork.
- Treat adapter code here as reference + reusable base, not a forced integration patch.
