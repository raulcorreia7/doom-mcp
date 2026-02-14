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
| `DMCP_BUILD_ADAPTER_ZDOOM` | OFF | Build ZDoom adapter |
| `DMCP_BUILD_ADAPTER_CHOCOLATE` | OFF | Build Chocolate Doom adapter |
| `DMCP_BUILD_INTEGRATION_TESTS` | OFF | Build integration tests |
| `DMCP_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer |

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

## Error Handling

All functions return `mcp_result_generic_t`:

```c
mcp_result_generic_t result = dmcp_screenshot_submit(ctx, &frame);
if (result.code != MCP_RESULT_CODE_OK) {
    fprintf(stderr, "Error: %s\n", result.message);
}
```

## License

MIT License - See [LICENSE](LICENSE) for details.
