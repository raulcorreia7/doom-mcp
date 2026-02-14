# Chocolate Doom Adapter for DMCP

This adapter integrates Chocolate Doom with the DMCP SDK, enabling MCP protocol support.

## Structure

```
adapters/chocolate-doom/
├── adapter.h          - Adapter API
├── adapter.c          - Implementation (bridges DMCP <-> Chocolate Doom)
├── CMakeLists.txt     - Build configuration
└── README.md          - This file
```

## Requirements

- Chocolate Doom source (submodule at `../../chocolate-doom/`)
- DMCP SDK (built at `../../build/`)
- SDL2 libraries

## Building

```bash
# From doom-mcp root
cmake -B build -DDMCP_BUILD_ADAPTER_CHOCOLATE=ON
cmake --build build
```

## Integration

The adapter patches Chocolate Doom at these points:

1. **D_DoomMain()** - Initialize DMCP
2. **G_Ticker()** - Tick DMCP every game tic
3. **I_Quit()** - Shutdown DMCP

## Usage

```c
#include "adapters/chocolate-doom/adapter.h"

// In D_DoomMain, after I_PrintStartupBanner:
DMCP_Chocolate_Init(6060);

// In G_Ticker, near end:
DMCP_Chocolate_Tick();

// In I_Quit, before SDL_Quit:
DMCP_Chocolate_Shutdown();
```

## Headless Mode

Chocolate Doom supports headless operation:
```bash
chocolate-doom -iwad doom1.wad -nodraw -nosound
```

## License

Adapter code: MIT
Chocolate Doom: GPL v2
