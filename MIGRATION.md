# Migration Guide for v0.5.0

This guide helps you migrate your code from the old API to the new, consistent type-oriented naming convention introduced in v0.5.0.

## Table of Contents

- [Overview](#overview)
- [Type-Oriented Naming Convention](#type-oriented-naming-convention)
- [Quick Reference Table](#quick-reference-table)
- [Breaking Changes by Category](#breaking-changes-by-category)
  - [1. Generic MCP API (Task 4)](#1-generic-mcp-api-task-4)
  - [2. Doom MCP API (Task 5)](#2-doom-mcp-api-task-5)
  - [3. Adapter API (Task 6)](#3-adapter-api-task-6)
  - [4. Result Types (Task 3)](#4-result-types-task-3)
  - [5. Configuration (Task 6)](#5-configuration-task-6)
- [Complete Code Examples](#complete-code-examples)
- [Troubleshooting](#troubleshooting)
- [Version Bump Recommendation](#version-bump-recommendation)

## Overview

### What Changed

The v0.5.0 release introduces a comprehensive refactor to make the API more consistent, intuitive, and production-ready. All public APIs now follow a **Type-Oriented** naming pattern that groups functions by the type they operate on.

### Why the Breaking Changes?

1. **Consistency**: All APIs now follow the same naming convention
2. **Better Discoverability**: Functions are alphabetically grouped by type
3. **Rich Error Messages**: Error results now include descriptive messages
4. **Semantic Correctness**: Functions return more accurate types (e.g., client count instead of boolean)

### Migration Strategy

1. Review the [Quick Reference Table](#quick-reference-table) for all API changes
2. Read the detailed sections relevant to your use case
3. Update your code using the [Complete Code Examples](#complete-code-examples)
4. Refer to [Troubleshooting](#troubleshooting) for common issues

## Type-Oriented Naming Convention

All APIs now follow a **Type-Oriented** naming pattern:

```
{namespace}_{type}_{action}
```

- **namespace**: Library prefix (`mcp_`, `dmcp_`)
- **type**: The primary type being operated on (`server`, `context`, `config`, `method`, `event`, `stats`)
- **action**: The operation being performed (`create`, `destroy`, `register`, `broadcast`, `get`)

### Examples

```c
// Server type operations
mcp_server_create()              // Create a server
mcp_server_destroy()             // Destroy a server
mcp_server_is_running()          // Check if server is running

// Server methods (sub-resource of server)
mcp_server_method_register()     // Register a method on server
mcp_server_method_unregister()   // Unregister a method
mcp_server_methods_register()    // Batch register methods

// Server events (sub-resource of server)
mcp_server_event_broadcast()     // Broadcast event from server

// Server stats (sub-resource of server)
mcp_server_stats_get()          // Get server statistics
mcp_server_clients_count()       // Get connected client count

// Context type operations
dmcp_context_create()           // Create a context
dmcp_context_destroy()          // Destroy a context
dmcp_context_tick()            // Tick the context
dmcp_context_is_running()      // Check if context is running

// Screenshots (sub-resource of context)
dmcp_screenshot_is_requested() // Check if screenshot requested
dmcp_screenshot_submit()       // Submit screenshot data

// Stats (sub-resource of context)
dmcp_stats_get()               // Get context statistics

// Configuration
dmcp_config_default()          // Get default config
dmcp_zdoom_config_default()   // Get ZDoom default config
```

### Benefits

1. **Groups by Type**: All context functions alphabetically together
2. **Matches OOP**: Like `Context::create()` in C++
3. **Scalable**: Easy to add new actions to existing types
4. **Readable**: "Context create", "Server method register"
5. **Consistent**: One pattern everywhere

## Quick Reference Table

| Old API | New API | Change Type | Reason |
|----------|----------|-------------|--------|
| `dmcp_create()` | `dmcp_context_create()` | Renamed | Add type infix |
| `dmcp_destroy()` | `dmcp_context_destroy()` | Renamed | Add type infix |
| `dmcp_tick()` | `dmcp_context_tick()` | Renamed | Add type infix |
| `dmcp_is_running()` | `dmcp_context_is_running()` | Renamed | Add type infix |
| `dmcp_screenshot_requested()` | `dmcp_screenshot_is_requested()` | Renamed | Add "screenshot" infix |
| `dmcp_submit_screenshot()` | `dmcp_screenshot_submit()` | Renamed | Add "screenshot" infix |
| `dmcp_get_stats()` | `dmcp_stats_get()` | Renamed | Add "stats" infix |
| `dmcp_default_config()` | `dmcp_config_default()` | Renamed | Add type infix |
| `dmcp_zdoom_default_config()` | `dmcp_zdoom_config_default()` | Renamed | Add type infix |
| `dmcp_zdoom_execute_command()` | `dmcp_zdoom_command_execute()` | Renamed | Add "command" infix |
| `dmcp_zdoom_process_commands()` | `dmcp_zdoom_commands_process()` | Renamed | Add "commands" infix |
| `mcp_server_register_method()` | `mcp_server_method_register()` | Renamed | Add type infix |
| `mcp_server_unregister_method()` | `mcp_server_method_unregister()` | Renamed | Add type infix |
| `mcp_server_broadcast()` | `mcp_server_event_broadcast()` | Renamed | Add type infix |
| `mcp_server_has_clients()` | `mcp_server_clients_count()` | Renamed + Type Change | Add type infix + return actual count |
| `mcp_server_get_stats()` | `mcp_server_stats_get()` | Renamed | Add type infix |
| `mcp_result_t` (enum) | `mcp_result_t` (struct) | Type Changed | Add message field |
| `dmcp_result_t` (enum) | `dmcp_result_t` (struct) | Type Changed | Add message field |

## Breaking Changes by Category

### 1. Generic MCP API (Task 4)

All Generic MCP server functions have been renamed to follow the type-oriented naming convention.

#### Function Renames

| Old API | New API | Migration Notes |
|---------|---------|-----------------|
| `mcp_server_register_method(server, method, handler, user_data)` | `mcp_server_method_register(server, method, handler, user_data)` | Direct rename, same parameters |
| `mcp_server_unregister_method(server, method)` | `mcp_server_method_unregister(server, method)` | Direct rename, same parameters |
| `mcp_server_broadcast(server, event_type, json_payload)` | `mcp_server_event_broadcast(server, event_type, json_payload)` | Direct rename, same parameters |
| `mcp_server_has_clients(server)` | `mcp_server_clients_count(server)` | **Type Change**: Returns `uint64_t` instead of `bool` |
| `mcp_server_get_stats(server, stats)` | `mcp_server_stats_get(server, stats)` | Direct rename, same parameters |

#### Important Change: `mcp_server_has_clients()` → `mcp_server_clients_count()`

**Old API (Boolean)**:
```c
if (mcp_server_has_clients(server)) {
    // At least one client connected
}
```

**New API (Count)**:
```c
uint64_t count = mcp_server_clients_count(server);
if (count > 0) {
    // At least one client connected
}
```

The new function returns the **actual number of connected clients** instead of a boolean. This provides more information and is semantically more correct.

### 2. Doom MCP API (Task 5)

All Doom MCP functions have been renamed to follow the type-oriented naming convention with "context" as the primary type.

#### Function Renames

| Old API | New API | Migration Notes |
|---------|---------|-----------------|
| `dmcp_create(config)` | `dmcp_context_create(config)` | Add "context" infix |
| `dmcp_destroy(ctx)` | `dmcp_context_destroy(ctx)` | Add "context" infix |
| `dmcp_tick(ctx)` | `dmcp_context_tick(ctx)` | Add "context" infix |
| `dmcp_is_running(ctx)` | `dmcp_context_is_running(ctx)` | Add "context" infix |
| `dmcp_screenshot_requested(ctx)` | `dmcp_screenshot_is_requested(ctx)` | Add "screenshot" infix |
| `dmcp_submit_screenshot(ctx, frame)` | `dmcp_screenshot_submit(ctx, frame)` | Add "screenshot" infix |
| `dmcp_get_stats(ctx, stats)` | `dmcp_stats_get(ctx, stats)` | Add "stats" infix |

#### Migration Example

**Before**:
```c
#include "dmcp/doom/api.h"

dmcp_config_t config = dmcp_default_config();
dmcp_context_t* ctx = dmcp_create(&config);

while (running) {
    dmcp_tick(ctx);

    if (dmcp_screenshot_requested(ctx)) {
        dmcp_screenshot_frame_t frame = { /* ... */ };
        dmcp_submit_screenshot(ctx, &frame);
    }
}

dmcp_destroy(ctx);
```

**After**:
```c
#include "dmcp/doom/api.h"

dmcp_config_t config = dmcp_config_default();
dmcp_context_t* ctx = dmcp_context_create(&config);

while (running) {
    dmcp_context_tick(ctx);

    if (dmcp_screenshot_is_requested(ctx)) {
        dmcp_screenshot_frame_t frame = { /* ... */ };
        dmcp_screenshot_submit(ctx, &frame);
    }
}

dmcp_context_destroy(ctx);
```

### 3. Adapter API (Task 6)

ZDoom adapter functions have been renamed for consistency with the type-oriented naming convention.

#### Function Renames

| Old API | New API | Migration Notes |
|---------|---------|-----------------|
| `dmcp_zdoom_default_config()` | `dmcp_zdoom_config_default()` | Add "config" infix |
| `dmcp_zdoom_execute_command(ctx, cmd)` | `dmcp_zdoom_command_execute(ctx, cmd)` | Add "command" infix |
| `dmcp_zdoom_process_commands(ctx)` | `dmcp_zdoom_commands_process(ctx)` | Add "commands" infix |

#### Migration Example

**Before**:
```cpp
#include "adapters/zdoom/adapter.h"

dmcp_zdoom_config_t cfg = dmcp_zdoom_default_config();
dmcp_zdoom_t* mcp = dmcp_zdoom_create(&cfg);

dmcp_command_t cmd = { /* ... */ };
dmcp_zdoom_execute_command(mcp, &cmd);

dmcp_zdoom_process_commands(mcp);

dmcp_zdoom_destroy(mcp);
```

**After**:
```cpp
#include "adapters/zdoom/adapter.h"

dmcp_zdoom_config_t cfg = dmcp_zdoom_config_default();
dmcp_zdoom_t* mcp = dmcp_zdoom_create(&cfg);

dmcp_command_t cmd = { /* ... */ };
dmcp_zdoom_command_execute(mcp, &cmd);

dmcp_zdoom_commands_process(mcp);

dmcp_zdoom_destroy(mcp);
```

### 4. Result Types (Task 3)

The most significant change is the transformation of result types from enums to structs with code and message fields.

#### Type Change: Enum → Struct

**Old Type (Enum)**:
```c
typedef enum {
    MCP_OK = 0,
    MCP_ERROR_INVALID_ARGS = -1,
    MCP_ERROR_ENCODING_FAILED = -2,
    // ... more error codes
} mcp_result_t;
```

**New Type (Struct)**:
```c
typedef struct {
    int32_t code;        // 0 = success, negative = error
    const char* message;  // Human-readable error message
} mcp_result_t;
```

#### Convenience Macros

New macros simplify result creation:
```c
#define MCP_OK ((mcp_result_t){0, NULL})
#define MCP_ERROR_INVALID_ARGS ((mcp_result_t){-1, "Invalid arguments"})
#define MCP_ERROR_ENCODING_FAILED ((mcp_result_t){-2, "Encoding failed"})
// ... more error macros
```

#### Error Handling Migration

**Before (Enum Comparison)**:
```c
mcp_result_t result = mcp_server_register_method(server, "tool", handler, NULL);
if (result != MCP_OK) {
    fprintf(stderr, "Error: %d\n", result);
}
```

**After (Struct Comparison)**:
```c
mcp_result_t result = mcp_server_method_register(server, "tool", handler, NULL);
if (result.code != MCP_RESULT_CODE_OK) {
    fprintf(stderr, "Error: %s: %s\n", result.code, result.message);
}
```

#### Key Points

1. **Access `.code` field**: Compare using `result.code` instead of `result`
2. **Access `.message` field**: Use `result.message` for human-readable error descriptions
3. **NULL message on success**: When `result.code == 0`, `result.message` is `NULL`
4. **Use result constants**: Compare against `MCP_RESULT_CODE_OK` (0) or `DMCP_RESULT_CODE_OK` (0)

#### Result Code Constants

**Generic MCP Result Codes**:
- `MCP_RESULT_CODE_OK` (0) - Success
- `MCP_RESULT_CODE_INVALID_ARGS` (-1) - Invalid arguments
- `MCP_RESULT_CODE_ENCODING_FAILED` (-2) - JSON encoding/decoding failed
- `MCP_RESULT_CODE_DISABLED` (-3) - Operation disabled
- `MCP_RESULT_CODE_QUEUE_FULL` (-4) - Queue is full
- `MCP_RESULT_CODE_NOT_FOUND` (-5) - Resource/method not found
- `MCP_RESULT_CODE_INTERNAL` (-6) - Internal error

**Doom MCP Result Codes**:
- `DMCP_RESULT_CODE_OK` (0) - Success
- `DMCP_RESULT_CODE_INVALID_ARGS` (-1) - Invalid arguments
- `DMCP_RESULT_CODE_ENCODING_FAILED` (-2) - JSON encoding/decoding failed
- `DMCP_RESULT_CODE_DISABLED` (-3) - Operation disabled
- `DMCP_RESULT_CODE_QUEUE_FULL` (-4) - Queue is full
- `DMCP_RESULT_CODE_NOT_FOUND` (-5) - Resource/method not found
- `DMCP_RESULT_CODE_INTERNAL` (-6) - Internal error
- `DMCP_RESULT_CODE_SERVER_FAILED` (-7) - Server operation failed

### 5. Configuration (Task 6)

Configuration functions have been renamed to follow the type-oriented naming convention.

#### Function Renames

| Old API | New API | Migration Notes |
|---------|---------|-----------------|
| `dmcp_default_config()` | `dmcp_config_default()` | Add "config" infix |
| `dmcp_zdoom_default_config()` | `dmcp_zdoom_config_default()` | Add "config" infix |

#### Configuration Structure Change

The `dmcp_zdoom_config_t` structure now **embeds** `dmcp_config_t` directly instead of using a pointer.

**Old Structure (Pointer)**:
```c
typedef struct {
    uint32_t struct_size;
    const dmcp_config_t* dmcp_config;  // Pointer indirection
    void (*log_fn)(void* user, int level, const char* message);
    bool (*should_tick_fn)(void* user);
} dmcp_zdoom_config_t;
```

**New Structure (Embedding)**:
```c
typedef struct {
    uint32_t struct_size;
    dmcp_config_t base;  // Embedded, not pointer!
    void (*log_fn)(void* user, int level, const char* message);
    bool (*should_tick_fn)(void* user);
} dmcp_zdoom_config_t;
```

#### Configuration Migration

**Before (Pointer Indirection)**:
```c
dmcp_zdoom_config_t cfg = dmcp_zdoom_default_config();
cfg.dmcp_config->port = 8080;  // Pointer indirection
```

**After (Direct Access)**:
```c
dmcp_zdoom_config_t cfg = dmcp_zdoom_config_default();
cfg.base.port = 8080;  // Direct access
```

## Complete Code Examples

### Example 1: Generic MCP Server

**Before**:
```c
#include "mcp/generic/server.h"

mcp_server_config_t config = mcp_default_config();
mcp_server_t* server = mcp_server_create(&config);

// Register method handler
bool OnGetState(void* user, const char* method, const char* params,
                char* response, size_t response_size) {
  strcpy(response, "{\"result\":{\"hp\":100}}");
  return true;
}

mcp_result_t result = mcp_server_register_method(server, "tools/get_state", OnGetState, NULL);
if (result != MCP_OK) {
    fprintf(stderr, "Registration failed: %d\n", result);
}

// Broadcast events
mcp_server_broadcast(server, "state", "{\"hp\":100}");

// Check for clients
if (mcp_server_has_clients(server)) {
    printf("Clients connected\n");
}

// Get statistics
mcp_server_stats_t stats;
mcp_server_get_stats(server, &stats);

mcp_server_destroy(server);
```

**After**:
```c
#include "mcp/generic/server.h"

mcp_server_config_t config = mcp_default_config();
mcp_server_t* server = mcp_server_create(&config);

// Register method handler
bool OnGetState(void* user, const char* method, const char* params,
                char* response, size_t response_size) {
  strcpy(response, "{\"result\":{\"hp\":100}}");
  return true;
}

mcp_result_t result = mcp_server_method_register(server, "tools/get_state", OnGetState, NULL);
if (result.code != MCP_RESULT_CODE_OK) {
    fprintf(stderr, "Registration failed: %s: %s\n", result.code, result.message);
}

// Broadcast events
mcp_server_event_broadcast(server, "state", "{\"hp\":100}");

// Check for clients (now returns actual count)
uint64_t count = mcp_server_clients_count(server);
if (count > 0) {
    printf("Clients connected: %llu\n", (unsigned long long)count);
}

// Get statistics
mcp_server_stats_t stats;
mcp_server_stats_get(server, &stats);

mcp_server_destroy(server);
```

### Example 2: Doom MCP Integration

**Before**:
```c
#include "dmcp/doom/api.h"

void SnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
  snapshot->player.hp = player->health;
  snapshot->player.position.x = player->x;
  snapshot->player.position.y = player->y;
}

dmcp_config_t config = dmcp_default_config();
config.port = 6060;
config.on_snapshot = SnapshotCallback;

dmcp_context_t* ctx = dmcp_create(&config);
if (!ctx) {
  return -1;
}

while (running) {
  dmcp_tick(ctx);

  if (dmcp_screenshot_requested(ctx)) {
    uint8_t* pixels = CaptureScreenshot();
    dmcp_screenshot_frame_t frame = {
      .pixels = pixels,
      .width = 640,
      .height = 480,
      .stride = 640 * 4
    };
    dmcp_result_t result = dmcp_submit_screenshot(ctx, &frame);
    if (result != DMCP_OK) {
      fprintf(stderr, "Screenshot failed: %d\n", result);
    }
    free(pixels);
  }
}

dmcp_stats_t stats;
dmcp_get_stats(ctx, &stats);
printf("Dropped snapshots: %llu\n", stats.dropped_snapshots);

dmcp_destroy(ctx);
```

**After**:
```c
#include "dmcp/doom/api.h"

void SnapshotCallback(void* user_data, dmcp_snapshot_t* snapshot) {
  snapshot->player.hp = player->health;
  snapshot->player.position.x = player->x;
  snapshot->player.position.y = player->y;
}

dmcp_config_t config = dmcp_config_default();
config.port = 6060;
config.on_snapshot = SnapshotCallback;

dmcp_context_t* ctx = dmcp_context_create(&config);
if (!ctx) {
  return -1;
}

while (running) {
  dmcp_context_tick(ctx);

  if (dmcp_screenshot_is_requested(ctx)) {
    uint8_t* pixels = CaptureScreenshot();
    dmcp_screenshot_frame_t frame = {
      .pixels = pixels,
      .width = 640,
      .height = 480,
      .stride = 640 * 4
    };
    dmcp_result_t result = dmcp_screenshot_submit(ctx, &frame);
    if (result.code != DMCP_RESULT_CODE_OK) {
      fprintf(stderr, "Screenshot failed: %s: %s\n", result.code, result.message);
    }
    free(pixels);
  }
}

dmcp_stats_t stats;
dmcp_stats_get(ctx, &stats);
printf("Dropped snapshots: %llu\n", stats.dropped_snapshots);

dmcp_context_destroy(ctx);
```

### Example 3: ZDoom Adapter

**Before**:
```cpp
#include "adapters/zdoom/adapter.h"

dmcp_zdoom_config_t cfg = dmcp_zdoom_default_config();
cfg.dmcp_config->port = 8080;  // Pointer access

dmcp_zdoom_t* mcp = dmcp_zdoom_create(&cfg);

dmcp_command_t cmd = {
  .type = DMCP_COMMAND_SPAWN,
  .flags = DMCP_COMMAND_IMMEDIATE,
  .data.spawn = { .classname = "DoomImp" }
};
dmcp_zdoom_execute_command(mcp, &cmd);

dmcp_zdoom_process_commands(mcp);

dmcp_zdoom_destroy(mcp);
```

**After**:
```cpp
#include "adapters/zdoom/adapter.h"

dmcp_zdoom_config_t cfg = dmcp_zdoom_config_default();
cfg.base.port = 8080;  // Direct access via .base

dmcp_zdoom_t* mcp = dmcp_zdoom_create(&cfg);

dmcp_command_t cmd = {
  .type = DMCP_COMMAND_SPAWN,
  .flags = DMCP_COMMAND_IMMEDIATE,
  .data.spawn = { .classname = "DoomImp" }
};
dmcp_zdoom_command_execute(mcp, &cmd);

dmcp_zdoom_commands_process(mcp);

dmcp_zdoom_destroy(mcp);
```

### Example 4: Batch Method Registration

**Before**:
```c
#include "mcp/generic/server.h"

mcp_server_t* server = mcp_server_create(&config);

// Register methods one by one
mcp_server_register_method(server, "tools/list", OnListTools, NULL);
mcp_server_register_method(server, "tools/call", OnCallTool, NULL);
mcp_server_register_method(server, "resources/list", OnListResources, NULL);
mcp_server_register_method(server, "resources/read", OnReadResource, NULL);
mcp_server_register_method(server, "resources/write", OnWriteResource, NULL);
```

**After**:
```c
#include "mcp/generic/server.h"

mcp_server_t* server = mcp_server_create(&config);

// Batch register methods (atomic operation)
mcp_method_registration_t methods[] = {
  {"tools/list", OnListTools, NULL},
  {"tools/call", OnCallTool, NULL},
  {"resources/list", OnListResources, NULL},
  {"resources/read", OnReadResource, NULL},
  {"resources/write", OnWriteResource, NULL}
};

mcp_result_t result = mcp_server_methods_register(server, methods, 5);
if (result.code != MCP_RESULT_CODE_OK) {
  fprintf(stderr, "Batch registration failed: %s: %s\n", result.code, result.message);
}
```

## Troubleshooting

### Common Migration Issues

#### Q: I'm getting linker errors for undefined symbols like `mcp_server_register_method`

**A**: Update your code to use the new function names. The old names no longer exist.

```c
// OLD: mcp_server_register_method()
// NEW: mcp_server_method_register()
```

#### Q: My error handling code doesn't compile

**A**: Change result checks from comparing the result directly to comparing `result.code`:

```c
// Before
if (result != MCP_OK) { }

// After
if (result.code != MCP_RESULT_CODE_OK) {
    printf("Error: %s\n", result.message);
}
```

#### Q: The compiler says "invalid use of incomplete type" or "request for member 'code' in something not a structure or union"

**A**: Ensure you're including the correct headers and using the complete `mcp_result_t` struct:

```c
#include "mcp/generic/server.h"
#include "mcp/generic/protocol.h"  // For result type definition

mcp_result_t result = mcp_server_method_register(...);
if (result.code != MCP_RESULT_CODE_OK) { ... }
```

#### Q: How do I handle `result.message` being NULL for success cases?

**A**: Check `result.code == 0` before accessing `result.message`. NULL is expected for `MCP_OK`:

```c
mcp_result_t result = some_function(...);
if (result.code != 0) {
    fprintf(stderr, "Error: %s\n", result.message);
} else {
    // Success - result.message is NULL
    printf("Operation succeeded\n");
}
```

#### Q: Batch registration doesn't compile

**A**: Ensure you're including `include/mcp/generic/server.h` for the registration struct:

```c
#include "mcp/generic/server.h"

mcp_method_registration_t methods[] = {
  {"tool1", Handler1, NULL},
  {"tool2", Handler2, NULL}
};

mcp_result_t result = mcp_server_methods_register(server, methods, 2);
```

#### Q: I can't access ZDoom config fields like `cfg.dmcp_config->port`

**A**: The config structure now embeds `dmcp_config_t` directly. Use `.base` instead:

```c
// Before
cfg.dmcp_config->port = 8080;

// After
cfg.base.port = 8080;
```

#### Q: `mcp_server_has_clients()` doesn't exist

**A**: This function has been renamed to `mcp_server_clients_count()` and now returns `uint64_t`:

```c
// Before
if (mcp_server_has_clients(server)) { }

// After
uint64_t count = mcp_server_clients_count(server);
if (count > 0) { }
```

### Compilation Errors

#### Error: 'mcp_server_register_method' was not declared

**Solution**: Rename to `mcp_server_method_register()`

#### Error: 'mcp_server_broadcast' was not declared

**Solution**: Rename to `mcp_server_event_broadcast()`

#### Error: 'dmcp_create' was not declared

**Solution**: Rename to `dmcp_context_create()`

#### Error: 'dmcp_default_config' was not declared

**Solution**: Rename to `dmcp_config_default()`

### Runtime Issues

#### Issue: Error codes are strange numbers like -1, -2 instead of MCP_OK, MCP_ERROR

**Explanation**: The result type is now a struct with `.code` field. Use `result.code` for comparisons and `result.message` for descriptions.

**Solution**: Update all error handling to use `.code` field.

#### Issue: Screenshot submission fails with no error message

**Explanation**: When `result.code == 0` (success), `result.message` is NULL. This is expected behavior.

**Solution**: Check `result.code != 0` before accessing `result.message`.

## Version Bump Recommendation

### Recommended Version: **v0.5.0**

**Rationale**:
- Major API refactoring with breaking changes
- Maintains pre-1.0 status, signaling potential for additional breaking changes
- All deprecated/old APIs removed without backward compatibility shims

**Alternative: v1.0.0**

Use v1.0.0 if you consider the API stable and production-ready:
- This indicates the API is final and will not have breaking changes
- Signal to users that the API has matured to stable release
- Requires confidence that no further breaking changes will be needed

### Release Notes Template

```markdown
## v0.5.0 - API Refactor (2024-XX-XX)

### Breaking Changes

- **Type-Oriented Naming Convention**: All public APIs now follow `{namespace}_{type}_{action}` pattern
- **Result Type Redesign**: Changed from enum to struct with code and message fields
- **Configuration Changes**: ZDoom config now embeds base config instead of using pointer
- **Function Renames**: See [Migration Guide](MIGRATION.md) for complete list

### New Features

- **Batch Method Registration**: `mcp_server_methods_register()` for atomic multi-method registration
- **Rich Error Messages**: All errors now include descriptive messages
- **Client Count**: `mcp_server_clients_count()` returns actual connected count instead of boolean

### Migration

See [MIGRATION.md](MIGRATION.md) for detailed migration guide with before/after examples.

### Upgrade Path

1. Read the [Quick Reference Table](MIGRATION.md#quick-reference-table)
2. Update function calls to use new names
3. Update error handling to use `result.code` and `result.message`
4. Update ZDoom config access to use `.base` instead of pointer
5. Rebuild and test
```

## Summary

The v0.5.0 release is a major API refactoring that:

1. ✅ Introduces consistent **Type-Oriented Naming Convention**
2. ✅ Adds **rich error messages** to all result types
3. ✅ Improves **semantic correctness** (e.g., client count vs boolean)
4. ✅ Provides **atomic batch operations** for efficiency
5. ✅ Removes all **legacy/deprecated APIs** for cleaner codebase

### Migration Checklist

- [ ] Review Quick Reference Table for all API changes
- [ ] Update all function calls to use new names
- [ ] Update error handling to use `result.code` and `result.message`
- [ ] Update ZDoom config access to use `.base` field
- [ ] Update `mcp_server_has_clients()` to `mcp_server_clients_count()`
- [ ] Update `dmcp_default_config()` to `dmcp_config_default()`
- [ ] Update `dmcp_zdoom_default_config()` to `dmcp_zdoom_config_default()`
- [ ] Rebuild project
- [ ] Run tests
- [ ] Test with real Doom engine integration

### Need Help?

- Review the [Complete Code Examples](#complete-code-examples) section
- Check the [Troubleshooting](#troubleshooting) section for common issues
- Consult the main [README.md](README.md) for updated API documentation
- Review header files (`include/mcp/generic/server.h`, `include/dmcp/doom/api.h`) for function signatures
