# Migration Guide

## Migrating to v1.0 (Context-Based API)

The DMCP library has undergone a major refactor to support multiple instances, thread safety, and modern C++ standards.

### 1. Global State Removed
The global `dmcp_init()` and `dmcp_process_tick()` functions have been replaced with a context-based API.

**Old:**
```c
dmcp_config_t cfg = ...;
dmcp_init(&cfg);
dmcp_process_tick();
```

**New:**
```c
dmcp_config_t cfg = dmcp_default_config();
dmcp_context_t* ctx = dmcp_create(&cfg);
dmcp_update(ctx);
dmcp_destroy(ctx);
```

### 2. Return Codes
Functions that returned `int` (magic numbers) now return `dmcp_result_t` enum.

**Old:**
```c
int res = dmcp_submit_screenshot(...);
if (res == -1) { ... }
```

**New:**
```c
dmcp_result_t res = dmcp_submit_screenshot(...);
if (res != DMCP_OK) { ... }
```

### 3. Stats
`dmcp_dropped_snapshot_count()` has been replaced with a comprehensive stats struct.

**New:**
```c
dmcp_stats_t stats;
dmcp_get_stats(ctx, &stats);
printf("Dropped: %lu", stats.dropped_snapshots);
```

### 4. Adapter Changes
The `Binder` class has been removed in favor of simpler function composition. Adapters now reside in `adapters/<engine>/adapter.cpp` and are exposed as CMake `INTERFACE` libraries.

**To Update Your Adapter:**
1.  Remove `#include "dmcp/binder.hpp"`.
2.  Define a `PopulateSnapshot(dmcp::Snapshot& s)` function.
3.  Call your binding logic directly inside this function.
4.  Pass `&PopulateSnapshot` (wrapped in a callback) to `dmcp_config_t.on_tick`.

### 5. Build System
You must now use `vcpkg` to build DMCP.

```bash
cmake --preset default
cmake --build --preset default
```
