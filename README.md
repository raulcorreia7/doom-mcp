# DMCP - Doom Model Context Protocol SDK

**Version**: 0.6.0

A C/C++ SDK that exposes Doom game state to AI agents via the Model Context Protocol (MCP).
DMCP exists so engine integrations can keep Doom-specific state and commands out
of generic MCP transport code while still giving agents one stable HTTP/SSE MCP
surface.

## What

DMCP bridges Doom-family engines and AI assistants, enabling:

- Real-time game state access (player, enemies, inventory, level)
- Command execution (spawn entities, change levels, give items)
- Tick-by-tick player control (movement, aiming, attacks)
- Screenshot capture

## Why

| Use Case | Benefit |
|----------|---------|
| AI game playing | Agents read state and send inputs at 35Hz |
| Automated testing | Headless control and verification |
| Research | Low-latency state access for ML training |
| Streaming | Real-time state via HTTP/SSE |

## How

```
AI Agent
  <-> HTTP/SSE MCP
Generic MCP (`include/mcp/generic/`, `src/mcp/`)
  <-> JSON-RPC tools/routes
Doom MCP (`include/dmcp/doom/`, `src/doom/`)
  <-> public C API
Adapter (`adapters/<engine>/`)
  <-> engine hooks
Crispy Doom / another Doom-family engine
```

1. **Agent** sends MCP requests via HTTP POST
2. **DMCP** translates to game commands
3. **Engine** executes and returns state

The public integration boundary is a C99-compatible API (`extern "C"` headers under
`include/`) with opaque handles, explicit destroy functions, and size-versioned
configuration structs. C++ consumers also get source-stable convenience wrappers
over the C API.

## Quick Verify

```bash
# Build + test
make check

# Run example server
make run

# Verify
curl http://localhost:6060/health
```

## Documentation

| Document | Purpose |
|----------|---------|
| [docs/QUICKSTART.md](docs/QUICKSTART.md) | Get running in 5 minutes |
| [docs/USAGE.md](docs/USAGE.md) | Common patterns and workflows |
| [docs/FEATURES.md](docs/FEATURES.md) | Complete feature overview |
| [docs/README.md](docs/README.md) | Full API reference |
| [docs/INTEGRATION.md](docs/INTEGRATION.md) | Connect to MCP clients |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | System design and layers |
| [docs/MCP_COMPLIANCE.md](docs/MCP_COMPLIANCE.md) | Spec compliance status |
| [docs/MCP_TOOL_AUDIT.md](docs/MCP_TOOL_AUDIT.md) | Tool relevance/risk audit |
| [docs/CHANGELOG.md](docs/CHANGELOG.md) | Version history |

## Features

- **Clean public C API**: Public runtime objects use opaque handles and explicit
  ownership functions
- **Source-stable C++ wrappers**: Protocol constants are exposed as C macros and
  C++ `std::string_view` names
- **Modular Architecture**: Separate layers for protocol, game logic, and engine integration
- **Object Pooling**: Reuses snapshot objects to minimize allocations
- **Thread-Safe**: Proper synchronization for concurrent access
- **HTTP/SSE Transport**: Built-in server using cpp-httplib
- **JSON-RPC 2.0**: Full MCP protocol support (2025-11-25)

## Quick Start

```bash
# Using Makefile (recommended)
make check    # Build + test
make run      # Run example server

# Or using CMake directly
cmake -B build/default -DDMCP_BUILD_TESTS=ON -DDMCP_BUILD_EXAMPLES=ON
cmake --build build/default --parallel
ctest --test-dir build/default
./build/default/dummy_server

# Build shared libraries (.so/.dll)
cmake -B build/shared -DDMCP_BUILD_SHARED=ON -DDMCP_BUILD_TESTS=OFF
cmake --build build/shared --parallel
```

## Makefile Targets

```bash
make help          # Show all targets
make dmcp          # Build DMCP core (default)
make submodules    # Init/update Crispy submodule
make all           # Build DMCP + Crispy
make crispy-doom   # Build Crispy Doom with DMCP
make debug         # Build with sanitizers
make test          # Run unit tests
make test-unit     # Run unit tests only
make test-smoke    # Run no-game fake-adapter MCP smoke
make test-integration # Run C/C++ no-game integration suite
make test-e2e      # Run optional engine-backed e2e checks
make check         # Build + test (full verification)
make validate      # Validate core defaults + opt-in paths
make download-wad  # Download DOOM shareware
make headless      # Run optional engine-backed headless checks
make run           # Run dummy server
make compdb        # Generate compile_commands.json for clangd/LSP
make format        # Format source code
make clean         # Remove build directory
```

No-game validation:
- `make test-unit` validates protocol, Doom context/tool groups, and layer boundary
  rules without starting a Doom engine.
- `DMCP_BUILD_ADAPTER_FAKE=ON` builds the deterministic fake adapter used by
  no-game integration coverage when a real engine is not available.
- `DMCP_BUILD_INTEGRATION_TESTS=ON` builds the C/C++ fake-adapter transport
  checks. These tests exercise HTTP/SSE and MCP lifecycle behavior without a
  WAD or game process.
- Unit helpers set `mcp_server_config_t.start_transport=false` and
  `dmcp_config_t.start_transport=false` where live HTTP/SSE is not under test;
  the public default remains `true` for real embedders.
- Optional engine-backed e2e/headless checks are separate and may require a
  Crispy Doom binary plus an IWAD. They are not the default CI/test path.

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `DMCP_BUILD_EXAMPLES` | OFF | Build example servers |
| `DMCP_BUILD_TESTS` | OFF | Build test suite |
| `DMCP_BUILD_INTEGRATION_TESTS` | OFF | Build integration test targets |
| `DMCP_BUILD_ADAPTER_FAKE` | OFF | Build fake adapter (smoke/tests) |
| `DMCP_BUILD_ADAPTER_ZDOOM` | OFF | Build ZDoom adapter |
| `DMCP_BUILD_ADAPTER_CRISPY` | OFF | Build Crispy Doom adapter |
| `DMCP_BUILD_SHARED` | OFF | Build shared libraries (`.so`/`.dll`) |
| `DMCP_BUILD_SINGLE_DLL` | ON | Build unified `dmcp` shared library (`dmcp.so`/`dmcp.dll`) instead of split shared libs |
| `DMCP_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer |

Notes:
- Default CMake configure is core-only (`src/` + `include/` APIs), with adapters opt-in.
- Local source/build artifacts are the supported integration path.
- Adapter targets need engine-specific include paths/generated headers.

## Basic Usage

```c
#include "dmcp/doom/dmcp.h"  // Includes api.h, config.h, types.h, content.h, commands.h

void SnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
    snapshot->player.hp = player->health;
    snapshot->player.position.x = player->x;
    snapshot->player.position.y = player->y;
}

int main() {
    dmcp_config_t config = dmcp_config_default();
    config.port = 6060;
    config.on_snapshot = SnapshotCallback;
    
    dmcp_context_t* ctx = dmcp_context_create(&config);
    
    while (running) {
        GameTick();
        dmcp_context_tick(ctx);
    }
    
    dmcp_context_destroy(ctx);
    return 0;
}
```

## Adapters

### ZDoom Adapter (`adapters/zdoom/`)
For GZDoom/ZDoom-based source ports.

### Crispy Doom Adapter (`adapters/crispy-doom/`)
For enhanced vanilla-accurate gameplay with minimal checked-in engine hooks.
Use `make crispy-doom` to validate the hooks and build the game; this build
workflow does not launch Crispy Doom or require a WAD.

If this is a fresh clone, run `make submodules` first.

`make crispy-doom` validates that the pinned Crispy submodule already contains
the minimal DMCP hooks, then builds the DMCP runtime and the game without
launching it.

## API Endpoints

The DMCP server exposes the following HTTP endpoints:

### HTTP Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/mcp` | POST | JSON-RPC 2.0 protocol endpoint for MCP method calls |
| `/mcp` | GET | Server-Sent Events stream for real-time state updates |
| `/health` | GET | Health check endpoint |
| `/game/state` | GET | Current game snapshot as JSON |
| `/game/screenshot` | GET | Latest screenshot payload as JSON (if enabled) |

### Default Configuration

- **Port**: 6060
- **Protocol**: JSON-RPC 2.0
- **Transport**: HTTP + SSE (Server-Sent Events)

## Consumer Setup & Interaction

### Using cURL

#### 1. Health Check

Check if the server is running:

```bash
curl http://localhost:6060/health
```

Response:
```json
{"status": "ok", "clients": 0}
```

#### 2. MCP Protocol - Initialize

Initialize the MCP connection:

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    "params": {
      "protocolVersion": "2025-11-25",
      "capabilities": {},
      "clientInfo": {
        "name": "example-client",
        "version": "1.0.0"
      }
    }
  }'
```

Response:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "protocolVersion": "2025-11-25",
    "capabilities": {
      "tools": {"listChanged": true}
    },
    "serverInfo": {
      "name": "doom-mcp",
      "version": "0.6.0"
    },
    "sessionId": "abc123..."
  }
}
```

Save the returned `sessionId` and send it on every subsequent `/mcp` request with
`MCP-Session-Id`. Requests without a valid session ID are rejected after
`initialize`.

#### 3. Send initialized Notification

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: <SESSION_ID>" \
  -d '{
    "jsonrpc": "2.0",
    "method": "notifications/initialized",
    "params": {}
  }'
```

#### 4. List Available Tools

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: <SESSION_ID>" \
  -d '{
    "jsonrpc": "2.0",
    "id": 2,
    "method": "tools/list"
  }'
```

Response:
```json
{
  "jsonrpc": "2.0",
  "id": "2",
  "result": {
    "tools": [
      {
        "name": "get_player",
        "description": "Get current player state only",
        "inputSchema": {"type": "object", "properties": {}},
        "annotations": {"readOnlyHint": true, "idempotentHint": true}
      },
      {
        "name": "get_screenshot",
        "description": "Capture a screenshot of the current game state",
        "inputSchema": {"type": "object", "properties": {}}
      },
      {"name": "spawn_entity", "description": "Spawn an entity in the current level"},
      {"name": "give_item", "description": "Give an item to the player"},
      {"name": "change_level", "description": "Change to another map/level"},
      {"name": "execute_batch", "description": "Execute multiple mutating commands"}
    ]
  }
}
```

#### 4. Get Player State

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: <SESSION_ID>" \
  -d '{
    "jsonrpc": "2.0",
    "id": 3,
    "method": "tools/call",
    "params": {"name": "get_player"}
  }'
```

Response (truncated):
```json
{
  "jsonrpc": "2.0",
  "id": "3",
  "result": {
    "content": [{
      "type": "text",
      "text": "{\"player\":{\"hp\":100,...}}"
    }]
  }
}
```

#### 5. Execute Command Tool

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: <SESSION_ID>" \
  -d '{
    "jsonrpc": "2.0",
    "id": 4,
    "method": "tools/call",
    "params": {
      "name": "spawn_entity",
      "arguments": {
        "entity_class": "DoomImp",
        "x": 1000,
        "y": 500,
        "angle": 90
      }
    }
  }'
```

DMCP exposes one MCP input route for tools: JSON-RPC `tools/call` with
structured `arguments`. Native JSON-RPC aliases such as `get_player` or
`execute_command` are not part of the public MCP surface.

#### 6. Get Screenshot

```bash
curl http://localhost:6060/game/screenshot
```

#### 7. SSE Stream (Real-time Updates)

Connect to the SSE endpoint for real-time state updates:

```bash
curl http://localhost:6060/mcp
```

Response (SSE format):
```
event: connected
data: {"client_id":"client_0"}

event: state
data: {"player":{"hp":100,...},"level":{...}}

event: state
data: {"player":{"hp":95,...},"level":{...}}
```

### Using an MCP Client (Python Example)

```python
import requests
import json

class DMCPClient:
    def __init__(self, base_url="http://localhost:6060"):
        self.base_url = base_url
        self.mcp_url = f"{base_url}/mcp"
        self.session_id = None
    
    def initialize(self):
        """Initialize MCP connection"""
        payload = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {
                "protocolVersion": "2025-11-25",
                "capabilities": {},
                "clientInfo": {
                    "name": "dmcp-python-example",
                    "version": "1.0.0"
                }
            }
        }
        init_resp = requests.post(
            self.mcp_url,
            json=payload,
            headers={"MCP-Protocol-Version": "2025-11-25"},
        )
        init_resp.raise_for_status()
        data = init_resp.json()
        self.session_id = data["result"]["sessionId"]

        # Required lifecycle step after initialize success
        requests.post(
            self.mcp_url,
            json={"jsonrpc": "2.0", "method": "notifications/initialized", "params": {}},
            headers={
                "MCP-Protocol-Version": "2025-11-25",
                "MCP-Session-Id": self.session_id,
            },
        ).raise_for_status()

        return data
    
    def get_player(self):
        """Get current player state"""
        payload = {
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/call",
            "params": {"name": "get_player"}
        }
        resp = requests.post(
            self.mcp_url,
            json=payload,
            headers={
                "MCP-Protocol-Version": "2025-11-25",
                "MCP-Session-Id": self.session_id,
            },
        )
        result = resp.json()
        
        # Parse the JSON content
        if "result" in result and "content" in result["result"]:
            for item in result["result"]["content"]:
                if item.get("type") == "text":
                    return json.loads(item["text"])
        return result
    
    def health_check(self):
        """Check server health"""
        resp = requests.get(f"{self.base_url}/health")
        return resp.json()

# Usage
client = DMCPClient()
print(client.health_check())  # {'status': 'ok', 'clients': 0}
print(client.get_player())  # Player state
```

### MCP Client Configuration

Connect your MCP client to the running DMCP server.

Codex (`.codex/config.toml`):

```toml
[mcp_servers.doom]
url = "http://localhost:6060/mcp"
enabled = true
```

OpenCode (`opencode.json`):

```json
{
  "mcp": {
    "doom": {
      "type": "remote",
      "url": "http://localhost:6060/mcp",
      "enabled": true
    }
  }
}
```

Claude / generic MCP clients (`.mcp.json` or client config):

```json
{
  "mcpServers": {
    "doom": {
      "type": "http",
      "url": "http://localhost:6060/mcp"
    }
  }
}
```

First start the Doom engine with DMCP enabled (see [docs/INTEGRATION.md](docs/INTEGRATION.md)), then add the relevant configuration to your MCP client.

## Available Tools

### Granular State Tools

For agent loops that need focused, low-overhead reads:

- `get_player` - Player-only state
- `get_enemies` - Enemy list with pagination and status filter (`offset`, `limit`, `status=alive|dead|all`)
- `get_entities` - Enemies and world entities with filters (`offset`, `limit`, `kind=all|enemy|item|weapon|ammo|key|health|armor|powerup|world`, `status=alive|dead|all`)
- `get_items` - World pickups/items only (`offset`, `limit`, `kind=item|weapon|ammo|key|health|armor|powerup`)
- `get_map` - Current map/level details
- `get_inventory` - Inventory list with pagination (`offset`, `limit`)
- `get_game_info` - Runtime mode/version metadata
- `get_available_content` - Complete available-only canonical catalog
- `get_available_enemies` - Available enemy classes only
- `get_available_entities` - Available spawnable entity classes only
- `get_available_items` - Available item classes only
- `get_available_weapons` - Available weapon classes only
- `get_available_ammo` - Available ammo classes only
- `get_available_keys` - Available key classes only
- `get_available_maps` - Available map names only
- `get_available_giveable` - Available classes accepted by `give_item`
- `get_state` - Unified section query (`section: player|enemies|entities|items|map|inventory|game`)
- `get_state_batch` - Read-only batch query for multiple state sections (`requests: [{section,...}]`)
- `get_command_result` - Poll queued command status by `sequence`
- `get_command_examples` - Structured tool examples for all supported commands
- `player_input` - Send player input (movement, aim, attack) - one action per tick

### `get_screenshot`

Returns an ASCII representation of the current game view. Useful for visual debugging.

### Mutating Tools

Queue commands directly through `tools/call` using canonical names and
arguments. Use the granular `get_available_*` tools before content-dependent
commands; they return only canonical names available for the current game mode.

| Tool | Description | Arguments |
|--------------|-------------|------------|
| `spawn_entity` | Spawn an enemy/item | `entity_class`, `x`, `y`, `angle`, `tid` |
| `change_level` | Switch map | `map_name`, `skill_level`, `reset_inventory` |
| `give_item` | Give player a weapon, ammo, key, or item | `item_class`, `amount` |
| `set_player_health` | Set health | `health` |
| `set_player_position` | Move player | `x`, `y`, `angle` |
| `execute_console` | Run engine console command | `command` |
| `pause_game` | Pause/unpause game | `paused` |
| `damage_entity` | Damage specific target | `target_tid`, `damage`, `damage_type` |
| `kill_entity` | Kill specific target | `target_tid` |

### `player_input`

Send a single player input to control movement and actions. Each input executes for one game tick (35Hz). Designed for agent loops that need human-like control.

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

### `execute_batch`

Queue multiple commands in one `tools/call` request using `calls: []`.
Commands run in the order provided.

`change_level` is intentionally rejected in batch mode to avoid running follow-up
commands during a map transition.

Use `execute_batch` only for mutating commands.
Use `get_state_batch` for read-only grouped state fetches.

For level transitions, use this pattern:

1. Queue `change_level` as a direct tool call
2. Wait until `get_command_result` reports `completed=true` and `status=success`
3. Send the next commands (single or `execute_batch`)

### `get_state_batch`

Run multiple read-only state queries in one request:

```json
{
  "requests": [
    {"section": "player"},
    {"section": "enemies", "status": "alive", "limit": 8}
  ]
}
```

### `get_command_examples`

Returns structured examples (including `execute_batch` and `get_state_batch`) so
agents can generate valid command payloads without guessing parameter names.
Command arguments are intentionally canonical; aliases and shorthand fields are
rejected.

## Error Handling

All functions return `mcp_status_t`:

```c
mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);
if (result.code != MCP_STATUS_CODE_OK) {
    fprintf(stderr, "Error: %s\n", result.message);
}
```

JSON-RPC error responses follow the standard format:

```json
{
  "jsonrpc": "2.0",
  "id": "1",
  "error": {
    "code": -32601,
    "message": "Method not found"
  }
}
```

Common error codes:
- `-32600`: Invalid Request
- `-32601`: Method not found
- `-32602`: Invalid params
- `-32603`: Internal error
- `-32000`: Server error

## License

MIT License - See [LICENSE](LICENSE) for details.
