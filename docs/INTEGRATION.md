# MCP Tool Integration Guide

This guide shows how to connect DMCP to popular MCP clients and tools.

## Quick Start

DMCP exposes an HTTP endpoint that MCP clients can connect to:

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

First, [start the DMCP server](#starting-the-server), then add the configuration above to your MCP client.

## Table of Contents

- [Starting the Server](#starting-the-server)
- [Codex](#codex)
- [OpenCode](#opencode)
- [Claude Desktop](#claude-desktop)
- [Claude Code (CLI)](#claude-code-cli)
- [VS Code](#vs-code)
  - [Cline](#cline)
  - [Continue](#continue)
- [Generic HTTP Client](#generic-http-client)

---

## Starting the Server

Before connecting any MCP client, you need to run the Doom engine with DMCP enabled.

### Building with DMCP

**Prerequisite**: Build DMCP SDK first:

```bash
# Build DMCP (static library)
cmake -B build/default -DDMCP_BUILD_TESTS=ON
cmake --build build/default --parallel

# Or build shared libraries
cmake -B build/shared -DDMCP_BUILD_SHARED=ON -DDMCP_BUILD_SINGLE_DLL=ON -DDMCP_BUILD_TESTS=OFF
cmake --build build/shared --parallel
```

**Engine integration (recommended)**

```bash
# Fast path (validates hooks + builds DMCP + builds Crispy)
make submodules
make crispy-doom

# Or manual configure/build
cmake -S crispy-doom -B crispy-doom/build \
  -DDMCP_ROOT="$PWD" \
  -DDMCP_ENABLE=ON
cmake --build crispy-doom/build --parallel
```

For external engines, consume DMCP from local source/build artifacts:

```cmake
add_subdirectory(path/to/doom-mcp path/to/doom-mcp-build EXCLUDE_FROM_ALL)
target_link_libraries(myengine PRIVATE dmcp::runtime)
```

The public runtime surface is a C99-compatible API. Use the `include/mcp/*` and `include/dmcp/*`
headers from C or C++ code; C++ wrappers under `include/*/cxx/` are source-level
RAII conveniences over the C API.

### Starting the Server

```bash
# Run with default port 6060
./crispy-doom/build/src/crispy-doom -iwad assets/wads/doom1.wad -dmcp

# Or specify a custom port
./crispy-doom/build/src/crispy-doom -iwad assets/wads/doom1.wad -dmcp -dmcp_port 6061
```

### Verifying the Server

```bash
# Check health endpoint
curl http://localhost:6060/health

# Initialize session
INIT=$(curl -s -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"verify","version":"1.0"}}}')

SESSION_ID=$(printf '%s' "$INIT" | jq -r '.result.sessionId')

# Complete lifecycle
curl -s -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{"jsonrpc":"2.0","method":"notifications/initialized","params":{}}' >/dev/null

# Test MCP endpoint
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
```

The server runs as long as the game is active.

Strict mode is the default: after `initialize`, include both
`MCP-Protocol-Version` and `MCP-Session-Id` on every `/mcp` request.

### No-Game Validation

CI can validate the SDK and client-facing protocol without launching Crispy Doom:

```bash
cmake -B build/default \
  -DDMCP_BUILD_TESTS=ON \
  -DDMCP_BUILD_INTEGRATION_TESTS=ON \
  -DDMCP_BUILD_ADAPTER_FAKE=ON
cmake --build build/default --parallel
ctest --test-dir build/default -L "unit|no_game" -LE "requires_game|headless|e2e" --output-on-failure
```

This runs unit coverage for the Generic MCP layer, Doom MCP layer, layer
boundaries, the public C header smoke check, and the C/C++ fake-adapter MCP
transport integration test. Use engine-backed headless/e2e scripts only when an
engine binary and IWAD are available.

For unit tests or embedders that need a context without opening a listener, set
`mcp_server_config_t.start_transport=false` or
`dmcp_config_t.start_transport=false`. The default is `true`, so normal game
integration still starts the HTTP/SSE MCP endpoint.

---

## Codex

Project-local configuration:

```toml
# .codex/config.toml
[mcp_servers.doom]
url = "http://localhost:6060/mcp"
enabled = true
```

This is the same file shape packaged by the Crispy DMCP release artifact.

---

## OpenCode

Project-local configuration:

```json
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "doom": {
      "type": "remote",
      "url": "http://localhost:6060/mcp",
      "enabled": true,
      "timeout": 10000
    }
  }
}
```

The Crispy DMCP release artifact includes this as `opencode.json`.

---

## Claude Desktop

Claude Desktop is Anthropic's official desktop application with built-in MCP support.

### Configuration

Edit `~/Library/Application Support/Claude/claude_desktop_config.json` (macOS) or `%APPDATA%\Claude\claude_desktop_config.json` (Windows):

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

### Usage

Once the server is running and configured:

1. Restart Claude Desktop
2. Look for the hammer icon in the chat input
3. Ask Claude to interact with Doom:

```
"Show me the current game state"
"Spawn an enemy at position (1000, 500)"
"Take a screenshot of the current view"
```

---

## Claude Code (CLI)

Claude Code is Anthropic's command-line coding assistant.

### Configuration

Create or edit `~/.claude-code/config.json`:

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

### Project-Specific Setup

Add to your project's `.claude-code/config.json`:

```json
{
  "mcpServers": {
    "doom-dev": {
      "type": "http",
      "url": "http://localhost:6060/mcp"
    }
  }
}
```

### Usage

```bash
claude

# Then ask about Doom:
claude > What's my current health in Doom?
claude > Spawn a DoomImp near the player
```

---

## VS Code

### Cline

Cline is a VS Code extension that provides autonomous coding capabilities.

**Configuration** (`~/.vscode/mcp.json` or project `.vscode/mcp.json`):

```json
{
  "mcpServers": {
    "doom": {
      "url": "http://localhost:6060/mcp",
      "disabled": false,
      "autoApprove": ["get_player", "get_map", "get_screenshot"]
    }
  }
}
```

**Usage:**
1. Open Cline panel (sidebar icon)
2. Type requests like:

```
Check the current Doom game state
Spawn a health pack at my location
Take a screenshot
```

### Continue

Continue is an open-source AI coding assistant for VS Code.

**Configuration** (`~/.continue/config.json`):

```json
{
  "experimental": {
    "modelContextProtocolServers": [
      {
        "transport": {
          "type": "http",
          "url": "http://localhost:6060/mcp"
        }
      }
    ]
  }
}
```

**Project-specific:** `.continue/config.json`

```json
{
  "experimental": {
    "modelContextProtocolServers": [
      {
        "transport": {
          "type": "http",
          "url": "http://localhost:6060/mcp"
        }
      }
    ]
  }
}
```

**Usage:**
1. Open Continue chat panel
2. Ask questions like:

```
@doom What's the current game state?
@doom Spawn 3 enemies around the player
```

---

## Generic HTTP Client

For custom integrations or testing without an MCP client library:

```python
import requests
import json

class DMCPSimpleClient:
    def __init__(self, base_url="http://localhost:6060"):
        self.base_url = base_url
        self.mcp_url = f"{base_url}/mcp"
        self.session_id = None

    def initialize(self):
        response = requests.post(
            self.mcp_url,
            json={
                "jsonrpc": "2.0",
                "id": 1,
                "method": "initialize",
                "params": {
                    "protocolVersion": "2025-11-25",
                    "capabilities": {},
                    "clientInfo": {"name": "dmcp-simple-client", "version": "1.0.0"},
                },
            },
            headers={"MCP-Protocol-Version": "2025-11-25"},
        )
        response.raise_for_status()
        data = response.json()
        self.session_id = data["result"]["sessionId"]

        requests.post(
            self.mcp_url,
            json={"jsonrpc": "2.0", "method": "notifications/initialized", "params": {}},
            headers={
                "MCP-Protocol-Version": "2025-11-25",
                "MCP-Session-Id": self.session_id,
            },
        ).raise_for_status()

    def _headers(self):
        return {
            "MCP-Protocol-Version": "2025-11-25",
            "MCP-Session-Id": self.session_id,
        }
    
    def get_state(self):
        """Get current game state"""
        response = requests.post(self.mcp_url, json={
            "jsonrpc": "2.0",
            "id": 1,
            "method": "tools/call",
            "params": {"name": "get_state", "arguments": {"section": "player"}}
        }, headers=self._headers())
        result = response.json()
        if "result" in result and "content" in result["result"]:
            return json.loads(result["result"]["content"][0]["text"])
        return result
    
    def call_tool(self, name, arguments=None):
        """Call a DMCP MCP tool."""
        response = requests.post(self.mcp_url, json={
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/call",
            "params": {
                "name": name,
                "arguments": arguments or {}
            }
        }, headers=self._headers())
        return response.json()
    
    def health_check(self):
        """Check server health"""
        response = requests.get(f"{self.base_url}/health")
        return response.json()

# Usage
client = DMCPSimpleClient()
client.initialize()
print(client.health_check())
print(client.get_state())
client.call_tool("spawn_entity", {
    "entity_class": "DoomImp",
    "x": 1000,
    "y": 500,
    "angle": 0
})
```

---

## Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `DMCP_PORT` | HTTP server port | 6060 |

`DMCP_TARGET_HZ` and `DMCP_LOG_LEVEL` are configured in code/config structs,
not as runtime environment variables.

---

## Available Tools

| Tool | Description |
|------|-------------|
| `get_player` | Get player-only state |
| `get_enemies` | Get enemy list (paginated, `status=alive|dead|all`) |
| `get_entities` | Get enemies and world entities (paginated, `kind`, `status`) |
| `get_items` | Get world pickups/items only (paginated, `kind`) |
| `get_map` | Get current map/level state |
| `get_inventory` | Get inventory list (paginated) |
| `get_game_info` | Get game mode/version metadata |
| `get_available_content` | Get the complete available-only canonical content catalog |
| `get_available_enemies` | Get available enemy classes |
| `get_available_entities` | Get available spawnable entity classes |
| `get_available_items` | Get available item classes |
| `get_available_weapons` | Get available weapon classes |
| `get_available_ammo` | Get available ammo classes |
| `get_available_keys` | Get available key classes |
| `get_available_maps` | Get available map names |
| `get_available_giveable` | Get available classes accepted by `give_item` |
| `get_state` | Unified section query (`player`, `enemies`, `entities`, `items`, `map`, `inventory`, `game`) |
| `get_state_batch` | Read-only batch query for multiple state sections |
| `get_screenshot` | Capture ASCII screenshot of current view |
| `spawn_entity` | Spawn an available monster or pickup by canonical `entity_class`, `x`, `y`, `angle` |
| `give_item` | Give a canonical weapon, ammo, key, or item by `item_class` and `amount` |
| `change_level` | Change map using canonical `map_name` from `get_available_maps` |
| `set_player_position` | Change player location with `x`, `y`, `angle` |
| `get_command_result` | Poll async command completion by `sequence` |
| `execute_batch` | Queue mutating commands in order (rejects `change_level`) |
| `get_command_examples` | Fetch structured command payload examples |
| `player_input` | Queue one movement/aim/fire/use input action per tick |

### Mutating Tool Arguments

| Tool | Arguments |
|------|-----------|
| `spawn_entity` | `entity_class`, `x`, `y`, `angle` |
| `change_level` | `map_name`, `skill_level` |
| `give_item` | `item_class`, `amount` |
| `set_player_health` | `health` |
| `set_player_position` | `x`, `y`, `angle` |
| `execute_console` | `command` |
| `pause_game` | `paused` |
| `damage_entity` | `target_tid`, `damage` |
| `kill_entity` | `target_tid` |

### Batching and Level Changes

Use separate batch types:

- `get_state_batch` for grouped reads
- `execute_batch` for grouped mutating commands

`execute_batch` runs commands in-order but rejects `change_level` entries.

Use level transitions as a separate step:

1. Send `change_level` as a direct `tools/call`
2. Poll `get_command_result` until completion/success
3. Send follow-up commands (single or batch)

### Command Examples Tool

Use `get_command_examples` to discover valid canonical payload shapes:

```json
{
  "jsonrpc": "2.0",
  "id": 99,
  "method": "tools/call",
  "params": {
    "name": "get_command_examples"
  }
}
```

---

## Troubleshooting

### Server Not Found

```bash
curl http://localhost:6060/health
# Expected: {"status": "ok", "clients": 0}
```

### Port Already in Use

Start the server on a different port:
```bash
./crispy-doom/build/src/crispy-doom -iwad assets/wads/doom1.wad -dmcp -dmcp_port 6061
```

Then update the URL in your MCP config:
```json
{
  "url": "http://localhost:6061/mcp"
}
```

### Tools Not Appearing

1. Verify the server is running: `curl http://localhost:6060/health`
2. Check server logs for errors
3. Restart your MCP client after config changes

---

## See Also

- [API Documentation](README.md) - Complete API reference
- [Architecture Overview](ARCHITECTURE.md) - System design
