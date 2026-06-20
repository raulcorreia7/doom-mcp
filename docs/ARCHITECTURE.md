# DMCP Architecture

DMCP separates engine hooks from MCP transport so Doom-family engines can expose
state, commands, input, and screenshots to agents without coupling engine code to
JSON-RPC or HTTP internals.

```text
Engine <-> Engine Adapter <-> Doom MCP <-> Generic MCP <-> Core API/runtime
```

## Layers

| Layer | Responsibility | Primary Files |
|-------|----------------|---------------|
| Engine | Owns native game state, tick timing, rendering, and runtime assets | consuming engine repo |
| Engine Adapter | Translates engine state/commands to DMCP types and queues input safely | `adapters/`, `include/dmcp/adapters/` |
| Doom MCP | Owns Doom state types, content catalogs, command parsing, screenshots, and MCP tools | `src/doom/`, `include/dmcp/doom/` |
| Generic MCP | Owns MCP lifecycle, JSON-RPC, HTTP/SSE transport, sessions, and method dispatch | `src/mcp/`, `include/mcp/generic/` |
| Core | Owns shared status, generated version headers, export macros, string helpers, and package target metadata | `src/core/`, `include/mcp/core/` |

## Dependency Rules

```text
engine -> adapter -> doom mcp -> generic mcp -> core
                     ^
                     |
              adapter helpers
```

- Generic MCP does not include Doom or adapter headers.
- Doom MCP depends on Generic MCP and Core, not on adapters.
- Adapters depend on Doom MCP, adapter helpers, and their engine headers.
- Engines include only the adapter-facing headers they need.
- Serialization stays in Doom MCP; adapters populate structured snapshot and
  command types instead of hand-building JSON.

`tests/unit/test_layer_boundaries.cpp` enforces the most important include
direction rules.

## Public API

Public headers are C99 API first. Owned runtime objects use opaque handles and
explicit destroy functions. Configuration structs include `struct_size` so fields
can be added without breaking older callers. C++ wrappers are convenience
headers over the C API, not a replacement for the stable C boundary.

### Generic MCP

```c
#include "mcp/generic/server.h"

mcp_server_t* server = mcp_server_create(&config);
mcp_server_method_register(server, "tools/call", handler, user_data);
mcp_server_destroy(server);
```

### Doom MCP

```c
#include "dmcp/doom/dmcp.h"

dmcp_context_t* ctx = dmcp_context_create(&config);
dmcp_context_tick(ctx);
dmcp_context_destroy(ctx);
```

### Engine Adapters

Adapter source currently exists for:

- `adapters/crispy-doom/`
- `adapters/zdoom/`
- `adapters/fake/` for deterministic SDK tests

Adapter-specific build requirements and hook examples live in each adapter
README. The consuming engine repository owns game builds, release packaging, and
runtime assets.

## Build Targets

| Target | Purpose |
|--------|---------|
| `dmcp::runtime` | Preferred SDK runtime target for consumers |
| `dmcp::single` | Unified static/shared DMCP implementation target |
| `dmcp::core` | Doom MCP compatibility alias |
| `dmcp::generic` | Generic MCP compatibility alias |
| `dmcp::adapter_crispy` | Optional crispy-doom adapter target |
| `dmcp::adapter_zdoom` | Optional zdoom adapter target |
| `dmcp::adapter_fake` | Optional deterministic test adapter target |

## SDK Validation

SDK CI validates protocol, layer boundaries, C headers, and fake-adapter MCP
transport behavior without launching a game:

```bash
cmake -B build/default \
  -DDMCP_BUILD_TESTS=ON \
  -DDMCP_BUILD_INTEGRATION_TESTS=ON \
  -DDMCP_BUILD_ADAPTER_FAKE=ON
cmake --build build/default --parallel
ctest --test-dir build/default -L "unit|sdk" -LE "requires_game" --output-on-failure
```

Engine-backed runtime validation belongs in the consuming engine repository.
