# Work Plan: Doom-MCP Breaking Changes Refactor

## Executive Summary

This work plan implements a **breaking changes refactor** to transform doom-mcp into a clean, intuitive, production-ready SDK. We will remove legacy APIs, standardize naming, and redesign interfaces for maximum clarity and ease of use.

**Current Grade**: B+ (Solid foundation, API inconsistencies)  
**Target Grade**: A+ (Clean, intuitive, production-ready)

**Estimated Effort**: Medium-Large (5-7 days)  
**Breaking Changes**: YES - All deprecated/old APIs removed  
**Migration Guide**: Required - Will be provided

---

## Type-Oriented Naming Convention

All APIs follow a **Type-Oriented** naming pattern:

```
{namespace}_{type}_{action}
```

**Pattern Components:**
- `namespace`: Library prefix (`mcp_`, `dmcp_`)
- `type`: The primary type being operated on (`server`, `context`, `config`, `method`, `event`)
- `action`: The operation being performed (`create`, `destroy`, `register`, `broadcast`, `get`)

**Examples:**
```c
// Server type operations
mcp_server_create()
mcp_server_destroy()
mcp_server_method_register()
mcp_server_event_broadcast()

// Context type operations
dmcp_context_create()
dmcp_context_destroy()
dmcp_context_tick()
dmcp_screenshot_submit()

// Config type operations
dmcp_config_default()
dmcp_config_validate()
dmcp_zdoom_config_default()

// Stats type operations
dmcp_stats_get()
mcp_server_stats_get()
```

**Compound Types:**
When operating on sub-resources, extend the type name:
```c
// Server methods (sub-resource of server)
mcp_server_method_register()   // Register a method on server
mcp_server_method_unregister() // Unregister a method
mcp_server_methods_register()  // Batch register multiple methods

// Server events (sub-resource of server)
mcp_server_event_broadcast()   // Broadcast event from server

// Server stats (sub-resource of server)
mcp_server_stats_get()         // Get server statistics

// Screenshots (sub-resource of context)
dmcp_screenshot_is_requested() // Check if screenshot requested
dmcp_screenshot_submit()       // Submit screenshot data

// Stats (sub-resource of context)
dmcp_stats_get()               // Get context statistics
```

**Why Type-Oriented?**
1. **Groups by type**: All context functions alphabetically together
2. **Matches OOP**: Like `Context::create()` in C++
3. **Scalable**: Easy to add new actions to existing types
4. **Readable**: "Context create", "Server method register"
5. **Consistent**: One pattern everywhere

---

## Breaking Changes Philosophy

**Why Breaking Changes Are Better Here:**

1. **Clean Slate**: Remove inconsistent naming (`dmcp_create` vs `mcp_server_create`)
2. **Simpler Mental Model**: Unified patterns across all layers
3. **Better Defaults**: Fix configuration inconsistencies
4. **Future-Proof**: Design for extensibility from day one
5. **Easier Documentation**: One way to do things, not three

**Migration Strategy:**
- Provide clear migration guide
- Document every breaking change
- Offer before/after code examples
- Semantic versioning bump (0.5.0 or 1.0.0)

---

## Breaking Changes Summary

### 1. API Naming (BREAKING)

**BEFORE:**
```c
// Inconsistent - sometimes has type, sometimes doesn't
mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
dmcp_context_t* dmcp_create(const dmcp_config_t* config);  // Missing "context"
dmcp_tick(ctx);  // Missing "context"
```

**AFTER:**
```c
// Consistent - always {namespace}_{type}_{action}
mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
dmcp_context_t* dmcp_context_create(const dmcp_config_t* config);
dmcp_context_tick(dmcp_context_t* ctx);
```

### 2. Error Handling (BREAKING)

**BEFORE:**
```c
mcp_result_t result = mcp_server_register_method(server, "tool", handler, NULL);
if (result != MCP_OK) {
    // No idea what went wrong
    fprintf(stderr, "Error: %d\n", result);
}
```

**AFTER:**
```c
mcp_result_t result = mcp_server_register_method(server, "tool", handler, NULL);
if (result.code != MCP_OK) {
    // Clear error message
    fprintf(stderr, "Error: %s\n", result.message);
}
```

### 3. Configuration (BREAKING)

**BEFORE:**
```c
// Inconsistent - flat vs nested
dmcp_config_t dmcp_default_config();  // Flat
dmcp_zdoom_config_t cfg;              // Nested pointer to dmcp_config
```

**AFTER:**
```c
// Consistent - all flat, explicit extension pattern
dmcp_config_t dmcp_config_default(void);
dmcp_zdoom_config_t dmcp_zdoom_config_default(void);
// dmcp_zdoom_config_t embeds dmcp_config_t directly, not pointer
```

### 4. Result Types (BREAKING)

**BEFORE:**
```c
typedef enum {
    MCP_OK = 0,
    MCP_ERROR_INVALID_ARGS = -1,
    // ...
} mcp_result_t;
```

**AFTER:**
```c
typedef struct {
    int code;
    const char* message;
} mcp_result_t;

#define MCP_OK ((mcp_result_t){0, NULL})
#define MCP_ERROR_INVALID_ARGS ((mcp_result_t){-1, "Invalid arguments"})
```

### 5. Callback Signatures (BREAKING)

**BEFORE:**
```c
// Duplicate log callback types
typedef void (*mcp_log_callback_t)(void* user_data, int level, const char* message);
typedef void (*dmcp_log_callback_t)(void* user_data, int level, const char* message);
```

**AFTER:**
```c
// Single type, reused everywhere
typedef void (*mcp_log_callback_t)(void* user_data, mcp_log_level_t level, 
                                    const char* message);
```

---

## Work Objectives

### Core Objective
Redesign all public APIs to be consistent, intuitive, and production-ready. Remove all legacy/deprecated code. Provide clear migration path.

### Concrete Deliverables
1. **Consistent API naming** - `{namespace}_{type}_{action}` everywhere
2. **Rich error handling** - Result structs with codes and messages
3. **Unified configuration** - Flat configs, consistent patterns
4. **Simplified callbacks** - Reuse common types
5. **Constants header** - Zero magic numbers
6. **Batch operations** - Efficient bulk registration
7. **Transport registry** - Runtime transport selection
8. **Complete TODOs** - Screenshot endpoint, PNG encoding
9. **Unit test suite** - Comprehensive coverage
10. **Migration guide** - Document all breaking changes

### Definition of Done
- [ ] All public APIs follow consistent naming convention
- [ ] Error results include descriptive messages
- [ ] Configuration patterns are unified
- [ ] Batch registration works efficiently
- [ ] Runtime transport selection works
- [ ] Zero magic numbers in code
- [ ] Unit tests achieve >70% coverage
- [ ] All TODO items resolved
- [ ] All public APIs documented
- [ ] Migration guide complete
- [ ] Examples updated to new API
- [ ] Version bumped to 0.5.0 or 1.0.0

### Must Have
- Clean, consistent API design
- Comprehensive error messages
- Complete documentation
- Working test suite
- Migration guide

### Must NOT Have (Guardrails)
- No deprecated/legacy APIs
- No backward compatibility shims
- No inconsistent naming
- No magic numbers
- No duplicate code

---

## Execution Strategy

### Phase 1: Foundation (Days 1-2)

**Wave 1 - Critical Infrastructure:**
- [x] 1. Setup test infrastructure
- [x] 2. Create constants header
- [x] 3. Redesign result types (rich errors)

**Wave 3 - Advanced Features:**
7. Implement batch operations
8. Implement transport registry
9. Complete TODO items

**Wave 4 - Quality:**
10. Write comprehensive tests
11. Add documentation
12. Update examples

**Wave 5 - Finalization:**
13. Write migration guide
14. Final integration testing
15. Version bump and release notes

---

## TODOs (Breaking Changes Refactor)

### Phase 1: Foundation

- [x] 1. Setup Test Infrastructure

  **What to do**:
  - Add Catch2 testing framework via CPM
  - Create tests/ directory structure
  - Write first test to verify setup
  - Add test target to CMake

  **Breaking Changes**: None

  **Acceptance Criteria**:
  - [ ] `cmake --build build` compiles tests
  - [ ] `ctest` runs and shows test results
  - [ ] At least one example test passes

  **Commit**: `test: Add Catch2 testing framework`

- [ ] 2. Create Constants Header

  **What to do**:
  - Create `include/mcp/generic/constants.h`
  - Define ALL magic numbers:
    - Buffer sizes: `MCP_BUFFER_SIZE_DEFAULT` (8192)
    - Max sizes: `MCP_MAX_ENEMIES` (256), `MCP_MAX_INVENTORY` (64)
    - Default port: `MCP_DEFAULT_PORT` (6060)
    - Protocol strings: `MCP_ENDPOINT_MCP` ("/mcp")
    - JSON-RPC: `MCP_JSONRPC_VERSION` ("2.0")
  - Update all source files to use constants

  **Breaking Changes**: None (internal only)

  **Acceptance Criteria**:
  - [ ] Constants header created
  - [ ] All source files updated
  - [ ] `grep -r "8192\|16384\|6060" src/ include/` returns nothing

  **Commit**: `refactor: Extract magic numbers to constants.h`

- [ ] 3. Redesign Result Types (BREAKING)

  **What to do**:
  - Change `mcp_result_t` from enum to struct:
    ```c
    typedef struct {
        int code;
        const char* message;
    } mcp_result_t;
    ```
  - Define macros for common results:
    ```c
    #define MCP_OK ((mcp_result_t){0, NULL})
    #define MCP_ERROR_INVALID ((mcp_result_t){-1, "Invalid arguments"})
    ```
  - Same for `dmcp_result_t`
  - Update all functions to return new result type
  - Update all error sites to set messages

  **Breaking Changes**:
  - `mcp_result_t` is now struct, not enum
  - Comparison changes: `result == MCP_OK` → `result.code == 0`
  - Functions return struct instead of int

  **Migration**:
  ```c
  // Before
  if (mcp_server_register_method(...) != MCP_OK) { }
  
  // After
  mcp_result_t result = mcp_server_register_method(...);
  if (result.code != 0) {
      printf("Error: %s\n", result.message);
  }
  ```

  **Acceptance Criteria**:
  - [ ] Result type changed to struct
  - [ ] All functions updated
  - [ ] All error messages meaningful
  - [ ] Tests verify error handling

  **Commit**: `feat(api)!: Redesign result types with error messages`
  
  **BREAKING CHANGE**: `mcp_result_t` changed from enum to struct

### Phase 2: Core API Redesign

- [ ] 4. Redesign Generic MCP API (BREAKING)

  **What to do**:
  
  **BEFORE:**
  ```c
  // server.h
  mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
  void mcp_server_destroy(mcp_server_t* server);
  bool mcp_server_is_running(const mcp_server_t* server);
  mcp_result_t mcp_server_register_method(mcp_server_t* server, ...);
  void mcp_server_broadcast(mcp_server_t* server, ...);
  ```
  
  **AFTER:**
  ```c
  // server.h - Consistent naming
  mcp_server_t* mcp_server_create(const mcp_server_config_t* config);
  void mcp_server_destroy(mcp_server_t* server);
  bool mcp_server_is_running(const mcp_server_t* server);
  
  // Method registration - consistent action name
  mcp_result_t mcp_server_method_register(mcp_server_t* server, ...);
  void mcp_server_method_unregister(mcp_server_t* server, const char* method);
  
  // Broadcasting - consistent action name  
  void mcp_server_event_broadcast(mcp_server_t* server, ...);
  bool mcp_server_has_clients(const mcp_server_t* server);
  
  // Statistics - new feature
  void mcp_server_stats_get(const mcp_server_t* server, mcp_server_stats_t* stats);
  
  // Batch operations - new feature
  typedef struct {
      const char* method;
      mcp_method_handler_t handler;
      void* user_data;
  } mcp_method_registration_t;
  
  mcp_result_t mcp_server_methods_register(mcp_server_t* server,
                                           const mcp_method_registration_t* methods,
                                           size_t count);
  ```

  **Breaking Changes**:
  - `mcp_server_register_method` → `mcp_server_method_register`
  - `mcp_server_broadcast` → `mcp_server_event_broadcast`

  **Migration**:
  ```c
  // Before
  mcp_server_register_method(server, "tool", handler, data);
  mcp_server_broadcast(server, "event", json);
  
  // After
  mcp_server_method_register(server, "tool", handler, data);
  mcp_server_event_broadcast(server, "event", json);
  ```

  **Acceptance Criteria**:
  - [ ] All functions renamed consistently
  - [ ] Batch registration implemented
  - [ ] Old function names removed (no compat)
  - [ ] Tests updated

  **Commit**: `feat(api)!: Redesign Generic MCP API naming`
  
  **BREAKING CHANGE**: Function names changed for consistency

^[x] 5. Redesign Doom MCP API (BREAKING)

  **What to do**:
  
  **BEFORE:**
  ```c
  // api.h - Inconsistent naming
  dmcp_context_t* dmcp_create(const dmcp_config_t* config);
  void dmcp_destroy(dmcp_context_t* ctx);
  void dmcp_tick(dmcp_context_t* ctx);
  bool dmcp_is_running(const dmcp_context_t* ctx);
  ```
  
  **AFTER:**
  ```c
  // api.h - Consistent naming with "context" type
  dmcp_context_t* dmcp_context_create(const dmcp_config_t* config);
  void dmcp_context_destroy(dmcp_context_t* ctx);
  void dmcp_context_tick(dmcp_context_t* ctx);
  bool dmcp_context_is_running(const dmcp_context_t* ctx);
  
  // Screenshot - consistent naming
  bool dmcp_screenshot_is_requested(const dmcp_context_t* ctx);
  dmcp_result_t dmcp_screenshot_submit(dmcp_context_t* ctx, ...);
  
  // Statistics - consistent naming
  void dmcp_stats_get(const dmcp_context_t* ctx, dmcp_stats_t* stats);
  
  // Utilities - keep but document
  void dmcp_snapshot_clear(dmcp_snapshot_t* snapshot);
  bool dmcp_snapshot_add_enemy(dmcp_snapshot_t* snapshot, ...);
  ```

  **Breaking Changes**:
  - `dmcp_create` → `dmcp_context_create`
  - `dmcp_destroy` → `dmcp_context_destroy`
  - `dmcp_tick` → `dmcp_context_tick`
  - `dmcp_is_running` → `dmcp_context_is_running`
  - `dmcp_screenshot_requested` → `dmcp_screenshot_is_requested`
  - `dmcp_submit_screenshot` → `dmcp_screenshot_submit`
  - `dmcp_get_stats` → `dmcp_stats_get`

  **Migration**:
  ```c
  // Before
  dmcp_context_t* ctx = dmcp_create(&config);
  dmcp_tick(ctx);
  dmcp_destroy(ctx);
  
  // After
  dmcp_context_t* ctx = dmcp_context_create(&config);
  dmcp_context_tick(ctx);
  dmcp_context_destroy(ctx);
  ```

  **Acceptance Criteria**:
  - [ ] All functions renamed consistently
  - [ ] Old function names removed
  - [ ] Examples updated
  - [ ] Tests updated

  **Commit**: `feat(api)!: Redesign Doom MCP API with consistent naming`
  
  **BREAKING CHANGE**: All function names changed to include "context"

- [ ] 6. Redesign Configuration (BREAKING)

  **What to do**:
  
  **BEFORE:**
  ```c
  // types.h - dmcp_config_t
  typedef struct {
      uint32_t struct_size;
      uint16_t port;
      uint32_t target_hz;
      // ... flat config
  } dmcp_config_t;
  
  // adapter.h - dmcp_zdoom_config_t
  typedef struct {
      uint32_t struct_size;
      const dmcp_config_t* dmcp_config;  // Pointer - inconsistent!
      // ...
  } dmcp_zdoom_config_t;
  ```
  
  **AFTER:**
  ```c
  // types.h - dmcp_config_t (unchanged structure)
  typedef struct {
      uint32_t struct_size;
      uint16_t port;
      uint32_t target_hz;
      // ...
  } dmcp_config_t;
  
  // adapter.h - dmcp_zdoom_config_t embeds directly
  typedef struct {
      uint32_t struct_size;
      dmcp_config_t base;  // Embedded, not pointer!
      // ZDoom-specific additions
      void (*log_fn)(void* user, int level, const char* message);
      bool (*should_tick_fn)(void* user);
  } dmcp_zdoom_config_t;
  
  // New naming for default config
  dmcp_config_t dmcp_config_default(void);  // Was: dmcp_default_config
  dmcp_zdoom_config_t dmcp_zdoom_config_default(void);
  ```

  **Breaking Changes**:
  - `dmcp_default_config` → `dmcp_config_default`
  - `dmcp_zdoom_default_config` → `dmcp_zdoom_config_default`
  - ZDoom config embeds dmcp_config directly (not pointer)

  **Migration**:
  ```c
  // Before
  dmcp_zdoom_config_t cfg = dmcp_zdoom_default_config();
  cfg.dmcp_config->port = 8080;  // Pointer indirection
  
  // After
  dmcp_zdoom_config_t cfg = dmcp_zdoom_config_default();
  cfg.base.port = 8080;  // Direct access
  ```

  **Acceptance Criteria**:
  - [ ] Config naming consistent
  - [ ] ZDoom config uses embedding
  - [ ] All usages updated
  - [ ] Tests updated

  **Commit**: `feat(api)!: Unify configuration patterns`
  
  **BREAKING CHANGE**: ZDoom config structure changed from pointer to embedding

### Phase 3: Features

- [ ] 7. Implement Transport Registry

  **What to do**:
  ```c
  // transport.h additions
  mcp_result_t mcp_transport_register(const char* name,
                                       const mcp_transport_interface_t* interface);
  mcp_result_t mcp_transport_unregister(const char* name);
  const mcp_transport_interface_t* mcp_transport_get(const char* name);
  
  // server_config_t addition
  typedef struct {
      uint32_t struct_size;
      uint16_t port;
      const char* transport_name;  // "sse", "websocket", etc.
      // ...
  } mcp_server_config_t;
  ```

  **Breaking Changes**: None (additive)

  **Acceptance Criteria**:
  - [ ] Transport registry implemented
  - [ ] Multiple transports can be registered
  - [ ] Server can select transport by name

  **Commit**: `feat(api): Add transport registry`

- [ ] 8. Complete TODO Items

  **What to do**:
  - Implement screenshot endpoint
  - Add PNG encoding (use stb_image_write)
  - Implement command acknowledgment

  **Breaking Changes**: None (additive)

  **Acceptance Criteria**:
  - [ ] Screenshot endpoint serves PNG
  - [ ] PNG encoding works
  - [ ] Command acknowledgment implemented

  **Commit**: `feat: Complete TODO items`

### Phase 4: Quality

- [ ] 9. Write Comprehensive Tests

  **What to do**:
  - Unit tests for all public APIs
  - Test error handling
  - Test edge cases
  - Integration tests

  **Acceptance Criteria**:
  - [ ] >70% code coverage
  - [ ] All tests pass

  **Commit**: `test: Add comprehensive test suite`

- [ ] 10. Add Documentation

  **What to do**:
  - Document all public APIs
  - Add usage examples
  - Update README

  **Acceptance Criteria**:
  - [ ] All functions documented
  - [ ] README updated

  **Commit**: `docs: Add comprehensive API documentation`

- [ ] 11. Update Examples

  **What to do**:
  - Update dummy_server.cpp
  - Create minimal_server.c
  - Update adapter examples

  **Acceptance Criteria**:
  - [ ] All examples compile
  - [ ] All examples run

  **Commit**: `examples: Update to new API`

### Phase 5: Release

- [ ] 12. Write Migration Guide

  **What to do**:
  - Document all breaking changes
  - Provide before/after examples
  - Create migration script if possible

  **Deliverable**: `MIGRATION.md`

  **Commit**: `docs: Add migration guide`

- [ ] 13. Final Integration Testing

  **What to do**:
  - Run all tests
  - Run integration tests
  - Verify examples
  - Check for memory leaks

  **Commit**: `chore: Final integration testing`

- [ ] 14. Version Bump and Release

  **What to do**:
  - Bump version to 0.5.0 or 1.0.0
  - Write release notes
  - Tag release

  **Commit**: `chore(release): Bump version to 0.5.0`

---

## Migration Guide (Draft)

### Quick Reference Table

| Old API | New API | Change Type |
|---------|---------|-------------|
| `dmcp_create()` | `dmcp_context_create()` | Renamed |
| `dmcp_destroy()` | `dmcp_context_destroy()` | Renamed |
| `dmcp_tick()` | `dmcp_context_tick()` | Renamed |
| `dmcp_is_running()` | `dmcp_context_is_running()` | Renamed |
| `dmcp_get_stats()` | `dmcp_stats_get()` | Renamed |
| `dmcp_screenshot_requested()` | `dmcp_screenshot_is_requested()` | Renamed |
| `dmcp_submit_screenshot()` | `dmcp_screenshot_submit()` | Renamed |
| `dmcp_default_config()` | `dmcp_config_default()` | Renamed |
| `dmcp_zdoom_default_config()` | `dmcp_zdoom_config_default()` | Renamed |
| `mcp_server_register_method()` | `mcp_server_method_register()` | Renamed |
| `mcp_server_broadcast()` | `mcp_server_event_broadcast()` | Renamed |
| `mcp_result_t` (enum) | `mcp_result_t` (struct) | Type changed |

### Detailed Migration Examples

**1. Context Creation:**
```c
// Before
#include "dmcp/doom/api.h"
dmcp_config_t config = dmcp_default_config();
dmcp_context_t* ctx = dmcp_create(&config);

// After
#include "dmcp/doom/api.h"
dmcp_config_t config = dmcp_config_default();
dmcp_context_t* ctx = dmcp_context_create(&config);
```

**2. Error Handling:**
```c
// Before
if (dmcp_push_command(ctx, &cmd) != DMCP_OK) {
    fprintf(stderr, "Command failed\n");
}

// After
dmcp_result_t result = dmcp_push_command(ctx, &cmd);
if (result.code != 0) {
    fprintf(stderr, "Command failed: %s\n", result.message);
}
```

**3. ZDoom Configuration:**
```c
// Before
dmcp_zdoom_config_t cfg = dmcp_zdoom_default_config();
dmcp_config_t base = dmcp_default_config();
base.port = 8080;
cfg.dmcp_config = &base;  // Pointer!

// After
dmcp_zdoom_config_t cfg = dmcp_zdoom_config_default();
cfg.base.port = 8080;  // Embedded!
```

---

## Success Criteria

### Build Verification
```bash
rm -rf build
cmake -B build -S .
cmake --build build
# Should have zero warnings
```

### Test Verification
```bash
ctest --test-dir build --output-on-failure
# Should show 100% pass rate
```

### Example Verification
```bash
./build/examples/dummy_server &
sleep 1
curl http://localhost:6060/health
# Should return {"status":"ok"}
pkill dummy_server
```

### Code Quality Verification
```bash
# No magic numbers
grep -r "8192\|16384\|6060" src/ include/ || echo "PASS: No magic numbers"

# No deprecated APIs
grep -r "dmcp_create\|dmcp_destroy\|dmcp_tick" include/ src/ || echo "PASS: No old APIs"
```

---

## Version Strategy

**Recommended: 0.5.0 (major refactoring)**

Alternative: 1.0.0 (if this is considered stable release)

**Rationale:**
- Breaking changes warrant minor version bump (0.4.0 → 0.5.0)
- Not 1.0.0 yet because:
  - May have more breaking changes planned
  - Not battle-tested in production
  - Some features still TODO

---

*Breaking Changes Refactor Plan*  
*Generated by Prometheus on 2026-01-28*
