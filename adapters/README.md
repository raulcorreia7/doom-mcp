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
  crispy-doom/            # Crispy integration
  zdoom/                  # ZDoom integration
  fake/                   # Deterministic test adapter
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
- `include/dmcp/adapter/utils.h`
- `include/dmcp/adapter/validation.h`

## Engine Loop Pattern

```c
if (adapter) {
  dmcp_<engine>_tick(adapter);
  dmcp_<engine>_commands_process(adapter);
  dmcp_<engine>_inputs_process(adapter);
}
```

## Integration Note

For engine repos consuming this project as a submodule:

- Link core from this repo (`dmcp::single` preferred).
- Keep the final engine hook points in the engine repo/fork.
- Treat adapter code here as reference + reusable base, not a forced integration patch.
