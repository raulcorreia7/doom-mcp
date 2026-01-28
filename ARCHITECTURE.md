# DMCP Architecture (Clean & Modular)

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
│  include/dmcp/doom/schema.h      - Data structures (Snapshot, Player, etc.) │
│  include/dmcp/doom/api.h         - C API for game data                       │
│  src/doom/serializer.cpp         - JSON serialization                        │
│  - Knows about Doom game state                                                │
│  - No networking/transport logic                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                     GENERIC MCP LAYER (Protocol & Transport)                 │
│  include/mcp/generic/protocol.h  - MCP protocol types                        │
│  include/mcp/generic/server.h    - Generic server interface                  │
│  include/mcp/generic/transport.h - Abstract transport (SSE/WebSocket)       │
│  src/mcp/server.cpp              - JSON-RPC + SSE implementation            │
│  src/mcp/sse_transport.cpp       - SSE over HTTP (uWebSockets)              │
│  - No game-specific knowledge                                                 │
│  - Pure MCP protocol implementation                                           │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Dependency Rules

```
Engine ───────► Adapter ───────► Doom MCP ───────► Generic MCP
                    ▲                                ▲
                    │                                │
                    └──────── No back-references ────┘
```

1. **Generic MCP Layer**: No dependencies on anything above it. Pure protocol.
2. **Doom MCP Layer**: Depends only on Generic MCP Layer for registration
3. **Adapter Layer**: Depends on Doom MCP types and engine headers
4. **Engine**: Only touches Adapter public headers

## Public API Surface

### Generic MCP Layer (for any game/tool)
```c
// mcp/generic/server.h
mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
void          mcp_server_destroy(mcp_server_t* server);
bool          mcp_server_register_handler(mcp_server_t* server, 
                                          const char* method,
                                          mcp_handler_fn handler);
void          mcp_server_broadcast(mcp_server_t* server, 
                                   const char* event_type,
                                   const char* json_payload);
```

### Doom MCP Layer (Doom-specific)
```c
// dmcp/doom/api.h
dmcp_context_t* dmcp_create(const dmcp_config_t* config);
void            dmcp_submit_snapshot(dmcp_context_t* ctx, 
                                     const dmcp_snapshot_t* snapshot);
```

### Adapter Layer (Engine-specific)
```c
// adapters/zdoom/adapter.h
dmcp_zdoom_t* dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg);
void          dmcp_zdoom_tick(dmcp_zdoom_t* ctx);
```

## File Structure

```
dmcp/
├── cmake/
│   └── dmcp-config.cmake.in
│
├── include/
│   ├── mcp/
│   │   └── generic/
│   │       ├── protocol.h      # MCP protocol types
│   │       ├── server.h        # Generic server API
│   │       └── transport.h     # Transport abstraction
│   │
│   └── dmcp/
│       └── doom/
│           ├── api.h           # Public C API
│           ├── schema.h        # Data structures
│           └── types.h         # Type definitions
│
├── src/
│   ├── mcp/
│   │   ├── server.cpp          # Generic MCP server
│   │   ├── sse_transport.cpp   # SSE implementation
│   │   ├── jsonrpc.cpp         # JSON-RPC handling
│   │   └── protocol.cpp        # Protocol utilities
│   │
│   └── doom/
│       ├── context.cpp         # Doom MCP context
│       ├── serializer.cpp      # JSON serialization
│       └── pool.cpp            # Snapshot pooling
│
├── adapters/
│   └── zdoom/
│       ├── adapter.h           # Public adapter header
│       └── adapter.cpp         # ZDoom integration
│
├── examples/
│   └── dummy_server.cpp
│
├── tests/
│   └── (unit tests)
│
├── CMakeLists.txt
└── README.md
```

## CMake Targets

| Target | Type | Dependencies | Purpose |
|--------|------|--------------|---------|
| `mcp::generic` | STATIC | uWebSockets, nlohmann_json | Generic MCP protocol |
| `dmcp::core` | STATIC | mcp::generic | Doom-specific MCP |
| `dmcp::zdoom` | INTERFACE | dmcp::core | ZDoom adapter |

## Build Configuration

```cmake
# Options
option(DMCP_BUILD_TESTS "Build tests" ON)
option(DMCP_BUILD_EXAMPLES "Build examples" ON)
option(DMCP_BUILD_ADAPTER_ZDOOM "Build ZDoom adapter" OFF)

# Consumers can do:
find_package(dmcp REQUIRED)
target_link_libraries(myengine PRIVATE dmcp::zdoom)
```

## Key Design Principles

1. **Separation of Concerns**: Each layer has one job
2. **Dependency Inversion**: Upper layers depend on abstractions, not implementations
3. **Minimal Public API**: Only expose what's necessary
4. **Zero-Copy Where Possible**: Use object pools and pointer passing
5. **C API at Boundaries**: All public APIs are C-compatible
6. **Optional Components**: Adapters are opt-in via CMake options
