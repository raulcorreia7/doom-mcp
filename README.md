# DMCP - Doom Model Context Protocol SDK

**Version**: 0.6.0

A clean, modular C/C++ SDK for integrating Doom-family engines with AI agents via the Model Context Protocol (MCP).

## Features

- **Clean C API**: All public APIs are C-compatible for maximum portability
- **Modular Architecture**: Separate layers for protocol, game logic, and engine integration
- **Object Pooling**: Reuses snapshot objects to minimize allocations
- **Thread-Safe**: Proper synchronization for concurrent access
- **HTTP/SSE Transport**: Built-in server using uWebSockets
- **JSON-RPC 2.0**: Full MCP protocol support

## Documentation

- **[docs/README.md](docs/README.md)** - Full API documentation and examples
- **[docs/INTEGRATION.md](docs/INTEGRATION.md)** - Connect to Claude, Cline, Continue, and other MCP tools
- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** - Architecture overview
- **[docs/CHANGELOG.md](docs/CHANGELOG.md)** - Version history

## Quick Start

```bash
# Using Makefile (recommended)
make check    # Build + test
make run      # Run example server

# Or using CMake directly
cmake -B build -DDMCP_BUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build
./build/dummy_server

# Build shared libraries (.so/.dll)
cmake -B build-shared -DDMCP_BUILD_SHARED=ON -DDMCP_BUILD_TESTS=OFF
cmake --build build-shared -j$(nproc)
```

## Makefile Targets

```bash
make help          # Show all targets
make build         # Build (release)
make debug         # Build with sanitizers
make test          # Run unit tests
make check         # Build + test (full verification)
make download-wad  # Download DOOM shareware
make headless      # Run e2e headless tests
make run           # Run dummy server
make format        # Format source code
make clean         # Remove build directory
```

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `DMCP_BUILD_EXAMPLES` | ON | Build example servers |
| `DMCP_BUILD_TESTS` | OFF | Build test suite |
| `DMCP_BUILD_INTEGRATION_TESTS` | OFF | Build integration test targets |
| `DMCP_BUILD_ADAPTER_ZDOOM` | OFF | Build ZDoom adapter |
| `DMCP_BUILD_ADAPTER_CHOCOLATE` | OFF | Build Chocolate Doom adapter |
| `DMCP_BUILD_SHARED` | OFF | Build shared libraries (`.so`/`.dll`) |
| `DMCP_INSTALL` | ON | Enable install rules for headers/libs |
| `DMCP_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer |

Notes:
- With `DMCP_BUILD_SHARED=OFF`, install copies headers only (no static archives).
- Adapter targets need engine-specific include paths/generated headers.

## Basic Usage

```c
#include "dmcp/doom/dmcp.h"

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

### Chocolate Doom Adapter (`adapters/chocolate-doom/`)
For vanilla-accurate Chocolate Doom. Includes headless testing support.

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
    "params": {}
  }'
```

Response:
```json
{
  "jsonrpc": "2.0",
  "id": "1",
  "result": {
    "protocolVersion": "2025-03-26",
    "capabilities": {
      "notifications": true,
      "tools": {"listChanged": true}
    },
    "serverInfo": {
      "name": "doom-mcp",
      "version": "0.6.0"
    }
  }
}
```

#### 3. List Available Tools

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
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
        "name": "get_game_state",
        "description": "Get current game state including player position, health, enemies",
        "inputSchema": {"type": "object", "properties": {}}
      },
      {
        "name": "get_screenshot",
        "description": "Capture a screenshot of the current game state",
        "inputSchema": {"type": "object", "properties": {}}
      },
      {
        "name": "execute_command",
        "description": "Execute a game command (spawn enemy, change level, etc.)",
        "inputSchema": {
          "type": "object",
          "properties": {
            "type": {"type": "string"},
            "params": {"type": "object"}
          },
          "required": ["type"]
        }
      }
    ]
  }
}
```

#### 4. Get Game State

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 3,
    "method": "tools/call",
    "params": {"name": "get_game_state"}
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
      "text": "{\"player\":{\"hp\":100,...}, \"level\":{...}, \"enemies\":[...]}"
    }]
  }
}
```

#### 5. Execute Command

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 4,
    "method": "tools/call",
    "params": {
      "name": "execute_command",
      "arguments": {
        "type": "spawn_entity",
        "params": {
          "entity_class": "DoomImp",
          "position": {"x": 1000, "y": 500},
          "angle": 90
        }
      }
    }
  }'
```

#### 5b. Direct JSON-RPC Method Aliases (agent compatibility)

The server also accepts direct method calls for agentic clients that do not use
`tools/call` wrappers.

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 41,
    "method": "get_game_state",
    "params": {}
  }'
```

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "execute_command",
    "params": {
      "type": "set_player_health",
      "value": 100
    }
  }'
```

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
    
    def initialize(self):
        """Initialize MCP connection"""
        payload = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {}
        }
        resp = requests.post(self.mcp_url, json=payload)
        return resp.json()
    
    def get_game_state(self):
        """Get current game state"""
        payload = {
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/call",
            "params": {"name": "get_game_state"}
        }
        resp = requests.post(self.mcp_url, json=payload)
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
print(client.get_game_state())  # Full game state
```

### MCP Client Configuration

Connect your MCP client to the running DMCP server:

```json
{
  "mcpServers": {
    "doom": {
      "url": "http://localhost:6060/mcp"
    }
  }
}
```

First start the Doom engine with DMCP enabled (see [docs/INTEGRATION.md](docs/INTEGRATION.md)), then add the configuration above to your MCP client (Claude, Cline, Continue, Opencode, etc.).

## Available Tools

### `get_game_state`

Returns the complete game state including:
- **Player**: Health, armor, position, weapons, ammo, powerups, keys
- **Level**: Current map, time, skill, kill/item/secret counts
- **Game**: Mode (single_player/cooperative/deathmatch), respawn settings
- **Enemies**: Array of visible enemies with position, health, type

### `get_screenshot`

Returns an ASCII representation of the current game view. Useful for visual debugging.

### `execute_command`

Queue a command to be executed by the game:

| Command Type | Description | Parameters |
|--------------|-------------|------------|
| `spawn_entity` | Spawn an enemy/item | `entity_class`, `position.x`, `position.y`, `angle`, `tid` |
| `change_level` | Switch map | `map_name`, `skill_level`, `reset_inventory` |
| `give_item` | Give player an item | `item_class`, `amount` |
| `set_player_health` | Set health | `health` |
| `set_player_position` | Move player | `x`, `y`, `angle` |
| `execute_console` | Run engine console command | `command` |
| `pause_game` | Pause/unpause game | `paused` |
| `set_timescale` | Adjust simulation speed | `scale` |
| `damage_entity` | Damage specific target | `target_tid`, `damage`, `damage_type` |
| `kill_entity` | Kill specific target | `target_tid` |

Agent-friendly aliases accepted by parser:
- `teleport_player` -> `set_player_position`
- `item`/`quantity` -> `item_class`/`amount`
- `level` -> `map_name`
- `value` -> `health`
- `entity`/`class` -> `entity_class`

## Error Handling

All functions return `mcp_result_generic_t`:

```c
mcp_result_generic_t result = dmcp_screenshot_submit(ctx, &frame);
if (result.code != MCP_RESULT_CODE_OK) {
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
