# DMCP Architecture (Clean & Modular)

**Version**: 0.6.0  
**Target Architecture**: Engine ↔ Adapter ↔ Doom MCP ↔ Generic MCP Layer

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
│  src/mcp/http_sse_transport.cpp  - HTTP + SSE transport (uWebSockets)       │
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
// adapters/zdoom/adapter.h
dmcp_zdoom_t*        dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg);
mcp_result_generic_t dmcp_zdoom_tick(dmcp_zdoom_t* ctx);
void                 dmcp_zdoom_destroy(dmcp_zdoom_t* ctx);

// adapters/chocolate-doom/dmcp_adapter.h
dmcp_chocolate_t* dmcp_chocolate_create(const dmcp_chocolate_config_t* cfg);
void              dmcp_chocolate_tick(dmcp_chocolate_t* ctx);
void              dmcp_chocolate_destroy(dmcp_chocolate_t* ctx);
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
│       └── commands/
│           ├── parsers.cpp     # Command parsing
│           ├── json_parsers.hpp
│           └── types/          # Per-command implementations
│               ├── spawn.cpp
│               ├── set_health.cpp
│               ├── set_position.cpp
│               ├── change_level.cpp
│               ├── give_item.cpp
│               ├── damage.cpp
│               ├── kill.cpp
│               ├── pause.cpp
│               ├── timescale.cpp
│               └── console.cpp
│
├── adapters/
│   ├── zdoom/
│   │   ├── adapter.h           # Public adapter header
│   │   ├── internal.h          # Shared internal utilities
│   │   ├── adapter.cpp         # ZDoom integration
│   │   └── commands.cpp        # ZDoom command handlers
│   │
│   └── chocolate-doom/
│       ├── dmcp_adapter.h      # Public adapter header
│       ├── dmcp_adapter.c      # Lifecycle: create/destroy/tick
│       ├── dmcp_ascii.h        # ASCII screenshot API
│       ├── dmcp_ascii.c        # ASCII capture implementation
│       ├── dmcp_mappings.h     # Type conversion utilities
│       ├── enemy_types.h       # Enemy lookup table
│       ├── state_player.c      # Player state extraction
│       ├── state_level.c       # Level/game state extraction
│       ├── state_enemies.c     # Enemy + interactive entity enumeration
│       └── commands.c          # Command execution
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
├── chocolate-doom/             # Chocolate Doom submodule
├── crispy-doom/                # Crispy Doom submodule
├── CMakeLists.txt
├── Makefile
├── AGENTS.md
└── README.md
```

## CMake Targets

| Target | Type | Dependencies | Purpose |
|--------|------|--------------|---------|
| `dmcp::generic` | STATIC/SHARED | yyjson, uWebSockets | Generic MCP protocol |
| `dmcp::core` | STATIC/SHARED | dmcp::generic | Doom-specific MCP |
| `dmcp::zdoom` | STATIC | dmcp::core | ZDoom adapter |

## Build Configuration

```cmake
# Options
option(DMCP_BUILD_TESTS "Build tests" OFF)
option(DMCP_BUILD_EXAMPLES "Build examples" ON)
option(DMCP_BUILD_ADAPTER_ZDOOM "Build ZDoom adapter" OFF)
option(DMCP_BUILD_ADAPTER_CHOCOLATE "Build Chocolate Doom adapter" OFF)
option(DMCP_BUILD_SHARED "Build shared libraries" OFF)
option(DMCP_ENABLE_SANITIZERS "Enable sanitizers" OFF)

# Build shared libraries
cmake -B build-shared -DDMCP_BUILD_SHARED=ON -DDMCP_BUILD_TESTS=OFF
cmake --build build-shared -j$(nproc)

# Consumers can do:
find_package(dmcp REQUIRED)
target_link_libraries(myengine PRIVATE dmcp::core)
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
