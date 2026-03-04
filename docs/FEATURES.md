# DMCP Features

Complete feature overview of the Doom Model Context Protocol SDK.

## Core Capabilities

### Real-Time State Access

| Feature | Description |
|---------|-------------|
| Player state | Health, armor, position, angle, ammo, current weapon |
| Enemy tracking | Position, HP, type, alive/dead status, TID |
| Entity enumeration | Pickups, barrels, interactive objects (excludes projectiles/decor) |
| Level info | Map name, kill/item/secret counts, skill level |
| Inventory | Full inventory list with quantities |
| Game metadata | Game mode, version, engine info |

### Command Execution

| Feature | Description |
|---------|-------------|
| Entity spawning | Spawn any enemy or item at specified position |
| Level changes | Switch maps with skill level control |
| Inventory management | Give items, set health, teleport player |
| Entity manipulation | Damage or kill specific entities |
| Game control | Pause/unpause, timescale adjustment |
| Console commands | Execute engine-specific console commands |

### Player Control

| Feature | Description |
|---------|-------------|
| Movement | Forward, backward, strafe left/right |
| Aiming | Turn left/right, aim at specific angle |
| Actions | Attack, use/interact |
| Weapons | Switch between weapons 1-7 |
| Tick-accurate | 35Hz input rate matching game ticks |

### Transport & Protocol

| Feature | Description |
|---------|-------------|
| HTTP/SSE | POST for JSON-RPC, GET for real-time SSE stream |
| JSON-RPC 2.0 | Full protocol compliance with error handling |
| MCP 2025-11-25 | Model Context Protocol specification compliant |
| Per-session state | Multiple clients with independent lifecycles |

## Performance Features

| Feature | Benefit |
|---------|---------|
| Object pooling | Reuses snapshot objects to minimize allocations |
| Rate limiting | Configurable snapshot rate (target_hz) |
| Batch operations | Multiple commands/queries in single request |
| Async commands | Non-blocking command queue with completion polling |

## Integration Features

### C/C++ API

```c
// Simple lifecycle
dmcp_context_t* ctx = dmcp_context_create(&config);
dmcp_context_tick(ctx);  // Call each game tick
dmcp_context_destroy(ctx);
```

- Clean C API for maximum portability
- C++ convenience wrappers available
- Thread-safe internal state
- Rich error messages

### Engine Adapters

| Adapter | Engine | Status |
|---------|--------|--------|
| Crispy Doom | Enhanced vanilla port | Stable |
| ZDoom | GZDoom/ZDoom family | Beta |

### MCP Client Support

| Client | Platform | Config |
|--------|----------|--------|
| Claude Desktop | macOS, Windows | `claude_desktop_config.json` |
| Claude Code CLI | Cross-platform | `~/.claude-code/config.json` |
| Cline | VS Code | `.vscode/mcp.json` |
| Continue | VS Code | `.continue/config.json` |
| Opencode | CLI | `.opencode/mcp.json` |

## HTTP Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/mcp` | POST | JSON-RPC 2.0 protocol endpoint |
| `/mcp` | GET | Server-Sent Events stream |
| `/health` | GET | Health check (returns JSON) |
| `/game/state` | GET | Current game snapshot as JSON |
| `/game/screenshot` | GET | Latest screenshot payload as JSON |

## MCP Tools

### State Query Tools

| Tool | Description | Arguments |
|------|-------------|-----------|
| `get_player` | Player-only state | None |
| `get_enemies` | Enemy list | `status`, `offset`, `limit` |
| `get_entities` | Interactive entities | `offset`, `limit` |
| `get_map` / `get_level` | Map/level details | None |
| `get_inventory` | Inventory items | `offset`, `limit` |
| `get_game_info` / `get_game` | Game metadata | None |
| `get_state` | Unified section query | `section`, `status` |
| `get_state_batch` | Multi-section query | `requests[]` |
| `get_screenshot` | ASCII screenshot | None |

### Action Tools

| Tool | Description | Arguments |
|------|-------------|-----------|
| `execute_command` | Queue a command | `type`, command params |
| `execute_batch` | Queue multiple commands | `commands[]` |
| `input` | Player control | `a` (action), `v` (value) |
| `get_command_result` | Poll command status | `sequence` |
| `get_command_examples` | Structured examples | None |
| `get_available_content` | Entity/item classes | None |

## Build Options

| CMake Option | Default | Description |
|--------------|---------|-------------|
| `DMCP_BUILD_EXAMPLES` | ON | Build example servers |
| `DMCP_BUILD_TESTS` | OFF | Build unit test suite |
| `DMCP_BUILD_INTEGRATION_TESTS` | OFF | Build integration tests |
| `DMCP_BUILD_SHARED` | OFF | Build shared libraries |
| `DMCP_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer |
| `DMCP_BUILD_ADAPTER_ZDOOM` | OFF | Build ZDoom adapter |
| `DMCP_BUILD_ADAPTER_CRISPY` | OFF | Build Crispy adapter |

## Security & Safety

| Feature | Description |
|---------|-------------|
| Input validation | All inputs validated at API boundaries |
| No secrets in logs | Credentials never logged |
| Payload limits | Maximum request size enforced |
| Graceful degradation | Invalid commands return errors, don't crash |

## Platform Support

| Platform | Compiler | Status |
|----------|----------|--------|
| Linux | GCC 9+, Clang 10+ | Primary |
| macOS | Clang (Xcode) | Supported |
| Windows | MSVC 2019+ | Experimental |

## See Also

- [Quick Start](QUICKSTART.md) - Get running in 5 minutes
- [Usage Guide](USAGE.md) - Common patterns
- [API Reference](README.md) - Full API documentation
- [Architecture](ARCHITECTURE.md) - System design
