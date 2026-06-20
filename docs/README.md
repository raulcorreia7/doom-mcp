# DMCP API Reference

Complete API documentation for the Doom Model Context Protocol SDK.

**Version**: 0.6.0

## Documentation Map

| Document | Purpose |
|----------|---------|
| [QUICKSTART.md](QUICKSTART.md) | Get running in 5 minutes |
| [USAGE.md](USAGE.md) | Common patterns and workflows |
| [FEATURES.md](FEATURES.md) | Complete feature overview |
| [INTEGRATION.md](INTEGRATION.md) | Connect to MCP clients |
| [ARCHITECTURE.md](ARCHITECTURE.md) | System design and layers |
| [MCP_COMPLIANCE.md](MCP_COMPLIANCE.md) | Protocol compliance matrix |
| [MCP_TOOL_AUDIT.md](MCP_TOOL_AUDIT.md) | MCP tool surface relevance and risk review |
| [RUNBOOK.md](RUNBOOK.md) | Documentation freshness and maintenance workflow |
| [CHANGELOG.md](CHANGELOG.md) | Version history |
| [../README.md](../README.md) | Project entry point |

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           DOOM ENGINE (ZDoom, etc.)                          │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                      ADAPTER INTERFACE LAYER (C API)                         │
│                 adapters/crispy-doom/, adapters/zdoom/                     │
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

## Building

### Prerequisites

- CMake 3.25+
- C99 compiler for adapter/public C headers
- C++17 compiler for the SDK implementation
- SDL2 (for adapters)

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  pkg-config \
  git \
  curl \
  jq \
  libsdl2-dev \
  libpng-dev \
  libsamplerate0-dev
```

macOS with Homebrew:

```bash
brew install cmake ninja pkg-config jq sdl2 libpng libsamplerate
```

### Using Makefile (Recommended)

```bash
make help      # Show all targets
make submodules # Init/update Crispy submodule
make all       # Build DMCP + Crispy
make check     # Build + test
make validate  # Validate core defaults + opt-in paths
make run       # Run example server
make debug     # Debug build with sanitizers
make headless  # Run optional engine-backed headless checks
```

### Using CMake Directly

```bash
# Configure
cmake -B build/default -S . -DDMCP_BUILD_TESTS=ON -DDMCP_BUILD_EXAMPLES=ON

# Build
cmake --build build/default --parallel

# Test (run sequentially to avoid port race conditions)
ctest --test-dir build/default -j1

# Run
./build/default/dummy_server
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `DMCP_BUILD_EXAMPLES` | OFF | Build example servers |
| `DMCP_BUILD_TESTS` | OFF | Build test suite |
| `DMCP_BUILD_INTEGRATION_TESTS` | OFF | Build C/C++ no-game integration tests |
| `DMCP_BUILD_ADAPTER_FAKE` | OFF | Build deterministic fake adapter |
| `DMCP_BUILD_ADAPTER_ZDOOM` | OFF | Build ZDoom adapter |
| `DMCP_BUILD_ADAPTER_CRISPY` | OFF | Build Crispy Doom adapter |
| `DMCP_BUILD_SHARED` | OFF | Build shared libraries |
| `DMCP_BUILD_SINGLE_DLL` | ON | Build unified `dmcp` runtime surface (`dmcp.so`/`dmcp.dll`) |
| `DMCP_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer |

## Integration

### Local CMake Integration (Recommended)

DMCP is consumed from local source/build artifacts.
Engine integrations pass the DMCP source/build directories explicitly, as the
Crispy integration does with `DMCP_ROOT` and `DMCP_BUILD_DIR`.

```cmake
add_subdirectory(path/to/doom-mcp path/to/doom-mcp-build EXCLUDE_FROM_ALL)
target_link_libraries(myengine PRIVATE dmcp::runtime)
```

This provides:
- `dmcp::runtime` - Preferred local runtime surface
- `dmcp::doom` - Doom-specific MCP API surface
- `mcp::generic` - Generic MCP protocol layer
- `dmcp::adapter_crispy` - Crispy Doom adapter (if enabled)
- `dmcp::adapter_zdoom` - ZDoom adapter (if enabled)

### Generic MCP (for any game/tool)

```c
#include "mcp/generic/server.h"

// Create server
mcp_server_config_t config = mcp_default_config();
// For registration-only tests, set config.start_transport = false.
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
mcp_status_t result = mcp_server_methods_register(server, methods, 3);
if (result.code != MCP_STATUS_CODE_OK) {
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
#include "dmcp/doom/dmcp.h"  // Convenience header (includes all DMCP headers)
// Or include specific headers:
// #include "dmcp/doom/api.h"
// #include "dmcp/doom/config.h"
// #include "dmcp/doom/types.h"
// #include "dmcp/doom/content.h"  // Content availability APIs

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
// For no-listener tests, set config.start_transport = false.

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
  mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);
  if (result.code != MCP_STATUS_CODE_OK) {
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
#include "dmcp/adapters/zdoom.h"

// Startup
dmcp_zdoom_config_t cfg = dmcp_zdoom_config_default();
cfg.base.port = 8080;  // Optional: override default port

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

### Adapter Helpers (Optional Shared Utilities)

The adapter helpers provide common utilities for engine adapters to reduce code duplication:

```c
#include "dmcp/adapter/utils.h"
#include "dmcp/adapter/entities.h"
#include "dmcp/adapter/validation.h"

// Coordinate conversions
fixed_t fixed_val = player->x;
float world_x = dmcp_fixed_to_float(fixed_val);
float degrees = dmcp_angle_to_degrees(player->angle);
float radians = dmcp_angle_to_radians(player->angle);

// Entity names
const char* name = dmcp_entity_name(MT_POSSESSED);  // "Zombieman"

// Input validation
if (!dmcp_validate_health(health)) {
    // Handle invalid health value
}
```

## API Reference

### Generic MCP API

```c
// Server lifecycle
mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
void          mcp_server_destroy(mcp_server_t* server);
bool          mcp_server_is_running(const mcp_server_t* server);

// Method registration (single and batch)
mcp_status_t mcp_server_method_register(mcp_server_t* server,
                                        const char* method,
                                        mcp_method_handler_t handler,
                                        void* user_data);
mcp_status_t mcp_server_methods_register(mcp_server_t* server,
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
mcp_status_t dmcp_screenshot_submit(dmcp_context_t* ctx,
                                      const dmcp_screenshot_frame_t* frame);

// Utilities
void dmcp_stats_get(const dmcp_context_t* ctx, dmcp_stats_t* stats);
int dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot, char* buffer,
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
void          dmcp_zdoom_destroy(dmcp_zdoom_t* ctx);

// Game loop
mcp_status_t dmcp_zdoom_tick(dmcp_zdoom_t* ctx);

// State
bool          dmcp_zdoom_is_running(const dmcp_zdoom_t* ctx);
void          dmcp_zdoom_get_stats(dmcp_zdoom_t* ctx, dmcp_stats_t* out_stats);

// Command processing
bool          dmcp_zdoom_command_execute(dmcp_zdoom_t* ctx, const dmcp_command_t* cmd);
void          dmcp_zdoom_commands_process(dmcp_zdoom_t* ctx);
```

### Adapter Helpers API

```c
// dmcp/adapter/utils.h - Coordinate and angle conversions
static inline float dmcp_fixed_to_float(int32_t fixed_val);
static inline int32_t dmcp_float_to_fixed(float val);
static inline float dmcp_angle_to_degrees(uint32_t angle);
static inline uint32_t dmcp_degrees_to_angle(float degrees);
static inline float dmcp_angle_to_radians(uint32_t angle);
static inline uint32_t dmcp_radians_to_angle(float radians);

// dmcp/adapter/entities.h - Entity name mappings
static inline const char* dmcp_entity_name(int mobj_type);

// dmcp/adapter/validation.h - Input validation
bool dmcp_validate_health(int32_t health);
```

## Protocol Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/mcp` | POST | MCP protocol handshake |
| `/mcp` | GET | Server-Sent Events stream |
| `/health` | GET | Health check |
| `/game/state` | GET | Current game snapshot as JSON |
| `/game/screenshot` | GET | Latest screenshot payload as JSON |

DMCP exposes tools through JSON-RPC `tools/list` and `tools/call`. Direct
native JSON-RPC method aliases are not part of the public MCP surface.

Tool operations include:

- `get_command_result` for async command completion polling
- `get_state_batch` for read-only grouped state queries
- `execute_batch` for queuing mutating commands in-order
- `get_command_examples` for structured command/example discovery
- `spawn_entity`, `give_item`, `change_level`, `set_player_position`, and
  other mutating command tools
- `player_input` for single-tick player control (movement, aim, attack, use, weapon)

### `player_input` Tool

Send a single player input to control movement and actions. Each input executes for one game tick (35Hz).

| Action | Value |
|--------|-------|
| `forward` | - |
| `backward` | - |
| `strafe_left` | - |
| `strafe_right` | - |
| `turn_left` | - |
| `turn_right` | - |
| `aim` | `value`: 0-360 degrees |
| `attack` | - |
| `use` | - |
| `weapon` | `value`: 1-7 |

Weapon slots: `1` Fist/Chainsaw, `2` Pistol, `3` Shotgun/SuperShotgun,
`4` Chaingun, `5` RocketLauncher, `6` PlasmaRifle, `7` BFG9000.

Example:
```json
{"name": "player_input", "arguments": {"action": "forward"}}
{"name": "player_input", "arguments": {"action": "aim", "value": 90}}
{"name": "player_input", "arguments": {"action": "weapon", "value": 3}}
```

`execute_batch` rejects `change_level`; queue map changes separately with
the direct `change_level` tool, wait for `get_command_result` success, then send follow-up
commands.

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
  char    level_id[DMCP_MAX_LEVEL_ID];
  char    level_name[DMCP_MAX_LEVEL_NAME];
  int32_t kill_count, item_count, secret_count;
} dmcp_level_t;

// Enemy
typedef struct {
  int32_t     id;
  float       hp, max_hp;
  dmcp_vec2_t position;
  char        type[DMCP_MAX_ENEMY_TYPE];
} dmcp_enemy_t;

// Full snapshot
typedef struct {
  dmcp_player_t player;
  dmcp_level_t  level;

  dmcp_enemy_t enemies[DMCP_MAX_ENEMIES];
  uint32_t     enemy_count;

  dmcp_entity_t entities[DMCP_MAX_ENTITIES];
  uint32_t      entity_count;

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
} mcp_status_t;

// Convenience macros
#define MCP_STATUS_OK(msg)       // Create success result
#define MCP_STATUS_ERROR(code, msg)  // Create error result

// Check result
mcp_status_t result = some_function(...);
if (result.code != MCP_STATUS_CODE_OK) {
  printf("Error: %s\n", result.message);
}
```

Result codes:
- `MCP_STATUS_CODE_OK` (0) - Success
- `MCP_STATUS_CODE_INVALID_ARGS` (-1) - Invalid arguments
- `MCP_STATUS_CODE_ENCODING_FAILED` (-2) - JSON encoding/decoding failed
- `MCP_STATUS_CODE_DISABLED` (-3) - Operation disabled
- `MCP_STATUS_CODE_QUEUE_FULL` (-4) - Queue is full
- `MCP_STATUS_CODE_NOT_FOUND` (-5) - Resource/method not found
- `MCP_STATUS_CODE_INTERNAL` (-6) - Internal error

## Design Principles

1. **Separation of Concerns**: Each layer has one job
2. **Dependency Inversion**: Upper layers depend on abstractions
3. **Minimal Public API**: Only expose what's necessary
4. **Object Pooling for Reduced Allocations**: Reuses snapshot objects to minimize memory allocation overhead
5. **C API at Boundaries**: All public APIs are C-compatible
6. **Optional Components**: Adapters are opt-in
7. **Type-Oriented Naming**: Consistent `{namespace}_{type}_{action}` pattern
8. **Rich Error Messages**: All errors include human-readable descriptions

## License

MIT License - See LICENSE file for details
