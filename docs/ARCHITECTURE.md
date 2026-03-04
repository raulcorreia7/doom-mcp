# DMCP Architecture (Clean & Modular)

**Version**: 0.6.0  
**Target Architecture**: Engine ↔ Adapter ↔ Doom MCP ↔ Generic MCP Layer

## Layers Explained

### What are the layers?

DMCP is organized into four distinct layers, each with a single responsibility:

| Layer | What it does | Why it exists |
|-------|--------------|---------------|
| **Generic MCP** | HTTP/SSE transport, JSON-RPC protocol | Reusable for any game/tool |
| **Doom MCP** | Game state types, serialization, commands | Doom-specific logic |
| **Adapter** | Engine integration, state extraction | Isolates engine specifics |
| **Engine** | Game execution | The actual Doom port |

### How do they interact?

```
Request Flow (Agent → Engine):
  Agent → HTTP POST → Generic MCP → Doom MCP → Adapter → Engine

Response Flow (Engine → Agent):  
  Engine → Adapter → Doom MCP → Generic MCP → HTTP/SSE → Agent
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

**Dependencies**: yyjson, cpp-httplib (no DMCP dependencies)

#### Doom MCP Layer

**What**: Doom-specific types, serialization, and MCP tools.

**Files**: `src/doom/`, `include/dmcp/doom/`

**Responsibilities**:
- Define game state types (player, enemies, level)
- Serialize state to JSON
- Implement MCP tools (get_player, execute_command, etc.)
- Command parsing and dispatch

**Dependencies**: Generic MCP layer only

#### Adapter Layer

**What**: Engine-specific integration code.

**Files**: `adapters/<engine>/`

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
│                           DOOM ENGINE (ZDoom, etc.)                          │
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
Engine ───────► Adapter ───────► Doom MCP ───────► Generic MCP
                     ▲               ▲
                     │               │
                     └──── Adapter Helpers ──────┘
                     (optional shared utilities)
```

1. **Generic MCP Layer**: No dependencies on anything above it. Pure protocol.
2. **Doom MCP Layer**: Depends only on Generic MCP Layer for registration
3. **Adapter Helpers**: Optional utilities; depends only on standard headers
4. **Adapter Layer**: Depends on Doom MCP types, adapter helpers, and engine headers
5. **Engine**: Only touches Adapter public headers

## Public API Surface

### Generic MCP Layer (for any game/tool)
```c
// mcp/generic/server.h
mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
void          mcp_server_destroy(mcp_server_t* server);
mcp_result_generic_t mcp_server_method_register(mcp_server_t* server,
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
bool dmcp_validate_timescale(float timescale);
```

### Adapter Layer (Engine-specific)
```c
// include/dmcp/adapters/zdoom.h
dmcp_zdoom_t*        dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg);
mcp_result_t         dmcp_zdoom_tick(dmcp_zdoom_t* ctx);
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
│   ├── CHANGELOG.md           # Version history
│   └── plans/                 # Active development plans
│
├── include/
│   ├── mcp/
│   │   └── generic/
│   │       ├── protocol.h     # MCP protocol types
│   │       ├── server.h       # Generic server API
│   │       ├── transport.h    # Transport abstraction
│   │       ├── constants.h    # Buffer sizes, limits
│   │       ├── result.h       # Result type and macros
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
│       │   ├── layer.h         # Layer plugin interface
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
│   ├── mcp/
│   │   ├── server.cpp          # Generic MCP server
│   │   ├── http_sse_transport.cpp # HTTP + SSE implementation
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
│       │   ├── methods.cpp     # JSON-RPC method dispatch
│       │   └── tools/          # Tool implementations
│       │       ├── execute_command.cpp
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
│       │       ├── timescale.cpp
│       │       └── console.cpp
│       └── layers/            # Layer plugin system
│           ├── registry.cpp    # Layer registry implementation
│           ├── registry.hpp    # Layer registry header
│           ├── orchestrator/   # Orchestrator layer
│           │   ├── orchestrator.cpp
│           │   └── orchestrator.hpp
│           └── input/          # Input layer
│               ├── input.cpp
│               └── input.hpp
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
│   ├── integration/            # Integration tests
│   └── e2e/                    # E2E tests (pytest)
│
├── crispy-doom/                # Crispy Doom submodule
├── CMakeLists.txt
├── Makefile
├── AGENTS.md
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
# Option 1: find_package integration (recommended for consumers)
find_package(dmcp CONFIG REQUIRED)
target_link_libraries(myengine PRIVATE dmcp::single)

# Options
option(DMCP_BUILD_TESTS "Build tests" OFF)
option(DMCP_BUILD_EXAMPLES "Build examples" OFF)
option(DMCP_BUILD_ADAPTERS "Enable bundled adapter projects" OFF)
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
5. **C API at Boundaries**: All public APIs are C-compatible
6. **Optional Components**: Adapters are opt-in via CMake options
7. **Unified Error Types**: All DMCP functions use `mcp_result_generic_t` from MCP layer
8. **Shared Adapter Utilities**: Common conversions and validations live in `include/dmcp/adapter/`
9. **Library Flexibility**: Support both static and shared library builds
10. **Stable JSON Boundary**: Doom core uses `internal/json_types.hpp` wrapper, never imports generic JSON internals directly
11. **Content API Ownership**: Content availability APIs (`include/dmcp/doom/content.h`) owned by Doom core, not adapters
12. **Layer Plugin System**: Composable functionality layers that can be enabled/disabled independently

---

## Layer Plugin System

DMCP implements a composable layer/plugin system that splits functionality into independent layers, all sharing a single MCP server with namespaced tools. This enables agents to use only the integration patterns they need.

### Layer Interface

Each layer implements a virtual table (vtable) with registration hooks and lifecycle callbacks:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Layer VTable                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│  name()           → const char*       (layer identifier)                   │
│  description()    → const char*       (human-readable description)        │
│  tool_count()     → size_t            (number of tools)                    │
│  tools()          → const char**       (tool name array)                   │
│  register_methods() → bool             (register JSON-RPC methods)         │
│  register_routes() → bool              (register HTTP routes)             │
│  tick()           → void               (per-frame updates)                 │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Available Layers

| Layer | Tools | Purpose |
|-------|-------|---------|
| **Orchestrator** | `get_*` state tools, `execute_command`, `execute_batch`, command aliases, `get_screenshot` | Game state management and entity manipulation |
| **Input** | `player_input` | Human-like input control for agents |

### Configuration

Layers are configured via `dmcp_config_t`:

```c
typedef struct {
    // ... other fields ...
    
    struct {
        bool orchestrator;  // Enable orchestrator layer (default: true)
        bool input;         // Enable input layer (default: true)
    } layers;
    
} dmcp_config_t;
```

**Default behavior**: Both layers are enabled. Set `config.layers.orchestrator = false` or `config.layers.input = false` to disable specific layers.

### Tool Registration

Tools are grouped internally by layer ownership (orchestrator/input) and exposed through the MCP `tools/list` surface as stable public names (for example `get_player`, `execute_command`, `player_input`).

### Layer Lifecycle

1. **Registration**: Layers register themselves with the global registry at startup
2. **Enable/Disable**: Based on `dmcp_config_t` settings, layers are enabled/disabled
3. **Method Registration**: Enabled layers register their JSON-RPC methods with the MCP server
4. **Tick**: Enabled layers receive per-frame tick callbacks for state updates

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
