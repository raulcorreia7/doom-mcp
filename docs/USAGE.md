# DMCP Usage Guide

Common patterns for using the Doom MCP SDK.

## Table of Contents

- [Basic Server Setup](#basic-server-setup)
- [Reading Game State](#reading-game-state)
- [Calling Mutating Tools](#calling-mutating-tools)
- [Player Control](#player-control)
- [Screenshots](#screenshots)
- [Batch Operations](#batch-operations)
- [Error Handling](#error-handling)

---

## Basic Server Setup

### C/C++ Integration

```c
#include "dmcp/doom/dmcp.h"

void SnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
    // Fill snapshot from your game state
    snapshot->player.hp = player->health;
    snapshot->player.position.x = player->x;
    snapshot->player.position.y = player->y;
}

int main() {
    dmcp_config_t config = dmcp_config_default();
    config.port = 6060;
    config.target_hz = 10;  // 10 snapshots per second
    config.on_snapshot = SnapshotCallback;
    
    dmcp_context_t* ctx = dmcp_context_create(&config);
    if (!ctx) {
        fprintf(stderr, "Failed to create DMCP context\n");
        return 1;
    }
    
    while (running) {
        GameTick();
        dmcp_context_tick(ctx);
    }
    
    dmcp_context_destroy(ctx);
    return 0;
}
```

### HTTP Client

```python
import requests
import json

class DMCPClient:
    def __init__(self, base_url="http://localhost:6060"):
        self.base_url = base_url
        self.mcp_url = f"{base_url}/mcp"
        self.session_id = None
    
    def initialize(self):
        resp = requests.post(self.mcp_url, json={
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {
                "protocolVersion": "2025-11-25",
                "capabilities": {},
                "clientInfo": {"name": "my-client", "version": "1.0.0"}
            }
        }, headers={"MCP-Protocol-Version": "2025-11-25"})
        result = resp.json()
        self.session_id = result["result"]["sessionId"]
        
        # Send initialized notification
        requests.post(self.mcp_url, json={
            "jsonrpc": "2.0",
            "method": "notifications/initialized"
        }, headers={"MCP-Session-Id": self.session_id, "MCP-Protocol-Version": "2025-11-25"})
        return result
    
    def call_tool(self, name, arguments=None):
        params = {"name": name}
        if arguments:
            params["arguments"] = arguments
        return requests.post(self.mcp_url, json={
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/call",
            "params": params
        }, headers={"MCP-Session-Id": self.session_id, "MCP-Protocol-Version": "2025-11-25"}).json()

client = DMCPClient()
client.initialize()
```

---

## Reading Game State

### Single Section Queries

Initialize once and reuse session headers:

```bash
INIT=$(curl -s -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"usage-docs","version":"1.0"}}}')

SESSION_ID=$(printf '%s' "$INIT" | jq -r '.result.sessionId')
curl -s -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{"jsonrpc":"2.0","method":"notifications/initialized","params":{}}' >/dev/null
```

```bash
# Player only
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_player"}}'

# Enemies (alive only)
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_enemies","arguments":{"status":"alive"}}}'

# Map/level info
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_map"}}'
```

### Batch Queries

Read multiple sections in one request:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "get_state_batch",
    "arguments": {
      "requests": [
        {"section": "player"},
        {"section": "enemies", "status": "alive", "limit": 10},
        {"section": "inventory"}
      ]
    }
  }
}
```

### State Sections

| Section | Description | Pagination |
|---------|-------------|------------|
| `player` | Health, armor, position, ammo | No |
| `enemies` | Enemy list with HP and position | Yes (`offset`, `limit`, `status`) |
| `entities` | Enemies and world entities | Yes (`offset`, `limit`, `kind`, `status`) |
| `items` | World pickups/items only | Yes (`offset`, `limit`, `kind`) |
| `map` | Level name, kill/item/secret counts | No |
| `inventory` | Player inventory items | Yes (`offset`, `limit`) |
| `game` | Game mode, version, skill | No |

---

## Calling Mutating Tools

### Available Tools

Mutating tools are called through JSON-RPC `tools/call` with structured
`params.arguments`.

| Tool | Arguments | Description |
|------|-----------|-------------|
| `spawn_entity` | `entity_class`, `x`, `y`, `angle` | Spawn enemy or item |
| `change_level` | `map_name`, `skill_level`, `reset_inventory` | Switch to new map |
| `give_item` | `item_class`, `amount` | Give player an item |
| `set_player_health` | `health` | Set player health |
| `set_player_position` | `x`, `y`, `angle` | Teleport player |
| `pause_game` | `paused` | Pause/unpause game |
| `damage_entity` | `target_tid`, `damage`, `damage_type` | Damage entity |
| `kill_entity` | `target_tid` | Kill entity instantly |
| `execute_console` | `command` | Run console command |

### Spawn Entity

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
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

### Change Level

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/call",
    "params": {
      "name": "change_level",
      "arguments": {
        "map_name": "E1M2",
        "skill_level": 3
      }
    }
  }'
```

---

## Player Control

The `player_input` tool provides tick-by-tick control (35Hz). Each call executes for one game tick.

### Actions

| Action | Value | Description |
|--------|-------|-------------|
| `forward` | - | Move forward |
| `backward` | - | Move backward |
| `strafe_left` | - | Strafe left |
| `strafe_right` | - | Strafe right |
| `turn_left` | - | Turn left |
| `turn_right` | - | Turn right |
| `aim` | `value`: 0-360° | Aim at angle |
| `attack` | - | Attack |
| `use` | - | Use/interact |
| `weapon` | `value`: 1-7 | Switch weapon |

Weapon slots: `1` Fist/Chainsaw, `2` Pistol, `3` Shotgun/SuperShotgun,
`4` Chaingun, `5` RocketLauncher, `6` PlasmaRifle, `7` BFG9000.

### Examples

```json
// Move forward
{"name": "player_input", "arguments": {"action": "forward"}}

// Aim at 90 degrees and attack
{"name": "player_input", "arguments": {"action": "aim", "value": 90}}
{"name": "player_input", "arguments": {"action": "attack"}}

// Switch to shotgun (weapon 3)
{"name": "player_input", "arguments": {"action": "weapon", "value": 3}}
```

---

## Screenshots

### ASCII Screenshot

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_screenshot"}}'
```

Returns ASCII representation of the game view for visual debugging.

---

## Batch Operations

### Execute Multiple Commands

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "execute_batch",
    "arguments": {
      "calls": [
        {"name": "give_item", "arguments": {"item_class": "Chaingun", "amount": 1}},
        {"name": "spawn_entity", "arguments": {"entity_class": "Zombieman", "x": 500, "y": 500, "angle": 0}}
      ]
    }
  }
}
```

**Note:** `change_level` is rejected in batch mode. Queue level changes separately and wait for completion.

### Command Completion

Poll async command status:

```bash
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/call",
    "params": {
      "name": "get_command_result",
      "arguments": {"sequence": 1}
    }
  }'
```

Response:
```json
{
  "sequence": 1,
  "completed": true,
  "status": "success",
  "message": "Command executed"
}
```

---

## Error Handling

All errors follow JSON-RPC 2.0 format:

```json
{
  "jsonrpc": "2.0",
  "id": "1",
  "error": {
    "code": -32602,
    "message": "Invalid params"
  }
}
```

### Error Codes

| Code | Meaning |
|------|---------|
| -32700 | Parse error (invalid JSON) |
| -32600 | Invalid Request |
| -32601 | Method not found |
| -32602 | Invalid params |
| -32603 | Internal error |
| -32002 | Server not initialized |
| -32000 | Server error |

### C/C++ Error Pattern

```c
mcp_status_t result = dmcp_screenshot_submit(ctx, &frame);
if (result.code != MCP_STATUS_CODE_OK) {
    fprintf(stderr, "Error: %s\n", result.message);
}
```

---

## See Also

- [API Reference](README.md) - Full API documentation
- [Integration Guide](INTEGRATION.md) - Connect to MCP clients
- [Architecture](ARCHITECTURE.md) - System design
