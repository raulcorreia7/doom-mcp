# Doom MCP Examples

This directory contains example implementations showing how to integrate the Doom MCP SDK with your game engine.

## Examples

### dummy_server.cpp

A minimal example that demonstrates:
- Creating a DMCP context with custom configuration
- Implementing snapshot callbacks
- Handling logging callbacks
- Game loop integration with `dmcp_context_tick()`
- Statistics monitoring with `dmcp_stats_get()`

**Running the example:**
```bash
# Using Makefile
make run

# Or using CMake
cmake -B build -DDMCP_BUILD_EXAMPLES=ON
cmake --build build
./build/dummy_server
```

The server will:
- Listen on port 6060
- Accept MCP protocol connections at `/mcp`
- Stream server-sent events at `/mcp`
- Provide health checks at `/health`
- Serve game snapshot at `/game/state`
- Serve screenshot JSON at `/game/screenshot`
- Broadcast snapshots at 10 Hz (configurable via `target_hz`)

## Code Patterns

### Basic Setup
```c
#include "dmcp/doom/dmcp.h"  // Convenience header (includes all DMCP headers)

void SnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
    snapshot->player.hp = player->health;
    snapshot->player.position.x = player->x;
    snapshot->player.position.y = player->y;
}

dmcp_config_t config = dmcp_config_default();
config.port = 6060;
config.target_hz = 10;
config.on_snapshot = SnapshotCallback;

dmcp_context_t* ctx = dmcp_context_create(&config);
```

### Game Loop Integration
```c
while (game_running) {
    GameTick();
    dmcp_context_tick(ctx);
}
```

### Screenshot Handling
```c
if (dmcp_screenshot_is_requested(ctx)) {
    dmcp_screenshot_frame_t frame = {
        .pixels = pixels,
        .width = width,
        .height = height,
        .stride = width * 4
    };
    mcp_result_generic_t result = dmcp_screenshot_submit(ctx, &frame);
    if (result.code != MCP_RESULT_CODE_OK) {
        fprintf(stderr, "Screenshot failed: %s\n", result.message);
    }
}
```

### Statistics
```c
dmcp_stats_t stats;
dmcp_stats_get(ctx, &stats);
printf("Clients: %llu, Dropped: %llu\n", 
       stats.connected_clients, stats.dropped_snapshots);
```

### Error Handling
```c
mcp_result_generic_t result = dmcp_push_command(ctx, &cmd);
if (result.code != MCP_RESULT_CODE_OK) {
    fprintf(stderr, "Error: %s\n", result.message);
}
```

## Troubleshooting

- **Server fails to start**: Check if port 6060 is already in use
- **No clients can connect**: Verify server is running with `curl http://localhost:6060/health`
- **Snapshots not updating**: Verify `on_snapshot` callback is being called

## Resources

- [Main README](../docs/README.md) - Full API documentation
- [ARCHITECTURE.md](../docs/ARCHITECTURE.md) - Architecture overview
- [CHANGELOG.md](../docs/CHANGELOG.md) - Version history
