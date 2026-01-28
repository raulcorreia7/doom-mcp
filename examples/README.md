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
cmake --build build
./build/dummy_server
```

The server will:
- Listen on port 6060
- Accept MCP protocol connections at `/mcp`
- Stream server-sent events at `/sse`
- Provide health checks at `/health`
- Broadcast snapshots at 10 Hz (configurable via `target_hz`)

## Migration Notes (v0.5.0 Breaking Changes)

This example has been updated to use the new type-oriented API naming convention. If you're upgrading from an older version, here are the key changes:

### Function Renames

| Old Function | New Function | Example Usage |
|--------------|---------------|---------------|
| `dmcp_default_config()` | `dmcp_config_default()` | `dmcp_config_t cfg = dmcp_config_default();` |
| `dmcp_create()` | `dmcp_context_create()` | `dmcp_context_t* ctx = dmcp_context_create(&cfg);` |
| `dmcp_destroy()` | `dmcp_context_destroy()` | `dmcp_context_destroy(ctx);` |
| `dmcp_tick()` | `dmcp_context_tick()` | `dmcp_context_tick(ctx);` |
| `dmcp_is_running()` | `dmcp_context_is_running()` | `if (dmcp_context_is_running(ctx)) { ... }` |
| `dmcp_get_stats()` | `dmcp_stats_get()` | `dmcp_stats_get(ctx, &stats);` |
| `dmcp_screenshot_requested()` | `dmcp_screenshot_is_requested()` | `if (dmcp_screenshot_is_requested(ctx)) { ... }` |
| `dmcp_submit_screenshot()` | `dmcp_screenshot_submit()` | `dmcp_result_t result = dmcp_screenshot_submit(ctx, &frame);` |

### Error Handling Changes

The `dmcp_result_t` type changed from an enum to a struct:

**Before:**
```c
if (dmcp_screenshot_submit(ctx, &frame) != DMCP_OK) {
    fprintf(stderr, "Screenshot failed\n");
}
```

**After:**
```c
dmcp_result_t result = dmcp_screenshot_submit(ctx, &frame);
if (result.code != DMCP_RESULT_CODE_OK) {
    fprintf(stderr, "Screenshot failed: %s\n", result.message);
}
```

### Key Takeaways

1. **Type-oriented naming**: All functions now follow `{namespace}_{type}_{action}` pattern
2. **Rich error messages**: `result.message` provides human-readable error descriptions
3. **Check `.code` field**: Always compare `result.code` not the entire result struct
4. **Consistent patterns**: Once you learn one function (e.g., `dmcp_context_*`), the rest follow the same pattern

For a complete migration guide, see the main README.md file.

## Example Code Patterns

### Basic Setup
```c
#include "dmcp/doom/api.h"

void SnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
    // Fill snapshot with game state
    snapshot->player.hp = player->health;
    snapshot->player.position.x = player->x;
    snapshot->player.position.y = player->y;
}

dmcp_config_t config = dmcp_config_default();
config.port = 6060;
config.target_hz = 10;
config.on_snapshot = SnapshotCallback;

dmcp_context_t* ctx = dmcp_context_create(&config);
if (!ctx) {
    fprintf(stderr, "Failed to create DMCP context\n");
    return -1;
}
```

### Game Loop Integration
```c
while (game_running) {
    GameTick();  // Your game logic

    // Update DMCP (process commands, broadcast snapshots)
    dmcp_context_tick(ctx);

    // Maintain target frame rate
    Sleep(frametime);
}
```

### Screenshot Handling
```c
// In your render loop
if (dmcp_screenshot_is_requested(ctx)) {
    uint8_t* pixels = CaptureScreenshot();
    dmcp_screenshot_frame_t frame = {
        .pixels = pixels,
        .width = width,
        .height = height,
        .stride = width * 4  // RGBA = 4 bytes per pixel
    };

    dmcp_result_t result = dmcp_screenshot_submit(ctx, &frame);
    if (result.code != DMCP_RESULT_CODE_OK) {
        fprintf(stderr, "Screenshot failed: %s\n", result.message);
    }

    free(pixels);  // Safe to free after submit (data is copied)
}
```

### Statistics Monitoring
```c
dmcp_stats_t stats;
dmcp_stats_get(ctx, &stats);

printf("Connected clients: %llu\n", stats.connected_clients);
printf("Dropped snapshots: %llu\n", stats.dropped_snapshots);
printf("Dropped screenshots: %llu\n", stats.dropped_screenshots);
```

## Troubleshooting

### Server fails to start
- Check if port is already in use: `netstat -an | grep 6060`
- Try a different port: `config.port = 6061;`
- Check firewall settings

### No clients can connect
- Verify server is running: `curl http://localhost:6060/health`
- Check firewall rules allow TCP connections on the port
- Ensure client uses correct endpoint: `http://<server-ip>:<port>/mcp`

### Snapshots not updating
- Verify `on_snapshot` callback is being called
- Check `target_hz` is not too low (default 10 Hz)
- Verify MCP client is listening on `/sse` endpoint

### Screenshots failing
- Ensure screenshot dimensions match config: `config.screenshot.width/height`
- Verify pixel data is valid RGBA format (4 bytes per pixel)
- Check `result.message` for specific error details

## Additional Resources

- [Main README](../README.md) - Full API documentation and architecture
- [Migration Guide](../README.md#migration-guide-v050-breaking-changes) - Detailed migration instructions
- [API Reference](../README.md#api-reference) - Complete API documentation
