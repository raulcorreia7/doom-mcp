# DMCP Instance Handle Migration

This document provides an end-to-end plan for replacing the global DMCP
singleton (`g_state`) with an opaque `dmcp_instance_t` handle returned from
`dmcp_init`. The goal is to make DMCP embedding explicit, support multiple
instances if needed, and avoid global state.

---

## Overview

### Current Architecture (Before)
- `src/dmcp.cpp` declares a global `RuntimeState g_state`. Every public API
  (`dmcp_init`, `dmcp_process_tick`, screenshot helpers, etc.) assumes it.
- The adapter, server thread, and PNG encoder capture pointers to elements
  inside this global.
- No formal lifecycle handle is exposed to integrators.

### Target Architecture (After)
- `dmcp_init(const dmcp_config_t*)` returns a `dmcp_instance_t*`.
- All public APIs accept a handle, e.g., `dmcp_process_tick(dmcp_instance_t*)`.
- Internal systems (server thread, adapter) store references to the handle.
- Potential to host multiple DMCP instances, though typical embedding uses one.

---

## Migration Tasks & Steps

### 1. Define Opaque Handle
- Create `struct dmcp_instance_t;` forward declaration in `include/dmcp/dmcp.h`.
- Add a private struct (formerly `RuntimeState`) to `src/dmcp.cpp` and typedef
  it to `dmcp_instance_t`.
- Remove the global `g_state`; each call will refer to a `dmcp_instance_t*`.

### 2. Rework Public API
Update all public functions:

| Before | After |
| --- | --- |
| `int dmcp_init(const dmcp_config_t*);` | `dmcp_instance_t* dmcp_init(const dmcp_config_t*);` |
| `void dmcp_shutdown(void);` | `void dmcp_shutdown(dmcp_instance_t*);` |
| `bool dmcp_is_running(void);` | `bool dmcp_is_running(dmcp_instance_t*);` |
| `void dmcp_process_tick(void);` | `void dmcp_process_tick(dmcp_instance_t*);` |
| `... screenshot helpers ...` | `... accept dmcp_instance_t* ...` |
| `uint64_t dmcp_dropped_snapshot_count(void);` | `uint64_t dmcp_dropped_snapshot_count(dmcp_instance_t*);` |

Implementation steps:
1. Store the new instance pointer returned from init.
2. Validate all call sites pass the handle (adapters, engine integration).
3. Provide optional shim functions (deprecated) for old signature if needed.

### 3. Internal Refactor
- Move the contents of `RuntimeState` into the `dmcp_instance_t` struct. This includes mutex, queue, pool, server, screenshot state, throttling, drop counter.
- In `dmcp_process_tick`, `dmcp_submit_screenshot`, etc., operate on the passed-in instance rather than globals.
- Server thread context:
  - Pass the handle pointer into `ServerRunner` constructor; store it instead of referencing globals.
  - All queue/pool accesses become `instance->queue`.
- Snapshot pool, queue, PNG encoder, and binder should either store references to the instance or receive it as an argument.

### 4. Adapter Integration
- Modify `dmcp_adapter_setup()` to accept a `dmcp_instance_t*` or provide a setter so the adapter can store the active instance pointer.
- Ensure all lambdas that previously captured global state now capture the instance via a static pointer or parameter.
- Update UZDoom integration docs to show storing the returned handle and passing it into each DMCP call.

### 5. Documentation & Samples
- Update `README.md` to document the new lifecycle:
  ```c
  dmcp_instance_t* ctx = dmcp_init(&cfg);
  if (!ctx) { /* handle failure */ }
  dmcp_adapter_setup(ctx);
  ...
  dmcp_process_tick(ctx);
  ...
  dmcp_shutdown(ctx);
  ```
- Add notes on multiple instances (if supported) or clarify only one should be created.
- Update the JSON schema and commands section if necessary for clarity.

### 6. Optional Backwards Compatibility (if needed)
- Provide `dmcp_instance_t* dmcp_default_instance();` that lazily initializes a singleton for legacy call sites.
- Keep deprecated wrappers (`dmcp_process_tick_legacy()`) that call into the default instance, but warn integrators to migrate.
- Remove these shims in a future major version.

### 7. Testing & Validation
- Unit tests: ensure creating two instances works without cross-talk (if desired).
- Integration tests: embed DMCP in a sample loop storing the handle.
- Thread-safety checks: confirm shutdown tears down the server thread per-instance.

---

## Execution Order

1. Introduce `dmcp_instance_t` structure and modify `dmcp_init` to return it.
2. Update internal code to operate on instances, while keeping legacy wrappers for existing APIs (optional).
3. Update adapters and UZDoom integration to store/pass the handle.
4. Clean up documentation, examples, and migration notes.
5. Remove legacy shims (if any) once codebases migrate.

By following these steps, DMCP moves from an implicit global to an explicit, handle-based API that is easier to embed and extend.
