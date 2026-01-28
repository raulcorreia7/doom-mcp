# Doom Model Context Protocol (DMCP) SDK

A clean, modular SDK for integrating Doom-family engines with AI agents via the Model Context Protocol (MCP).

## Migration Guide (v0.5.0 Breaking Changes)

**📖 See [MIGRATION.md](MIGRATION.md) for the comprehensive migration guide with detailed examples and troubleshooting.**

This section provides a quick reference. For complete before/after examples, troubleshooting, and version notes, see the full [MIGRATION.md](MIGRATION.md).

This guide helps you migrate from the old API to the new, consistent type-oriented naming convention.

### Type-Oriented Naming Convention

All APIs now follow a **Type-Oriented** naming pattern:

```
{namespace}_{type}_{action}
```

- `namespace`: Library prefix (`mcp_`, `dmcp_`)
- `type`: The primary type being operated on (`server`, `context`, `config`, `method`, `event`)
- `action`: The operation being performed (`create`, `destroy`, `register`, `broadcast`, `get`)

### Quick Reference Table

| Old API | New API | Change Type |
|----------|-----------|-------------|
| `dmcp_create()` | `dmcp_context_create()` | Renamed |
| `dmcp_destroy()` | `dmcp_context_destroy()` | Renamed |
| `dmcp_tick()` | `dmcp_context_tick()` | Renamed |
| `dmcp_is_running()` | `dmcp_context_is_running()` | Renamed |
| `dmcp_get_stats()` | `dmcp_stats_get()` | Renamed |
| `dmcp_screenshot_requested()` | `dmcp_screenshot_is_requested()` | Renamed |
| `dmcp_submit_screenshot()` | `dmcp_screenshot_submit()` | Renamed |
| `dmcp_default_config()` | `dmcp_config_default()` | Renamed |
| `dmcp_zdoom_default_config()` | `dmcp_zdoom_config_default()` | Renamed |
| `dmcp_zdoom_execute_command()` | `dmcp_zdoom_command_execute()` | Renamed |
| `dmcp_zdoom_process_commands()` | `dmcp_zdoom_commands_process()` | Renamed |
| `mcp_server_register_method()` | `mcp_server_method_register()` | Renamed |
| `mcp_server_broadcast()` | `mcp_server_event_broadcast()` | Renamed |
| `mcp_server_has_clients()` | `mcp_server_clients_count()` | Renamed, returns `uint64_t` instead of `bool` |
| `mcp_server_get_stats()` | `mcp_server_stats_get()` | Renamed |
| `mcp_result_t` (enum) | `mcp_result_t` (struct) | Type changed |

### Error Handling Migration

**BEFORE:**
```c
if (mcp_server_register_method(...) != MCP_OK) {
    fprintf(stderr, "Error: %d\n", result);
}
```

**AFTER:**
```c
mcp_result_t result = mcp_server_method_register(...);
if (result.code != MCP_RESULT_CODE_OK) {
    fprintf(stderr, "Error: %s\n", result.message);
}
```

The `mcp_result_t` type is now a struct with `code` and `message` fields instead of an enum. Use `result.code` for comparisons and `result.message` for human-readable error descriptions.

### Detailed Migration Examples

**1. Context Creation:**
```c
// Before
#include "dmcp/doom/api.h"
dmcp_config_t config = dmcp_default_config();
dmcp_context_t* ctx = dmcp_create(&config);

// After
#include "dmcp/doom/api.h"
dmcp_config_t config = dmcp_config_default();
dmcp_context_t* ctx = dmcp_context_create(&config);
```

**2. Game Loop Integration:**
```c
// Before
dmcp_tick(ctx);

// After
dmcp_context_tick(ctx);
```

**3. Generic MCP Method Registration:**
```c
// Before
mcp_server_register_method(server, "tools/call", OnGetState, my_data);

// After
mcp_server_method_register(server, "tools/call", OnGetState, my_data);
```

**4. Event Broadcasting:**
```c
// Before
mcp_server_broadcast(server, "state", "{\"hp\":100}");

// After
mcp_server_event_broadcast(server, "state", "{\"hp\":100}");
```

**5. Client Count:**
```c
// Before (boolean)
if (mcp_server_has_clients(server)) { ... }

// After (actual count)
uint64_t count = mcp_server_clients_count(server);
if (count > 0) { ... }
```

**6. ZDoom Configuration:**
```c
// Before
dmcp_zdoom_config_t cfg = dmcp_zdoom_default_config();

// After
dmcp_zdoom_config_t cfg = dmcp_zdoom_config_default();
```

**7. Result Handling:**
```c
// Before (enum comparison)
if (mcp_server_method_register(...) != MCP_OK) {
    // No error message available
}

// After (struct comparison)
mcp_result_t result = mcp_server_method_register(...);
if (result.code != MCP_RESULT_CODE_OK) {
    fprintf(stderr, "Error: %s\n", result.message);
}
```

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           DOOM ENGINE (ZDoom, etc.)                          │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                      ADAPTER INTERFACE LAYER (C API)                         │
│                    adapters/zdoom/adapter.{h,cpp}                           │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         DOOM MCP LAYER (Game-Specific)                       │
│                      include/dmcp/doom/api.h                                │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                     GENERIC MCP LAYER (Protocol & Transport)                 │
│                      include/mcp/generic/server.h                           │
└─────────────────────────────────────────────────────────────────────────────────────┘
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

### Type-Oriented Naming Convention

All APIs follow a **Type-Oriented** naming pattern for consistency:

```
{namespace}_{type}_{action}
```

- `namespace`: Library prefix (`mcp_`, `dmcp_`)
- `type`: The primary type being operated on (`server`, `context`, `config`, `method`, `event`)
- `action`: The operation being performed (`create`, `destroy`, `register`, `broadcast`, `get`)

Examples:
- `mcp_server_create()`, `mcp_server_method_register()`, `mcp_server_event_broadcast()`
- `dmcp_context_create()`, `dmcp_context_tick()`, `dmcp_stats_get()`

See [Migration Guide](#migration-guide-v050-breaking-changes) below for complete API changes.

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

mcp_server_method_register(server, "tools/call", OnGetState, my_data);

// Batch register multiple methods (atomic operation)
mcp_method_registration_t methods[] = {
  {"tools/list", OnListTools, NULL},
  {"tools/call", OnCallTool, NULL},
  {"resources/list", OnListResources, NULL}
};
mcp_result_t result = mcp_server_methods_register(server, methods, 3);
if (result.code != MCP_RESULT_CODE_OK) {
  fprintf(stderr, "Registration failed: %s\n", result.message);
}

// Broadcast events
mcp_server_event_broadcast(server, "state", "{\"hp\":100}");

// Get client count (actual number, not boolean)
uint64_t clients = mcp_server_clients_count(server);
printf("Connected clients: %llu\n", (unsigned long long)clients);

// Get statistics
mcp_server_stats_t stats;
mcp_server_stats_get(server, &stats);
printf("Requests handled: %llu\n", stats.requests_handled);

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
  snapshot->player.position.y = player->y;
  snapshot->player.ammo = player->ammo;
  // ... fill other fields
}

// Setup and run
dmcp_config_t config = dmcp_config_default();
config.port = 6060;
config.target_hz = 10;  // Snapshot rate limit
config.on_snapshot = SnapshotCallback;

dmcp_context_t* ctx = dmcp_context_create(&config);
if (!ctx) {
  fprintf(stderr, "Failed to create DMCP context\n");
  return -1;
}

// In game loop
dmcp_context_tick(ctx);

// Screenshot handling
if (dmcp_screenshot_is_requested(ctx)) {
  uint8_t* pixels = CaptureScreenshot();
  dmcp_screenshot_frame_t frame = {
    .pixels = pixels,
    .width = width,
    .height = height,
    .stride = width * 4  // RGBA = 4 bytes per pixel
  };
  dmcp_result_t result = dmcp_screenshot_submit(ctx, &frame);
  if (result.code != DMCP_RESULT_CODE_OK) {
    fprintf(stderr, "Screenshot submit failed: %s\n", result.message);
  }
  free(pixels);  // Safe to free after submit
}

// Get statistics
dmcp_stats_t stats;
dmcp_stats_get(ctx, &stats);
printf("Dropped snapshots: %llu\n", stats.dropped_snapshots);

// Cleanup
dmcp_context_destroy(ctx);
```

### ZDoom Adapter (zero-config integration)

```cpp
#include "adapters/zdoom/adapter.h"

// Startup
dmcp_zdoom_config_t cfg = dmcp_zdoom_config_default();
cfg.port_override = 8080;  // Optional: override default port

dmcp_zdoom_t* mcp = dmcp_zdoom_create(&cfg);

// In G_Ticker()
dmcp_zdoom_tick(mcp);

// State queries
if (dmcp_zdoom_is_running(mcp)) {
  printf("Server is active\n");
}

dmcp_stats_t stats;
dmcp_zdoom_get_stats(mcp, &stats);
printf("Connected clients: %llu\n", stats.connected_clients);

// Command processing
dmcp_zdoom_commands_process(mcp);

// Shutdown
dmcp_zdoom_destroy(mcp);
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

// Method registration (single and batch)
mcp_result_t mcp_server_method_register(mcp_server_t* server,
                                        const char* method,
                                        mcp_method_handler_t handler,
                                        void* user_data);
mcp_result_t mcp_server_methods_register(mcp_server_t* server,
                                          const mcp_method_registration_t* methods,
                                          size_t count);
void          mcp_server_method_unregister(mcp_server_t* server, const char* method);

// Broadcasting
void mcp_server_event_broadcast(mcp_server_t* server,
                                const char* event_type,
                                const char* json_payload);
uint64_t       mcp_server_clients_count(const mcp_server_t* server);

// Statistics
void          mcp_server_stats_get(const mcp_server_t* server,
                                mcp_server_stats_t* stats);

// Utility functions
size_t mcp_format_success_response(char* buffer, size_t buffer_size,
                                    const char* id, const char* result_json);
size_t mcp_format_error_response(char* buffer, size_t buffer_size,
                                  const char* id, int error_code,
                                  const char* error_message);
```

### Doom MCP API

```c
// Lifecycle
dmcp_context_t* dmcp_context_create(const dmcp_config_t* config);
void            dmcp_context_destroy(dmcp_context_t* ctx);
bool            dmcp_context_is_running(const dmcp_context_t* ctx);

// Game loop
void dmcp_context_tick(dmcp_context_t* ctx);

// Screenshots
bool dmcp_screenshot_is_requested(const dmcp_context_t* ctx);
dmcp_result_t dmcp_screenshot_submit(dmcp_context_t* ctx,
                                     const dmcp_screenshot_frame_t* frame);

// Utilities
void dmcp_stats_get(const dmcp_context_t* ctx, dmcp_stats_t* stats);
int  dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot, char* buffer,
                          size_t buffer_size);
void dmcp_snapshot_clear(dmcp_snapshot_t* snapshot);
bool dmcp_snapshot_add_enemy(dmcp_snapshot_t* snapshot, const dmcp_enemy_t* enemy);
bool dmcp_snapshot_add_item(dmcp_snapshot_t* snapshot, const dmcp_item_t* item);
void dmcp_strcpy(char* dest, const char* src, size_t dest_size);
```

### ZDoom Adapter API

```c
// Configuration
dmcp_zdoom_config_t dmcp_zdoom_config_default(void);

// Lifecycle
dmcp_zdoom_t* dmcp_zdoom_create(const dmcp_zdoom_config_t* cfg);
void              dmcp_zdoom_destroy(dmcp_zdoom_t* ctx);

// Game loop
int               dmcp_zdoom_tick(dmcp_zdoom_t* ctx);

// State
bool              dmcp_zdoom_is_running(const dmcp_zdoom_t* ctx);
void              dmcp_zdoom_get_stats(dmcp_zdoom_t* ctx, dmcp_stats_t* out_stats);

// Command processing
bool              dmcp_zdoom_command_execute(dmcp_zdoom_t* ctx, const dmcp_command_t* cmd);
void              dmcp_zdoom_commands_process(dmcp_zdoom_t* ctx);
```

## Protocol Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/mcp` | POST | MCP protocol handshake |
| `/sse` | GET | Server-Sent Events stream |
| `/health` | GET | Health check |
| `/screenshot/latest.png` | GET | Latest screenshot (PNG image) |

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
  char    level_id[MCP_MAX_LEVEL_ID];
  char    level_name[MCP_MAX_LEVEL_NAME];
  int32_t kill_count, item_count, secret_count;
} dmcp_level_t;

// Enemy
typedef struct {
  int32_t     id;
  float       hp, max_hp;
  dmcp_vec2_t position;
  char        type[MCP_MAX_ENEMY_TYPE];
} dmcp_enemy_t;

// Full snapshot
typedef struct {
  dmcp_player_t player;
  dmcp_level_t  level;

  dmcp_enemy_t enemies[DMCP_MAX_ENEMIES];
  uint32_t     enemy_count;

  dmcp_item_t inventory[DMCP_MAX_INVENTORY];
  uint32_t    inventory_count;
} dmcp_snapshot_t;
```

## Error Handling Pattern

All result-returning functions use a consistent error pattern:

```c
typedef struct {
  int32_t     code;     // 0 = success, negative = error
  const char* message;  // Human-readable error message
} mcp_result_t;

// Check result
mcp_result_t result = some_function(...);
if (result.code != MCP_RESULT_CODE_OK) {
  printf("Error: %s\n", result.message);
}
```

Result codes:
- `MCP_RESULT_CODE_OK` (0) - Success
- `MCP_RESULT_CODE_INVALID_ARGS` (-1) - Invalid arguments
- `MCP_RESULT_CODE_ENCODING_FAILED` (-2) - JSON encoding/decoding failed
- `MCP_RESULT_CODE_DISABLED` (-3) - Operation disabled
- `MCP_RESULT_CODE_QUEUE_FULL` (-4) - Queue is full
- `MCP_RESULT_CODE_NOT_FOUND` (-5) - Resource/method not found
- `MCP_RESULT_CODE_INTERNAL` (-6) - Internal error

## Design Principles

1. **Separation of Concerns**: Each layer has one job
2. **Dependency Inversion**: Upper layers depend on abstractions
3. **Minimal Public API**: Only expose what's necessary
4. **Zero-Copy Where Possible**: Object pools and pointer passing
5. **C API at Boundaries**: All public APIs are C-compatible
6. **Optional Components**: Adapters are opt-in
7. **Type-Oriented Naming**: Consistent `{namespace}_{type}_{action}` pattern
8. **Rich Error Messages**: All errors include human-readable descriptions

## License

MIT License - See LICENSE file for details
