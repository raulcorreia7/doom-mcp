# DMCP Adapters

Adapters bridge DMCP SDK with specific Doom engine implementations.

## Structure

```
adapters/
├── common/              # Shared utilities for all adapters
│   ├── include/dmcp_hooks.h
│   └── src/dmcp_hooks.c
├── crispy-doom/         # Crispy Doom adapter
├── zdoom/               # ZDoom adapter
└── fake/                # Test/fake adapter
```

## Adding a New Engine

### 1. Create adapter directory

```bash
mkdir -p adapters/my-engine/{include,src,patches}
```

### 2. Implement integration hooks

Create `adapters/my-engine/src/dmcp_integration.c`:

```c
#include "dmcp_integration.h"
#include "dmcp_hooks.h"
#include <engine_headers.h>

dmcp_myengine_t* g_dmcp_ctx = NULL;

void DMCP_Init(void) {
    dmcp_myengine_config_t cfg = dmcp_myengine_config_default();
    int port = dmcp_engine_port_from_argv(argc, argv, "dmcp_port");
    
    if (port > 0) cfg.base.port = port;
    
    g_dmcp_ctx = dmcp_myengine_create(&cfg);
}

void DMCP_Shutdown(void) {
    if (g_dmcp_ctx) {
        dmcp_myengine_destroy(g_dmcp_ctx);
        g_dmcp_ctx = NULL;
    }
}

void DMCP_Tick(void) {
    if (g_dmcp_ctx) {
        dmcp_myengine_tick(g_dmcp_ctx);
        dmcp_myengine_commands_process(g_dmcp_ctx);
    }
}
```

### 3. Create engine patch

Create minimal patch to add 3 hooks:

```diff
// In engine's main initialization (e.g., d_main.c)
+#ifdef DMCP
+#include "dmcp_integration.h"
+#endif

void D_DoomMain(void) {
    // ... engine init ...
    
+#ifdef DMCP
+    DMCP_Init();
+#endif
    
    D_DoomLoop();
}

// In engine's main loop tick (e.g., g_game.c)
void G_Ticker(void) {
    // ... engine tick ...
    
+#ifdef DMCP
+    DMCP_Tick();
+#endif
}
```

### 4. Add CMakeLists.txt

```cmake
file(GLOB SOURCES src/*.c)

add_library(myengine ${DMCP_LIBRARY_TYPE} ${SOURCES})

target_include_directories(myengine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${ENGINE_SRC_DIR}
        ${CMAKE_SOURCE_DIR}/adapters/common/include
)

target_link_libraries(myengine
    PUBLIC dmcp::core
    PRIVATE dmcp_adapter_common
)
```

### 5. Add to main CMakeLists.txt

```cmake
option(DMCP_BUILD_ADAPTER_MYENGINE "Build MyEngine adapter" OFF)

if(DMCP_BUILD_ADAPTER_MYENGINE)
    add_subdirectory(adapters/my-engine)
endif()
```

### 6. Add Makefile target

```makefile
.PHONY: my-engine
my-engine: submodules
    @./tests/integration/build_my_engine.sh
```

## Common Utilities

### dmcp_hooks.h

- `dmcp_engine_config_t` - Standard engine configuration
- `dmcp_engine_port_from_argv()` - Parse `-dmcp_port` from command line

## Build Commands

```bash
# Build specific engine
make engine ENGINE=crispy

# Test specific engine
make engine-test ENGINE=crispy

# Build all engines
make engine-all
```
