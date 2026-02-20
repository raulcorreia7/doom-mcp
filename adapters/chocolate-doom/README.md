# Chocolate Doom Adapter for DMCP

This adapter integrates Chocolate Doom with the DMCP SDK, enabling MCP protocol support for AI agents.

## Structure

```
adapters/chocolate-doom/
├── dmcp_adapter.h      # Public API
├── dmcp_adapter.c      # Lifecycle (create/destroy/tick)
├── dmcp_ascii.h        # ASCII screenshot API
├── dmcp_ascii.c        # ASCII capture from framebuffer
├── dmcp_mappings.h     # Type conversion utilities
├── enemy_types.h       # Enemy type lookup table
├── state_player.c      # Player state extraction
├── state_level.c       # Level/game state extraction
├── state_enemies.c     # Enemy + interactive entity enumeration
├── commands.c          # Command execution
├── CMakeLists.txt
└── README.md
```

## Design

The adapter follows a modular structure with single-responsibility files:

| File | Purpose |
|------|---------|
| `dmcp_adapter.c` | Lifecycle management (~120 lines) |
| `state_player.c` | Extract player state from `player_t` |
| `state_level.c` | Extract level/game state from globals |
| `state_enemies.c` | Enumerate enemies and interactive world entities |
| `dmcp_ascii.c` | Capture framebuffer as ASCII art |
| `dmcp_mappings.h` | `fixed_t`→float, enum→string conversions |
| `enemy_types.h` | Data-driven enemy type lookup |

Shared adapter utilities are available from `dmcp/adapter/utils.h` for common
coordinate/angle conversions and validation.

## Requirements

- Chocolate Doom source (submodule at `../../chocolate-doom/`)
- DMCP SDK (built at `../../build/`)
- SDL2 libraries

## Building

```bash
# From doom-mcp root
cmake -B build -DDMCP_BUILD_TESTS=ON
cmake --build build

# Build Chocolate Doom with DMCP
cmake -S chocolate-doom -B chocolate-doom/build \
  -DDMCP_ENABLE=ON \
  -DDMCP_INCLUDE_DIR="$PWD/include" \
  -DDMCP_LIB_DIR="$PWD/build"
cmake --build chocolate-doom/build
```

## Integration Points

The adapter patches Chocolate Doom at:

1. **D_DoomMain()** - Initialize DMCP via `dmcp_chocolate_create()`
2. **G_Ticker()** - Tick DMCP every game tic via `dmcp_chocolate_tick()`
3. **I_Quit()** - Shutdown DMCP via `dmcp_chocolate_destroy()`

## API

```c
#include "adapters/chocolate-doom/dmcp_adapter.h"

// Configuration
dmcp_chocolate_config_t cfg = dmcp_chocolate_config_default();
cfg.base.port = 6060;
cfg.iwad_path = "doom1.wad";

// Lifecycle
dmcp_chocolate_t* ctx = dmcp_chocolate_create(&cfg);

// In G_Ticker (35 Hz)
dmcp_chocolate_tick(ctx);
dmcp_chocolate_commands_process(ctx);

// Queries
bool running = dmcp_chocolate_is_running(ctx);
dmcp_stats_t stats;
dmcp_chocolate_get_stats(ctx, &stats);

// Cleanup (in I_Quit)
dmcp_chocolate_destroy(ctx);
```

## Human-Friendly Output

All enum values are output as readable strings:

- **Difficulty**: `"Hurt Me Plenty"`, `"Ultra-Violence"`, etc.
- **Weapons**: `"Pistol"`, `"Shotgun"`, `"Rocket Launcher"`, etc.
- **Player State**: `"alive"`, `"dead"`, `"reborn"`
- **Game State**: `"in_level"`, `"intermission"`, `"finale"`
- **Enemy Types**: `"Zombieman"`, `"Imp"`, `"Cacodemon"`, etc.

## Headless Mode

```bash
# Run headless for testing
./chocolate-doom/build/src/chocolate-doom \
  -iwad assets/wads/doom1.wad \
  -nodraw -nosound \
  -dmcp_port 6060
```

`-dmcp_port <n>` (or `-dmcp-port <n>`) overrides the DMCP HTTP/SSE port.

## E2E Testing

```bash
# Download shareware WAD
make download-wad

# Run headless integration tests
make headless

# Run Python e2e tests
pytest -q tests/e2e
```

## License

- Adapter code: MIT
- Chocolate Doom: GPL v2
