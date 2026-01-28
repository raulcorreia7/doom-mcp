# Doom Model Context Protocol (DMCP) SDK

A clean, modular SDK for integrating Doom-family engines with AI agents via the Model Context Protocol (MCP).

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           DOOM ENGINE (ZDoom, etc.)                          │
└─────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                      ADAPTER INTERFACE LAYER (C API)                         │
│                    adapters/zdoom/adapter.{h,cpp}                           │
└─────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         DOOM MCP LAYER (Game-Specific)                       │
│                      include/dmcp/doom/api.h                                │
└─────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                     GENERIC MCP LAYER (Protocol & Transport)                 │
│                      include/mcp/generic/server.h                           │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Layer Responsibilities

| Layer | Namespace/Library | Responsibility |
|-------|-------------------|----------------|
| **Generic MCP** | `mcp::generic` | Protocol implementation, HTTP/SSE transport, JSON-RPC handling |
| **Doom MCP** | `dmcp::core` | Game state types, serialization, screenshot handling |
| **Adapter** | N/A (engine-specific) | Bridge between engine internals and DMCP types |
| **Engine** | N/A | Raw game state and execution |

### Dependency Rules

- **Generic MCP**: No dependencies (pure protocol)
- **Doom MCP**: Depends only on Generic MCP
- **Adapter**: Depends on Doom MCP and engine headers
- **Engine**: Only includes adapter headers

## Building

### Prerequisites

- CMake 3.25+
- C++17 compiler
- Optional: ccache for faster rebuilds

### Quick Start

```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Run example
./build/dummy_server
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `DMCP_BUILD_EXAMPLES` | ON | Build example servers |
| `DMCP_BUILD_ADAPTER_ZDOOM` | OFF | Build ZDoom adapter |
| `DMCP_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer |
| `DMCP_USE_CCACHE` | ON | Use ccache if available |

## Integration

### Generic MCP (for any game/tool)

```c
#include "mcp/generic/server.h"

// Create server
mcp_server_config_t config = mcp_default_config();
mcp_server_t* server = mcp_server_create(&config);

// Register method handler
bool OnGetState(void* user, const char* method, const char* params,
                char* response, size_t response_size) {
  // Handle the method...
  strcpy(response, "{\"result\":{...}}");
  return true;
}

mcp_server_register_method(server, "tools/call", OnGetState, my_data);

// Broadcast events
mcp_server_broadcast(server, "state", "{\"hp\":100}");

// Cleanup
mcp_server_destroy(server);
```

### Doom MCP (Doom-family engines)

```c
#include "dmcp/doom/api.h"

// Fill snapshot in your game loop
void SnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
  snapshot->player.hp = player->health;
  snapshot->player.position.x = player->x;
  // ... fill other fields
}

// Setup and run
dmcp_config_t config = dmcp_default_config();
config.port = 6060;
config.on_snapshot = SnapshotCallback;

dmcp_context_t* ctx = dmcp_create(&config);

// In game loop
dmcp_tick(ctx);

// Cleanup
dmcp_destroy(ctx);
```

### ZDoom Adapter (zero-config integration)

```cpp
#include "adapters/zdoom/adapter.h"

// Startup
dmcp_zdoom_t* mcp = dmcp_zdoom_create(nullptr);

// In G_Ticker()
dmcp_zdoom_tick(mcp);

// Shutdown
dmcp_zdoom_destroy(mcp);
```

## API Reference

### Generic MCP API

```c
// Server lifecycle
mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
void          mcp_server_destroy(mcp_server_t* server);
bool          mcp_server_is_running(const mcp_server_t* server);

// Method registration
mcp_result_t mcp_server_register_method(mcp_server_t* server,
                                        const char* method,
                                        mcp_method_handler_t handler,
                                        void* user_data);

// Broadcasting
void mcp_server_broadcast(mcp_server_t* server,
                          const char* event_type,
                          const char* json_payload);
```

### Doom MCP API

```c
// Lifecycle
dmcp_context_t* dmcp_create(const dmcp_config_t* config);
void            dmcp_destroy(dmcp_context_t* ctx);

// Game loop
void dmcp_tick(dmcp_context_t* ctx);

// Screenshots
bool dmcp_screenshot_requested(const dmcp_context_t* ctx);
dmcp_result_t dmcp_submit_screenshot(dmcp_context_t* ctx,
                                     const dmcp_screenshot_frame_t* frame);

// Utilities
void dmcp_snapshot_clear(dmcp_snapshot_t* snapshot);
bool dmcp_snapshot_add_enemy(dmcp_snapshot_t* snapshot, const dmcp_enemy_t* enemy);
bool dmcp_snapshot_add_item(dmcp_snapshot_t* snapshot, const dmcp_item_t* item);
```

## Protocol Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/mcp` | POST | MCP protocol handshake |
| `/sse` | GET | Server-Sent Events stream |
| `/health` | GET | Health check |

## Data Types

```c
// 2D vector
typedef struct { float x, y; } dmcp_vec2_t;

// Player state
typedef struct {
  float hp, armor;
  dmcp_vec2_t position;
  int32_t ammo;
} dmcp_player_t;

// Level state
typedef struct {
  int32_t tic;
  char level_id[32];
  char level_name[96];
  int32_t kill_count, item_count, secret_count;
} dmcp_level_t;

// Enemy
typedef struct {
  int32_t id;
  float hp, max_hp;
  dmcp_vec2_t position;
  char type[128];
} dmcp_enemy_t;

// Full snapshot
typedef struct {
  dmcp_player_t player;
  dmcp_level_t level;
  dmcp_enemy_t enemies[256];
  uint32_t enemy_count;
  dmcp_item_t inventory[64];
  uint32_t inventory_count;
} dmcp_snapshot_t;
```

## Design Principles

1. **Separation of Concerns**: Each layer has one job
2. **Dependency Inversion**: Upper layers depend on abstractions
3. **Minimal Public API**: Only expose what's necessary
4. **Zero-Copy Where Possible**: Object pools and pointer passing
5. **C API at Boundaries**: All public APIs are C-compatible
6. **Optional Components**: Adapters are opt-in

## License

MIT License - See LICENSE file for details
