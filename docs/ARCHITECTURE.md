# DMCP Architecture (Clean & Modular)

**Version**: 0.6.0  
**Target Architecture**: Crispy/Engine -> Adapter -> Doom MCP -> Generic MCP -> core API/runtime concerns

DMCP separates Doom engine integration from MCP transport so a Doom port can
expose state, commands, input, and screenshots to agents without coupling engine
code to JSON-RPC/HTTP internals. The current source backs that split with public
C headers in `include/`, generic MCP implementation in `src/mcp/`, Doom-specific
implementation in `src/doom/`, and adapters under `adapters/`.

## Layers Explained

### What are the layers?

DMCP is organized into five target layers, each with a single responsibility:

| Layer | What it does | Why it exists |
|-------|--------------|---------------|
| **Crispy / Engine** | Runs the game loop and owns native state | Keeps source-port behavior authoritative |
| **Adapter** | Extracts state, queues input/commands, translates engine types | Isolates engine-specific hooks and fixed-point/entity quirks |
| **Doom MCP** | Doom state types, serialization, commands, layer tools | Keeps Doom semantics out of generic MCP |
| **Generic MCP** | HTTP/SSE transport, JSON-RPC lifecycle, method dispatch | Reusable protocol layer for any domain |
| **Core** | Shared status, API metadata, export/version headers, and package targets | Keeps public boundaries stable across static/shared builds |

### How do they interact?

```
Request Flow (Agent -> Engine):
  Agent -> HTTP POST -> Generic MCP -> Doom MCP -> Adapter -> Crispy/Engine

Response Flow (Engine -> Agent):
  Crispy/Engine -> Adapter -> Doom MCP -> Generic MCP -> HTTP/SSE -> Agent
```

Each layer only depends on layers below it. No circular dependencies.

### Layer Details

#### Generic MCP Layer

**What**: Pure MCP protocol implementation with no game knowledge.

**Files**: `src/mcp/`, `include/mcp/generic/`

**Responsibilities**:
- HTTP server (POST for JSON-RPC, GET for SSE)
- JSON-RPC 2.0 request/response handling
- Method registration and dispatch
- Session management

**Dependencies**: yyjson, cpp-httplib (no Doom dependencies)

#### Doom MCP Layer

**What**: Doom-specific types, serialization, and MCP tools.

**Files**: `src/doom/`, `include/dmcp/doom/`

**Responsibilities**:
- Define game state types (player, enemies, level)
- Serialize state to JSON
- Implement MCP tools (`get_player`, `spawn_entity`, `give_item`, `execute_batch`, etc.)
- Command parsing and dispatch

**Dependencies**: Generic MCP layer only

#### Core Boundary

**What**: Shared public API and packaging boundary used by the other layers.

**Files**: `include/mcp/core/status.h`, `include/mcp/core/api.h`,
`include/*/export.h`, `include/dmcp/doom/protocol.h`, CMake targets
`dmcp::single`, `dmcp::generic`, and `dmcp::core`

**Responsibilities**:
- Keep public handles opaque where ownership matters (`mcp_server_t`,
  `dmcp_context_t`)
- Export stable status, API metadata, constant, and symbol visibility definitions
- Provide C-compatible headers for API consumers
- Provide source-stable C++ protocol-name wrappers over the C constants

#### Adapter Layer

**What**: Engine-specific integration code.

**Files**: `adapters/<engine>/`; Crispy-specific source lives in
`adapters/crispy-doom/`

**Responsibilities**:
- Extract state from engine internals
- Execute commands in engine context
- Handle engine-specific quirks

**Dependencies**: Doom MCP types, engine headers

#### Adapter Helpers

**What**: Shared utilities for adapters.

**Files**: `include/dmcp/adapter/`

**Responsibilities**:
- Coordinate conversions (fixed-point to float)
- Angle conversions (BAM to degrees/radians)
- Entity name lookups
- Input validation

**Dependencies**: None (static inline functions)

---

## Layer Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      CRISPY / DOOM ENGINE (Crispy, ZDoom, etc.)              │
│                          - Raw game state access                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ADAPTER HELPERS (Optional Shared Utilities)               │
│  include/dmcp/adapter/utils.h       - Coordinate conversions                 │
│  include/dmcp/adapter/entities.h    - Entity name mappings                   │
│  include/dmcp/adapter/validation.h  - Input validation                       │
│  - Static inline functions, no dependencies                                  │
│  - Shared across all adapters to reduce code duplication                     │
└─────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                      ADAPTER INTERFACE LAYER (C API)                         │
│  adapters/<engine>/adapter.{h,cpp}                                           │
│  - Engine-specific state extraction                                          │
│  - Bridge between engine types and DMCP types                               │
│  - No dependencies on Generic MCP internals                                  │
└─────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         DOOM MCP LAYER (Game-Specific)                       │
│  include/dmcp/doom/types.h     - Data structures (Snapshot, Player, etc.)   │
│  include/dmcp/doom/config.h    - Configuration types                         │
│  include/dmcp/doom/api.h       - C API for game data                         │
│  include/dmcp/doom/content.h   - Content availability and restrictions       │
│  src/doom/context.cpp          - C API implementation                        │
│  src/doom/handlers/            - MCP tools + HTTP route handlers             │
│  src/doom/pool.cpp             - Snapshot pooling                            │
│  src/doom/serialization.cpp    - JSON serialization                          │
│  src/doom/internal/json_types.hpp - Stable JSON boundary wrapper            │
│  - Knows about Doom game state                                              │
│  - No networking/transport logic                                             │
│  - Content APIs owned by Doom core, not adapters                            │
└─────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                     GENERIC MCP LAYER (Protocol & Transport)                 │
│  include/mcp/generic/protocol.h  - MCP protocol types                        │
│  include/mcp/generic/server.h    - Generic server interface                  │
│  include/mcp/generic/transport.h - Abstract transport (SSE)                 │
│  src/mcp/server.cpp              - JSON-RPC + method dispatch               │
│  src/mcp/http_sse_transport.cpp  - HTTP + SSE transport (cpp-httplib)       │
│  - No game-specific knowledge                                                │
│  - Pure MCP protocol implementation                                          │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Dependency Rules

```
Crispy/Engine -> Adapter -> Doom MCP -> Generic MCP -> Core API/runtime
                 ^           ^
                 |           |
                 +-- Adapter Helpers --+
                     (optional shared utilities)
```

1. **Generic MCP Layer**: No dependencies on Doom or adapter headers. Pure protocol.
2. **Doom MCP Layer**: Depends on Generic MCP for registration and core status/export types.
3. **Adapter Helpers**: Optional utilities; depends only on standard headers and public constants.
4. **Adapter Layer**: Depends on Doom MCP types, adapter helpers, and engine headers.
5. **Engine**: Only touches adapter-facing public headers.

`tests/unit/test_layer_boundaries.cpp` validates key rules: Generic MCP must stay
game-agnostic, Doom core must not include adapter headers, and Doom core must use
its JSON boundary wrapper instead of importing generic JSON internals directly.

## Public API Surface

Public headers are C99 API first: exported functions use `extern "C"` and opaque
handles for owned runtime objects. Public configuration structs carry
`struct_size` so newer fields can be added without breaking older callers. C++
source convenience lives beside that API, not instead of it.
`include/dmcp/doom/protocol.h` defines C macros such as
`DMCP_TOOL_GET_PLAYER` and C++ `std::string_view` names such as
`dmcp::tools::get_player`; the strings remain owned by the C constants.

### Generic MCP Layer (for any game/tool)
```c
// mcp/generic/server.h
mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
void          mcp_server_destroy(mcp_server_t* server);
mcp_status_t mcp_server_method_register(mcp_server_t* server,
                                                 const char* method,
                                                 mcp_method_handler_t handler,
                                                 void* user_data);
void          mcp_server_event_broadcast(mcp_server_t* server,
                                          const char* event_type,
                                          const char* json_payload);
```

### Doom MCP Layer (Doom-specific)
```c
// dmcp/doom/api.h (or use dmcp/doom/dmcp.h convenience header)
dmcp_context_t* dmcp_context_create(const dmcp_config_t* config);
void            dmcp_context_destroy(dmcp_context_t* ctx);
void            dmcp_context_tick(dmcp_context_t* ctx);
```

### Adapter Helpers (Optional)
```c
// dmcp/adapter/utils.h
static inline float dmcp_fixed_to_float(int32_t fixed_val);
static inline float dmcp_angle_to_degrees(uint32_t angle);
static inline float dmcp_angle_to_radians(uint32_t angle);

// dmcp/adapter/entities.h
static inline const char* dmcp_entity_name(int mobj_type);

// dmcp/adapter/validation.h
bool dmcp_validate_health(int32_t health);
```

### Adapter Layer (Engine-specific)
```c
// include/dmcp/adapters/zdoom.h
dmcp_zdoom_t*        dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg);
mcp_status_t         dmcp_zdoom_tick(dmcp_zdoom_t* ctx);
void                 dmcp_zdoom_destroy(dmcp_zdoom_t* ctx);

// adapters/crispy-doom/include/dmcp_adapter.h
dmcp_engine_t* dmcp_engine_create(const dmcp_engine_config_t* cfg);
void           dmcp_engine_tick(dmcp_engine_t* ctx);
void           dmcp_engine_destroy(dmcp_engine_t* ctx);
```

## File Structure

```
doom-mcp/
├── cmake/
│   ├── CPM.cmake              # CPM package manager setup
│   ├── Dependencies.cmake     # External dependencies
│   ├── CompilerWarnings.cmake # Warning configuration
│   └── Sanitizers.cmake       # Sanitizer setup
│
├── docs/
│   ├── README.md              # Full documentation
│   ├── ARCHITECTURE.md        # This file
│   └── CHANGELOG.md           # Version history
│
├── include/
│   ├── mcp/
│   │   ├── core/
│   │   │   ├── api.h          # Public API metadata
│   │   │   ├── status.h       # Shared status type and macros
│   │   │   ├── string.h       # Small C API string helpers
│   │   │   ├── version.h      # Version query
│   │   │   └── export.h       # Core export macros
│   │   └── generic/
│   │       ├── protocol.h     # MCP protocol types
│   │       ├── server.h       # Generic server API
│   │       ├── transport.h    # Transport abstraction
│   │       ├── constants.h    # Buffer sizes, limits
│   │       └── export.h       # Export macros for shared libs
│   │
│   └── dmcp/
│       ├── doom/
│       │   ├── api.h           # Public C API
│       │   ├── config.h        # Configuration types
│       │   ├── types.h         # Data structures
│       │   ├── commands.h      # Command system
│       │   ├── content.h       # Content availability APIs
│       │   ├── constants.h     # Buffer sizes, limits
│       │   ├── export.h        # Export macros
│       │   └── dmcp.h          # Convenience header
│       │
│       └── adapter/            # Shared adapter utilities
│           ├── utils.h         # Coordinate/angle conversions
│           ├── entities.h      # Entity name mappings
│           ├── validation.h    # Input validation helpers
│           └── content.h       # Content lookup helpers for adapters
│
├── src/
│   ├── core/
│   │   ├── api.cpp            # Public API metadata implementation
│   │   ├── status.cpp         # Shared status helpers
│   │   ├── string.cpp         # Core string helpers
│   │   └── version.cpp        # Version implementation
│   │
│   ├── mcp/
│   │   ├── server.cpp          # Generic MCP server
│   │   ├── http_sse_transport.cpp # HTTP + SSE implementation
│   │   ├── json_rpc.cpp       # Reusable JSON-RPC helpers
│   │   ├── json_rpc.hpp
│   │   └── json/
│   │       ├── json.hpp        # JSON abstraction
│   │       └── yyjson.cpp      # yyjson implementation
│   │
│   └── doom/
│       ├── context.cpp         # C API implementation
│       ├── content.cpp         # Content availability checks
│       ├── pool.cpp            # Pool management
│       ├── serialization.cpp   # JSON serialization
│       ├── screenshot.cpp      # ASCII conversion
│       ├── commands.cpp        # Command dispatch
│       ├── internal.hpp        # Internal context header
│       ├── internal/
│       │   ├── pool.hpp        # Pool internals
│       │   ├── context.hpp     # Context internals
│       │   ├── json_types.hpp  # Stable JSON boundary
│       │   ├── screenshot.hpp  # Screenshot state
│       │   └── serialization.hpp
│       ├── handlers/
│       │   ├── common.cpp      # Shared handler utilities
│       │   ├── state_json.cpp  # State JSON builders
│       │   ├── schema_builders.cpp # Tool schema definitions
│       │   ├── routes.cpp      # HTTP route registration
│       │   └── tools/          # Tool implementations
│       │       ├── execute_batch.cpp
│       │       ├── get_command_result.cpp
│       │       ├── get_command_examples.cpp
│       │       ├── get_screenshot.cpp
│       │       ├── get_state_sections.cpp
│       │       ├── tools_list.cpp
│       │       └── get_available_content.cpp
│       ├── commands/
│       │   ├── parsers.cpp     # Command parsing
│       │   ├── json_parsers.hpp
│       │   └── types/          # Per-command implementations
│       │       ├── spawn.cpp
│       │       ├── set_health.cpp
│       │       ├── set_position.cpp
│       │       ├── change_level.cpp
│       │       ├── give_item.cpp
│       │       ├── damage.cpp
│       │       ├── kill.cpp
│       │       ├── pause.cpp
│       │       └── console.cpp
│
├── adapters/
│   ├── zdoom/
│   │   ├── include/dmcp_adapter.h  # Adapter-local include
│   │   ├── include/internal.h      # Shared internal utilities
│   │   ├── src/adapter.cpp         # ZDoom integration
│   │   └── src/commands.cpp        # ZDoom command handlers
│   │
│   └── crispy-doom/
│       ├── include/dmcp_adapter.h  # Public adapter header
│       ├── src/dmcp_adapter.c      # Lifecycle: create/destroy/tick
│       ├── src/dmcp_ascii.c        # ASCII capture implementation
│       ├── src/state_player.c      # Player state extraction
│       ├── src/state_level.c       # Level/game state extraction
│       ├── src/state_enemies.c     # Enemy + interactive entity enumeration
│       ├── src/input.c             # Tick-level input bridge
│       └── src/commands.c          # Command execution
│
├── examples/
│   ├── README.md
│   └── dummy_server.cpp
│
├── tests/
│   ├── test_utils.hpp          # Shared fixtures/factories
│   ├── unit/                   # Unit tests (Catch2)
│   │   ├── test_layer_boundaries.cpp  # Include direction verification
│   │   ├── test_doom_context.cpp
│   │   ├── test_mcp_server.cpp
│   │   ├── test_screenshot.cpp
│   │   └── test_zdoom_adapter.cpp
│   ├── support/                # Shared C++ test HTTP/network helpers
│   ├── integration/            # C/C++ no-game fake-adapter integration tests
│   │   └── run_headless.sh     # Optional engine-backed headless checks
│
├── crispy-doom/                # Crispy Doom submodule
├── CMakeLists.txt
├── Makefile
└── README.md
```

## CMake Targets

| Target | Type | Dependencies | Purpose |
|--------|------|--------------|---------|
| `dmcp::single` | STATIC/SHARED | yyjson, cpp-httplib | Unified DMCP runtime surface (`libdmcp`) |
| `dmcp::generic` | Alias | `dmcp::single` (shared) / `dmcp_generic` (static) | Generic MCP compatibility target |
| `dmcp::core` | Alias | `dmcp::single` (shared) / `dmcp_core` (static) | Doom MCP compatibility target |
| `dmcp::adapter_crispy` | STATIC/SHARED | dmcp::core | Crispy Doom adapter |
| `dmcp::adapter_zdoom` | STATIC | dmcp::core | ZDoom adapter |
| `dmcp::adapter_fake` | STATIC/SHARED | dmcp::core | Deterministic fake adapter for smoke/integration tests |

## Build Configuration

```cmake
# Recommended local-source integration for engine consumers.
add_subdirectory(path/to/doom-mcp path/to/doom-mcp-build EXCLUDE_FROM_ALL)
target_link_libraries(myengine PRIVATE dmcp::runtime)

# Options
option(DMCP_BUILD_TESTS "Build tests" OFF)
option(DMCP_BUILD_EXAMPLES "Build examples" OFF)
option(DMCP_BUILD_INTEGRATION_TESTS "Build integration tests" OFF)
option(DMCP_BUILD_ADAPTER_FAKE "Build fake adapter" OFF)
option(DMCP_BUILD_ADAPTER_ZDOOM "Build ZDoom adapter" OFF)
option(DMCP_BUILD_ADAPTER_CRISPY "Build Crispy Doom adapter" OFF)
option(DMCP_BUILD_SHARED "Build shared libraries" OFF)
option(DMCP_BUILD_SINGLE_DLL "Build single libdmcp artifact" ON)
option(DMCP_ENABLE_SANITIZERS "Enable sanitizers" OFF)

# Build shared libraries
cmake -B build/shared -DDMCP_BUILD_SHARED=ON -DDMCP_BUILD_TESTS=OFF
cmake --build build/shared --parallel
```

## Key Design Principles

1. **Separation of Concerns**: Each layer has one job
2. **Dependency Inversion**: Upper layers depend on abstractions, not implementations
3. **Minimal Public API**: Only expose what's necessary
4. **Object Pooling for Reduced Allocations**: Reuses snapshot objects to minimize memory allocation overhead
5. **C API at Boundaries**: Public integration APIs are C99-compatible
6. **Optional Components**: Adapters are opt-in via CMake options
7. **Unified Error Types**: All DMCP functions use `mcp_status_t` from MCP layer
8. **Shared Adapter Utilities**: Common conversions and validations live in `include/dmcp/adapter/`
9. **Library Flexibility**: Support both static and shared library builds
10. **Stable JSON Boundary**: Doom core uses `internal/json_types.hpp` wrapper, never imports generic JSON internals directly
11. **Content API Ownership**: Content availability APIs (`include/dmcp/doom/content.h`) owned by Doom core, not adapters
12. **Flat Tool Groups**: Doom exposes simple game/input tool groups instead of a vtable plugin system
13. **No-Game Validation**: Core protocol, layer boundaries, Doom context, and fake adapter behavior can be tested without launching Crispy Doom

## No-Game Validation

Use no-game checks for CI and contributor machines that do not have a WAD or
engine binary available:

```bash
cmake -B build/default \
  -DDMCP_BUILD_TESTS=ON \
  -DDMCP_BUILD_INTEGRATION_TESTS=ON \
  -DDMCP_BUILD_ADAPTER_FAKE=ON
cmake --build build/default --parallel
ctest --test-dir build/default -L "unit|no_game" -LE "requires_game|headless|e2e" --output-on-failure
```

This covers unit tests in `tests/unit/`, the public C header smoke test, and
C/C++ integration tests that start the deterministic fake adapter and MCP
HTTP/SSE transport without launching a game. API/context tests set
`start_transport=false` where the HTTP/SSE listener is not the behavior under
test; runtime defaults still start transport for real engines.

Engine-backed validation is separate and opt-in. It may use Crispy Doom,
`tests/integration/run_headless.sh`, and an IWAD, and should be guarded behind
the engine e2e build/test path rather than default CI.

---

## Tool Groups

DMCP exposes two first-party tool groups. They are configuration flags, not
runtime plugin objects. The Doom MCP integration layer registers the MCP
surface directly, then `tools/list` and `tools/call` consult these flags.

| Group | Tools | Purpose |
|-------|-------|---------|
| **game** | `get_*` state tools, direct command tools, `execute_batch`, `get_screenshot` | Game state management and entity manipulation |
| **input** | `player_input` | Human-like input control for agents |

### Configuration

Tool groups are configured via `dmcp_config_t`:

```c
typedef struct {
    // ... other fields ...

    struct {
        bool game;   // Enable state and command tools (default: true)
        bool input;  // Enable player_input (default: true)
    } tools;

    bool start_transport;   // Start HTTP/SSE MCP endpoint (default: true)
    
} dmcp_config_t;
```

**Default behavior**: Both tool groups are enabled. Set `config.tools.game = false` or `config.tools.input = false` to hide a group.

### Tool Registration

Tools are exposed through the MCP `tools/list` surface as stable public names
(for example `get_player`, `spawn_entity`, `give_item`, `player_input`).
Public tool calls use JSON-RPC `tools/call` with structured `arguments`;
direct native JSON-RPC method aliases are not registered.

---

## Source Code Organization

The Doom MCP source is organized by concern:

| File/Directory | Responsibility |
|----------------|----------------|
| `context.cpp` | C API implementation |
| `content.cpp` | Content availability and restrictions |
| `handlers/` | MCP tool and HTTP route handlers (modular) |
| `commands/` | Command parsing and per-type implementations |
| `pool.cpp` | Snapshot pool management |
| `serialization.cpp` | JSON serialization of snapshots |
| `internal/json_types.hpp` | Stable JSON boundary wrapper |

The handlers are split into focused modules:
- `common.cpp` - Shared utilities
- `state_json.cpp` - State JSON builders  
- `schema_builders.cpp` - Tool schema definitions
- `tools/` - Individual tool implementations
