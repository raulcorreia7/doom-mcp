# DMCP API Reference

**Version**: 0.6.0

DMCP is a C/C++ SDK for exposing Doom-family game state and commands through MCP.
The stable public boundary is C99-compatible; C++ helpers are source-level
conveniences over that C API.

## Documentation Map

| Document | Purpose |
|----------|---------|
| [QUICKSTART.md](QUICKSTART.md) | Build and validate the SDK |
| [USAGE.md](USAGE.md) | Common state/tool workflows |
| [FEATURES.md](FEATURES.md) | Feature overview |
| [INTEGRATION.md](INTEGRATION.md) | MCP client setup |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Layer boundaries |
| [MCP_COMPLIANCE.md](MCP_COMPLIANCE.md) | Protocol compliance matrix |
| [MCP_TOOL_AUDIT.md](MCP_TOOL_AUDIT.md) | Tool relevance/risk audit |
| [RUNBOOK.md](RUNBOOK.md) | Documentation maintenance |
| [CHANGELOG.md](CHANGELOG.md) | Version history |

## Architecture

```text
Engine <-> Engine Adapter <-> Doom MCP <-> Generic MCP <-> Core API/runtime
```

| Layer | Responsibility |
|-------|----------------|
| Engine Adapter | Engine-specific state extraction, commands, and input bridge |
| Doom MCP | Doom data types, content catalogs, command parsing, screenshots, tools |
| Generic MCP | MCP lifecycle, JSON-RPC, HTTP/SSE transport, sessions |
| Core | Shared status, version/export macros, string helpers |

Adapter source currently exists for `crispy-doom`, `zdoom`, and the deterministic
`fake` test adapter. Adapter-specific examples live in `adapters/*/README.md`.

## Build

Prerequisites:

- CMake 3.25+
- C99 compiler for public C headers and adapter glue
- C++17 compiler for the SDK implementation
- `curl` and `jq` for manual endpoint checks

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config git curl jq
```

macOS with Homebrew:

```bash
brew install cmake ninja pkg-config jq
```

Recommended local validation:

```bash
make check
make validate
```

Direct CMake:

```bash
cmake -B build/default -S . -DDMCP_BUILD_TESTS=ON -DDMCP_BUILD_EXAMPLES=ON
cmake --build build/default --parallel
ctest --test-dir build/default -j1 --output-on-failure
```

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `DMCP_BUILD_EXAMPLES` | OFF | Build example servers |
| `DMCP_BUILD_TESTS` | OFF | Build unit tests |
| `DMCP_BUILD_INTEGRATION_TESTS` | OFF | Build SDK C/C++ integration tests |
| `DMCP_BUILD_ADAPTER_FAKE` | OFF | Build deterministic fake adapter |
| `DMCP_BUILD_ADAPTER_CRISPY` | OFF | Build crispy-doom adapter |
| `DMCP_BUILD_ADAPTER_ZDOOM` | OFF | Build zdoom adapter |
| `DMCP_BUILD_SHARED` | OFF | Build shared libraries |
| `DMCP_BUILD_SINGLE_DLL` | ON | Build unified `dmcp` runtime surface |
| `DMCP_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer |

## CMake Consumption

```cmake
add_subdirectory(path/to/doom-mcp path/to/doom-mcp-build EXCLUDE_FROM_ALL)
target_link_libraries(myengine PRIVATE dmcp::runtime)
```

Packaged SDK archives provide `cmake/dmcp-config.cmake` and the imported
`dmcp::runtime` target.

## Generic MCP API

```c
#include "mcp/generic/server.h"

mcp_server_config_t mcp_server_config_default(void);
mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
void          mcp_server_destroy(mcp_server_t* server);
mcp_status_t mcp_server_method_register(mcp_server_t* server,
                                        const char* method,
                                        mcp_method_handler_t handler,
                                        void* user_data);
mcp_status_t mcp_server_methods_register(mcp_server_t* server,
                                         const mcp_method_registration_t* methods,
                                         size_t count);
void          mcp_server_event_broadcast(mcp_server_t* server,
                                         const char* event_type,
                                         const char* json_payload);
uint64_t      mcp_server_clients_count(const mcp_server_t* server);
void          mcp_server_stats_get(const mcp_server_t* server,
                                   mcp_server_stats_t* stats);
```

Set `mcp_server_config_t.start_transport=false` for registration-only tests that
must not open a listener.

## Doom MCP API

```c
#include "dmcp/doom/dmcp.h"

dmcp_context_t* dmcp_context_create(const dmcp_config_t* config);
void            dmcp_context_destroy(dmcp_context_t* ctx);
bool            dmcp_context_is_running(const dmcp_context_t* ctx);
void            dmcp_context_tick(dmcp_context_t* ctx);

bool         dmcp_screenshot_is_requested(const dmcp_context_t* ctx);
mcp_status_t dmcp_screenshot_submit(dmcp_context_t* ctx,
                                    const dmcp_screenshot_frame_t* frame);

mcp_status_t dmcp_command_push(dmcp_context_t* ctx, dmcp_command_t* cmd);
bool         dmcp_command_pop(dmcp_context_t* ctx, dmcp_command_t* out_cmd);
bool         dmcp_command_has_pending(const dmcp_context_t* ctx);
uint32_t     dmcp_command_count(const dmcp_context_t* ctx);
void         dmcp_command_clear(dmcp_context_t* ctx);

mcp_status_t dmcp_input_push(dmcp_context_t* ctx, dmcp_command_t* cmd);
bool         dmcp_input_pop(dmcp_context_t* ctx, dmcp_command_t* out_cmd);
bool         dmcp_input_has_pending(const dmcp_context_t* ctx);
uint32_t     dmcp_input_count(const dmcp_context_t* ctx);
void         dmcp_input_clear(dmcp_context_t* ctx);

void dmcp_stats_get(const dmcp_context_t* ctx, dmcp_stats_t* stats);
int  dmcp_snapshot_to_json(const dmcp_snapshot_t* snapshot,
                           char* buffer,
                           size_t buffer_size);
void dmcp_snapshot_clear(dmcp_snapshot_t* snapshot);
bool dmcp_snapshot_add_enemy(dmcp_snapshot_t* snapshot, const dmcp_enemy_t* enemy);
bool dmcp_snapshot_add_item(dmcp_snapshot_t* snapshot, const dmcp_item_t* item);
```

Set `dmcp_config_t.start_transport=false` for tests that only exercise context
state and registration.

## Adapter APIs

Adapter-specific APIs are intentionally documented next to their adapter source:

- `adapters/crispy-doom/README.md`
- `adapters/zdoom/README.md`
- `include/dmcp/adapters/*.h`

## MCP Tool Surface

DMCP exposes tools through JSON-RPC `tools/list` and `tools/call`.

Read tools include `get_player`, `get_enemies`, `get_entities`, `get_items`,
`get_map`, `get_inventory`, `get_game_info`, `get_screenshot`,
`get_state_batch`, and `get_available_content`.

Mutating tools include `spawn_entity`, `give_item`, `change_level`,
`set_player_position`, `player_input`, and `execute_batch`.

Use `get_command_examples` and the `get_available_*` tools to discover canonical
payloads and content names.

## SDK Validation

```bash
cmake -B build/default \
  -DDMCP_BUILD_TESTS=ON \
  -DDMCP_BUILD_INTEGRATION_TESTS=ON \
  -DDMCP_BUILD_ADAPTER_FAKE=ON
cmake --build build/default --parallel
ctest --test-dir build/default -L "unit|sdk" -LE "requires_game" --output-on-failure
```

SDK validation does not launch a game. Engine-backed runtime checks belong in
the consuming engine repository.
