# MCP Tool Integration Guide

This guide shows how to connect DMCP to popular MCP clients and tools.

## Quick Start

DMCP exposes an HTTP endpoint that MCP clients can connect to:

```json
{
  "mcpServers": {
    "doom": {
      "url": "http://localhost:6060/mcp"
    }
  }
}
```

First, [start the DMCP server](#starting-the-server), then add the configuration above to your MCP client.

## Table of Contents

- [Starting the Server](#starting-the-server)
- [Claude Desktop](#claude-desktop)
- [Claude Code (CLI)](#claude-code-cli)
- [VS Code](#vs-code)
  - [Cline](#cline)
  - [Continue](#continue)
- [Opencode](#opencode)
- [Generic HTTP Client](#generic-http-client)

---

## Starting the Server

Before connecting any MCP client, you need to run the Doom engine with DMCP enabled.

### Building with DMCP

```bash
# Build DMCP first (library used by adapters)
cmake -B build -DDMCP_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"

# Build Chocolate Doom with DMCP enabled
cmake -S chocolate-doom -B chocolate-doom/build \
  -DDMCP_ENABLE=ON \
  -DDMCP_INCLUDE_DIR="$PWD/include" \
  -DDMCP_LIB_DIR="$PWD/build"
cmake --build chocolate-doom/build -j"$(nproc)"

# Alternative: Crispy Doom with tracked DMCP patch
./tests/integration/build_crispy_doom.sh
```

### Starting the Server

```bash
# Run with default port 6060
./chocolate-doom/build/src/chocolate-doom -iwad assets/wads/doom1.wad -dmcp

# Or specify a custom port
./chocolate-doom/build/src/chocolate-doom -iwad assets/wads/doom1.wad -dmcp -dmcp_port 6061
```

### Verifying the Server

```bash
# Check health endpoint
curl http://localhost:6060/health

# Test MCP endpoint
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/list"}'
```

The server runs as long as the game is active.

---

## Claude Desktop

Claude Desktop is Anthropic's official desktop application with built-in MCP support.

### Configuration

Edit `~/Library/Application Support/Claude/claude_desktop_config.json` (macOS) or `%APPDATA%\Claude\claude_desktop_config.json` (Windows):

```json
{
  "mcpServers": {
    "doom": {
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

## Opencode

Opencode is an open-source AI coding assistant with built-in MCP support.

### Configuration

Project: `.opencode/mcp.json`

```json
{
  "mcpServers": {
    "doom": {
      "url": "http://localhost:6060/mcp"
    }
  }
}
```

Global: `~/.config/opencode/mcp.json`

```json
{
  "mcpServers": {
    "doom-dev": {
      "url": "http://localhost:6060/mcp"
    }
  }
}
```

### Auto-Approval

Configure in `.opencode/config.json`:

```json
{
  "autoApproveTools": ["get_player", "get_map", "get_screenshot"]
}
```

### Usage

```
What tools are available for the doom server?
Get the current game state
Spawn an imp near the player
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
    
    def get_state(self):
        """Get current game state"""
        response = requests.post(self.mcp_url, json={
            "jsonrpc": "2.0",
            "id": 1,
            "method": "tools/call",
            "params": {"name": "get_state", "arguments": {"section": "player"}}
        })
        result = response.json()
        if "result" in result and "content" in result["result"]:
            return json.loads(result["result"]["content"][0]["text"])
        return result
    
    def execute(self, cmd_type, params):
        """Execute a command"""
        response = requests.post(self.mcp_url, json={
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/call",
            "params": {
                "name": "execute_command",
                "arguments": {
                    "type": cmd_type,
                    "params": params
                }
            }
        })
        return response.json()
    
    def health_check(self):
        """Check server health"""
        response = requests.get(f"{self.base_url}/health")
        return response.json()

# Usage
client = DMCPSimpleClient()
print(client.health_check())
print(client.get_state())
client.execute("spawn_entity", {
    "entity_class": "DoomImp",
    "position": {"x": 1000, "y": 500}
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
| `get_entities` | Get pickups/barrels only (paginated, excludes projectiles/decor) |
| `get_map`/`get_level` | Get current map/level state |
| `get_inventory` | Get inventory list (paginated) |
| `get_game_info`/`get_game` | Get game mode/version metadata |
| `get_state` | Unified section query (`player`, `enemies`, `entities`, `map`, `inventory`, `game`; optional `status` for enemies) |
| `get_state_batch` | Read-only batch query for multiple state sections |
| `get_screenshot` | Capture ASCII screenshot of current view |
| `execute_command` | Spawn entities, change levels, give items, etc. |
| `get_command_result` | Poll async command completion by `sequence` |
| `execute_batch` | Queue mutating commands in order (rejects `change_level`) |
| `get_command_examples` | Fetch structured command payload examples |

### execute_command Types

| Type | Parameters |
|------|------------|
| `spawn_entity` | `entity_class`, `position.x`, `position.y`, `angle` |
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

1. Send `change_level` with `execute_command`
2. Poll `get_command_result` until completion/success
3. Send follow-up commands (single or batch)

### Command Examples Tool

Use `get_command_examples` to discover valid payload shapes and aliases:

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
./chocolate-doom/build/src/chocolate-doom -iwad assets/wads/doom1.wad -dmcp -dmcp_port 6061
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
