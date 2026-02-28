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

## Public API

Every adapter implements the same minimal interface:

```c
#include "dmcp_hooks.h"

typedef struct {
    uint16_t port;             // 0 = default (6060)
    uint16_t target_hz;        // 0 = default (35)
    bool     screenshot_enabled;
} dmcp_engine_config_t;

// Config helpers (optional - engines can build config manually)
dmcp_engine_config_t dmcp_engine_config_default(void);
dmcp_engine_config_t dmcp_engine_config_from_env(void);
dmcp_engine_config_t dmcp_engine_config_from_argv(int argc, char** argv, const char* flag);

// Lifecycle (required)
void DMCP_Init(const dmcp_engine_config_t* config);
void DMCP_Shutdown(void);
void DMCP_Tick(void);
```

## Adding a New Engine

### 1. Create adapter directory

```bash
mkdir -p adapters/my-engine/{include,src,patches}
```

### 2. Implement integration

Create `adapters/my-engine/include/dmcp_integration.h`:

```c
#ifndef DMCP_INTEGRATION_H
#define DMCP_INTEGRATION_H

#include "dmcp_adapter.h"
#include "dmcp_hooks.h"

#ifdef __cplusplus
extern "C" {
#endif

void DMCP_Init(const dmcp_engine_config_t* config);
void DMCP_Shutdown(void);
void DMCP_Tick(void);

#ifdef __cplusplus
}
#endif

#endif
```

Create `adapters/my-engine/src/dmcp_integration.c`:

```c
#include "dmcp_integration.h"

dmcp_myengine_t* g_dmcp_ctx = NULL;

void DMCP_Init(const dmcp_engine_config_t* config) {
    dmcp_engine_config_t cfg = config ? *config : dmcp_engine_config_default();

    dmcp_myengine_config_t adapter_cfg = dmcp_myengine_config_default();
    adapter_cfg.base.port = cfg.port;
    adapter_cfg.base.target_hz = cfg.target_hz;

    g_dmcp_ctx = dmcp_myengine_create(&adapter_cfg);
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

Minimal patch to add 3 hooks:

```diff
// In engine's main initialization (e.g., d_main.c)
+#ifdef DMCP
+#include "dmcp_integration.h"
+#endif

void D_DoomMain(void) {
    // ... engine init ...

+#ifdef DMCP
+    dmcp_engine_config_t dmcp_cfg = dmcp_engine_config_from_argv(
+        myargc, myargv, "-dmcp_port");
+    DMCP_Init(&dmcp_cfg);
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
)
```

## Build Commands

```bash
# Build specific engine
make engine ENGINE=crispy

# Test specific engine
make engine-test ENGINE=crispy

# Build all engines
make engine-all
```

## Config Sources

Engines can choose how to build config:

```c
// From command line
dmcp_engine_config_t cfg = dmcp_engine_config_from_argv(argc, argv, "-dmcp_port");

// From environment
dmcp_engine_config_t cfg = dmcp_engine_config_from_env();

// Manual
dmcp_engine_config_t cfg = dmcp_engine_config_default();
cfg.port = 8080;

// NULL for defaults
DMCP_Init(NULL);
```
