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

- **[README.md](docs/README.md)** - Full API documentation and examples
- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** - Architecture overview
- **[CHANGELOG.md](docs/CHANGELOG.md)** - Version history

## Quick Start

```bash
# Configure and build
cmake -B build -S .
cmake --build build

# Run example server
./build/dummy_server

# Test (optional)
cmake -B build -DDMCP_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

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

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `DMCP_BUILD_EXAMPLES` | ON | Build example servers |
| `DMCP_BUILD_TESTS` | OFF | Build test suite |
| `DMCP_BUILD_ADAPTER_ZDOOM` | OFF | Build ZDoom adapter |
| `DMCP_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer |
| `DMCP_USE_CCACHE` | ON | Use ccache if available |

## Error Handling

All functions return `mcp_result_generic_t` with error code and message:

```c
mcp_result_generic_t result = dmcp_screenshot_submit(ctx, &frame);
if (result.code != MCP_RESULT_CODE_OK) {
    fprintf(stderr, "Error: %s\n", result.message);
}
```

## License

MIT License - See [LICENSE](LICENSE) for details.
