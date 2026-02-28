# ZDoom Adapter for DMCP

This adapter integrates ZDoom/GZDoom-based source ports with the DMCP SDK, enabling MCP protocol support for AI agents.

## Structure

```
adapters/zdoom/
├── adapter.h         # Public API
├── internal.h        # Shared internal utilities (AdapterContext, logging)
├── adapter.cpp       # Lifecycle + state extraction
├── commands.cpp      # Command execution
├── CMakeLists.txt
└── README.md
```

## Design

The ZDoom adapter uses a streamlined C++ implementation:

| File | Purpose |
|------|---------|
| `internal.h` | Shared utilities (AdapterContext, logging helpers) |
| `adapter.cpp` | Lifecycle + state extraction (~300 lines) |
| `commands.cpp` | Command execution (~200 lines) |

## Requirements

- ZDoom or GZDoom source code
- DMCP SDK (built with `DMCP_BUILD_ADAPTER_ZDOOM=ON`)
- CMake 3.25+

## Building

```bash
# From doom-mcp root
cmake -B build -DDMCP_BUILD_TESTS=ON -DDMCP_BUILD_ADAPTER_ZDOOM=ON
cmake --build build
```

In your ZDoom-based engine's CMakeLists.txt:

```cmake
find_package(dmcp CONFIG REQUIRED)
target_link_libraries(myengine PRIVATE dmcp::zdoom)
```

## Integration Points

Integrate the adapter into your ZDoom engine at:

1. **Startup** - Initialize DMCP via `dmcp_zdoom_create()`
2. **G_Ticker()** - Tick DMCP every game tic via `dmcp_zdoom_tick()`
3. **Shutdown** - Cleanup DMCP via `dmcp_zdoom_destroy()`

## API

```cpp
#include "adapters/zdoom/adapter.h"

// Configuration
dmcp_zdoom_config_t cfg = dmcp_zdoom_config_default();
cfg.base.port = 6060;

// Lifecycle
dmcp_zdoom_t* ctx = dmcp_zdoom_create(&cfg);

// In G_Ticker (35 Hz)
mcp_result_generic_t result = dmcp_zdoom_tick(ctx);
if (result.code != MCP_RESULT_CODE_OK) {
    // Handle error
}

// Process pending commands
dmcp_zdoom_commands_process(ctx);

// Queries
bool running = dmcp_zdoom_is_running(ctx);
dmcp_stats_t stats;
dmcp_zdoom_get_stats(ctx, &stats);

// Cleanup
dmcp_zdoom_destroy(ctx);
```

## Human-Friendly Output

All enum values are output as readable strings:

- **Difficulty**: `"Hurt Me Plenty"`, `"Ultra-Violence"`, etc.
- **Weapons**: `"Pistol"`, `"Shotgun"`, `"Rocket Launcher"`, etc.
- **Player State**: `"alive"`, `"dead"`, `"reborn"`
- **Game State**: `"in_level"`, `"intermission"`, `"finale"`
- **Enemy Types**: `"Zombieman"`, `"Imp"`, `"Cacodemon"`, etc.

## License

MIT License - See LICENSE file for details.
