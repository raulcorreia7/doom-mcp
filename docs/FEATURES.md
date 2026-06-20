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
| Inventory management | Give items, set health, move player |
| Entity manipulation | Damage or kill specific entities |
| Game control | Pause/unpause |
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
| HTTP | MCP JSON-RPC endpoint plus health/game routes |
| JSON-RPC 2.0 | Full protocol compliance with error handling |
| MCP 2025-11-25 | Model Context Protocol specification compliant |
| Per-session state | Multiple clients with independent lifecycles |

## Performance Features

| Feature | Benefit |
|---------|---------|
| Writer-priority snapshots | Game thread publishes latest state without waiting on readers |
| Snapshot sampling | Configurable latest-snapshot rate (`target_hz`) |
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

- Public C99-compatible API for maximum portability
- C++ source convenience wrappers available
- Thread-safe internal state
- Rich error messages

### Engine Adapters

DMCP adapter source lives under `adapters/`. Adapter-specific requirements,
build flags, and engine hook examples are documented in each adapter directory.

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
| `get_entities` | Enemies and world entities | `kind`, `status`, `offset`, `limit` |
| `get_items` | World pickups/items only | `kind`, `offset`, `limit` |
| `get_map` | Map/level details | None |
| `get_inventory` | Inventory items | `offset`, `limit` |
| `get_game_info` | Game metadata | None |
| `get_available_content` | Complete available-only canonical content catalog | optional `game_mode` |
| `get_available_enemies` | Available enemy classes | optional `game_mode` |
| `get_available_entities` | Available spawnable entity classes | optional `game_mode` |
| `get_available_items` | Available item classes | optional `game_mode` |
| `get_available_weapons` | Available weapon classes | optional `game_mode` |
| `get_available_ammo` | Available ammo classes | optional `game_mode` |
| `get_available_keys` | Available key classes | optional `game_mode` |
| `get_available_maps` | Available map names | optional `game_mode` |
| `get_available_giveable` | Available classes accepted by `give_item` | optional `game_mode` |
| `get_state` | Unified section query | `section`, `kind`, `status` |
| `get_state_batch` | Multi-section query | `requests[]` |
| `get_screenshot` | ASCII screenshot when enabled by the embedder | None |

### Action Tools

| Tool | Description | Arguments |
|------|-------------|-----------|
| `spawn_entity` | Spawn available entity | `entity_class`, `x`, `y`, `angle` |
| `give_item` | Give weapon, ammo, key, or item | `item_class`, `amount` |
| `change_level` | Change map | `map_name`, `skill_level`, `reset_inventory` |
| `set_player_position` | Move player | `x`, `y`, `angle` |
| `execute_batch` | Queue multiple commands | `calls[]` |
| `player_input` | Player control | `action`, `value` |
| `get_command_result` | Poll command status | `sequence` |
| `get_command_examples` | Structured examples | None |

## Build Options

| CMake Option | Default | Description |
|--------------|---------|-------------|
| `DMCP_BUILD_EXAMPLES` | OFF | Build example servers |
| `DMCP_BUILD_TESTS` | OFF | Build unit test suite |
| `DMCP_BUILD_INTEGRATION_TESTS` | OFF | Build C/C++ SDK integration tests |
| `DMCP_BUILD_ADAPTER_FAKE` | OFF | Build fake adapter for smoke/integration |
| `DMCP_BUILD_SHARED` | OFF | Build shared libraries |
| `DMCP_BUILD_SINGLE_DLL` | ON | Build single `libdmcp` runtime surface |
| `DMCP_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer |

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
