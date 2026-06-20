# Changelog

All notable changes to the Doom Model Context Protocol (DMCP) SDK will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Current public C APIs use `mcp_status_t` from `include/mcp/core/status.h`.
Older release notes below may mention removed result aliases only as migration
history.

## [Unreleased]

## [0.7.0] - 2026-06-20

### Breaking Changes
- Public MCP command execution now uses direct `tools/call` tool names with structured `arguments`.
- Removed the public `execute_command` MCP tool and native JSON-RPC method aliases.
- Removed public content-name aliases and input shorthand fields. Use canonical names from `get_available_*` content tools.
- Renamed public C helpers for one naming convention:
  `mcp_server_config_default()`, `dmcp_command_push/pop/clear/has_pending()`,
  `dmcp_input_push/pop/clear/has_pending()`, and adapter
  `*_context_get()` / `*_stats_get()` accessors.
- Superseded package-config install guidance with local source/build artifact integration; DMCP is not installed as a system library by default
- Target names changed: `dmcp::dmcp_generic` → `dmcp::generic`, `dmcp::dmcp_core` → `dmcp::core`
- Library output names: `libdmcp_generic.so`/`libdmcp_core.so` (previously `libdmcp_dmcp_generic.so`)
- Protocol constants consolidated: raw string literals replaced with centralized constants in `include/dmcp/doom/protocol.h`
- MCP lifecycle is now per-session (not server-global) - `initialize` returns `sessionId` which must be included in subsequent requests via `MCP-Session-Id` HTTP header
- Removed backwards-compatibility aliases and deprecated constant names

### Added
- Granular available-content tools: `get_available_enemies`, `get_available_entities`, `get_available_items`, `get_available_weapons`, `get_available_ammo`, `get_available_keys`, `get_available_maps`, and `get_available_giveable`.
- Per-session lifecycle state tracking to avoid cross-client state leakage
- MCP JSON-RPC envelope validation
- MCP initialize parameter validation
- Protocol negotiation errors and pre-initialization request gating
- Linker version scripts for symbol visibility control on Linux/Unix
- Shared library build support with proper symbol export control
- Installed adapter headers at `include/dmcp/adapters/` for C API boundary

### Fixed
- SSE implementation aligned with MCP spec (Accept header, Content-Type, Cache-Control headers)
- Unknown endpoint responses no longer leak transport implementation details

### Changed
- `execute_batch` now accepts `calls: [{name, arguments}]`.
- `player_input` now uses `action` and optional `value`.
- `get_available_content` returns available-only canonical content for the active game mode, including enemies, entities, weapons, ammo, keys, items, giveable content, maps, and weapon-slot metadata.
- Internal C++ helpers now use lower-snake function names; C ABI/API names stay
  lower-snake and macro/enum constants stay upper-snake.
- Object library pattern for shared/static builds
- Simplified CMake config file for relocatable install
- Package version compatibility: `SameMajorVersion`

## [0.6.0] - 2026-02-13

### Breaking Changes
- Removed `dmcp_result_t` type alias - use `mcp_status_t` instead
- Removed all `DMCP_STATUS_CODE_*` macros - use `MCP_STATUS_CODE_*` instead
- Removed all `DMCP_LOG_*` macros - use `MCP_LOG_*` instead
- Removed `DMCP_OK` and `DMCP_ERROR_*` macros - use `MCP_STATUS_OK()` and `MCP_STATUS_ERROR()` instead
- Removed `dmcp_log_level_t` type alias - use `mcp_log_level_t` instead
- `dmcp_zdoom_tick()` now returns `mcp_status_t` instead of `int`
- `dmcp_screenshot_submit()` now returns `MCP_STATUS_CODE_DISABLED` (NYI)
- Added `struct_size` field to `mcp_server_stats_t`, `dmcp_stats_t`, `mcp_transport_callbacks_t`
- Added `version` field to `mcp_transport_interface_t`
- Added `_reserved` field to `dmcp_command_t` union

### Added
- `include/dmcp/doom/config.h` - Configuration types (extracted from types.h)
- `include/dmcp/doom/dmcp.h` - Convenience header that includes all DMCP headers
- `src/doom/handlers.cpp` - MCP method handlers (extracted from context.cpp)
- `src/doom/pool.cpp` - Snapshot pool management (extracted from context.cpp)
- `src/doom/serialization.cpp` - JSON serialization (extracted from context.cpp)
- `src/doom/internal/pool.hpp` - Pool internals
- `src/doom/internal/command_queue.hpp` - Command queue internals
- `src/doom/internal/screenshot.hpp` - Screenshot state internals
- `src/doom/internal/context.hpp` - Context internals
- `cmake/CPM.cmake` - CPM package manager setup
- `cmake/Dependencies.cmake` - Dependency declarations
- `cmake/CompilerWarnings.cmake` - Warning flag function
- `cmake/Sanitizers.cmake` - Sanitizer setup function
- `tests/test_utils.hpp` - Shared test fixtures and factories
- `tests/unit/` - Unit test directory
- `tests/integration/` - Integration test directory (ready for future)
- SSE transport callbacks now invoked (`on_sse_connect`, `on_sse_disconnect`, `on_sse_send`)
- Custom command parser lookup in `dmcp_parse_command_json`
- Command queue size limit (max 64 commands)
- Thread safety for custom parser registration
- Command queue tests (5 new test cases)
- Error path tests (invalid config, buffer overflow)

### Fixed
- SSE client double-free race condition (zombie flag pattern)
- JSON command params serialization (was hardcoded `"{}"`)
- ZDoom adapter now uses current API names
- `PauseGame` logic bug (removed redundant if/else)
- Duplicate code block in adapter removed
- Documentation example using wrong `strcpy` function

### Changed
- `src/doom/context.cpp` reduced from 541 lines to 178 lines (extracted handlers, pool, serialization)
- `include/dmcp/doom/types.h` now contains data types only (config moved to config.h)
- CMake now uses `CONFIGURE_DEPENDS` glob for automatic source detection
- CMake root reduced from 274 lines to ~100 lines using modular cmake/ files
- Memory management docs: "Object Pooling" instead of "Zero-Copy"
- Command parsers now scoped per-context (improved thread safety)
- Consolidated duplicate lambdas in `handle_tools_call`
- Version numbers unified across headers
- Test structure reorganized into unit/integration directories

### Removed
- nlohmann/json support (yyjson only now)
- All `DMCP_*` type aliases and macros (use `MCP_*` equivalents)
- Unused `DMCP_ERROR_SERVER_FAILED` macro
- Unused `dmcp_command_handler_t` typedef
- Unused `make_jsonrpc_response()` function
- Unused `Value::elements()` method
- Duplicate tests from `test_main.cpp`
- Duplicate documentation sections
- `command_queue_slots` config field for command/input queue capacity

## [0.5.0] - 2026-01-28

### BREAKING CHANGES

This release includes a comprehensive API redesign introducing a consistent type-oriented naming convention and rich error handling throughout the codebase.

#### API Naming Convention
All public APIs now follow `{namespace}_{type}_{action}` pattern for consistency and discoverability.

- **Generic MCP API**
  - `mcp_server_register_method()` → `mcp_server_method_register()`
  - `mcp_server_unregister_method()` → `mcp_server_method_unregister()`
  - `mcp_server_broadcast()` → `mcp_server_event_broadcast()`
  - `mcp_server_has_clients()` → `mcp_server_clients_count()` (returns `uint64_t` instead of `bool`)
  - `mcp_server_get_stats()` → `mcp_server_stats_get()`

- **Doom MCP API**
  - `dmcp_create()` → `dmcp_context_create()`
  - `dmcp_destroy()` → `dmcp_context_destroy()`
  - `dmcp_tick()` → `dmcp_context_tick()`
  - `dmcp_is_running()` → `dmcp_context_is_running()`
  - `dmcp_screenshot_requested()` → `dmcp_screenshot_is_requested()`
  - `dmcp_submit_screenshot()` → `dmcp_screenshot_submit()`
  - `dmcp_get_stats()` → `dmcp_stats_get()`

- **Adapter API**
  - `dmcp_zdoom_default_config()` → `dmcp_zdoom_config_default()`
  - `dmcp_zdoom_execute_command()` → `dmcp_zdoom_command_execute()`
  - `dmcp_zdoom_process_commands()` → `dmcp_zdoom_commands_process()`

- **Configuration**
  - `dmcp_default_config()` → `dmcp_config_default()`
  - `dmcp_zdoom_config_t.dmcp_config` pointer → `dmcp_zdoom_config_t.base` embedding

#### Result Type Redesign
Error handling now provides rich, human-readable messages.

- `mcp_status_t` changed from `enum` to `struct` with `code` and `message` fields
- `dmcp_result_t` changed from `enum` to `struct` with `code` and `message` fields
- All error sites now provide descriptive messages
- New convenience macros: `MCP_STATUS_MAKE()`, `MCP_STATUS_OK`, `MCP_ERROR_*`
- Result code constants: `MCP_STATUS_CODE_OK`, `MCP_STATUS_CODE_INVALID_ARGS`, etc.

**Migration:**
```c
// Before
if (mcp_server_register_method(...) != MCP_STATUS_OK("Success")) {
    fprintf(stderr, "Error: %d\n", result);
}

// After
mcp_status_t result = mcp_server_method_register(...);
if (result.code != MCP_STATUS_CODE_OK) {
    fprintf(stderr, "Error: %s\n", result.message);
}
```

### Added

#### Testing Infrastructure
- Added Catch2 v3.7.1 testing framework via CPM
- Created comprehensive test suite with >70% code coverage
- 281 test assertions across 25 test cases
- Tests for Generic MCP, Doom MCP, and Adapter APIs
- All public APIs tested with NULL safety and error paths

#### Batch Method Registration
- `mcp_server_methods_register()` for atomic multi-method registration
- Single mutex lock for entire batch operation (performance optimization)
- Hash map capacity reservation to prevent reallocations
- Zero-method batch support

#### Constants Header
- Created `include/mcp/generic/constants.h` for all magic numbers
- Eliminated magic numbers throughout codebase
- Named constants for buffer sizes, defaults, endpoints, array limits

#### Screenshot Endpoint
- `/screenshot/latest.png` endpoint serves PNG images
- Default 1x1 PNG buffer seeded on transport creation
- PNG encoding via stb_image_write

#### Transport Registry
- Runtime transport selection framework
- Interface for registering custom transports
- Future-proof design for WebSocket, HTTP/2, etc.

### Changed

#### Client Count Type
- `mcp_server_clients_count()` now returns `uint64_t` instead of `bool`
- Provides actual client count rather than boolean "has clients"
- More informative for monitoring and debugging

#### Configuration Structure
- `dmcp_zdoom_config_t` now embeds `dmcp_config_t` directly instead of pointer
- Access pattern: `cfg.dmcp_config->port` → `cfg.base.port`
- Consistent with other configuration patterns

### Documentation

- Added JSDoc-style documentation to all public headers
- Documented all parameters, return values, error conditions
- Updated README with API examples

### Fixed

- All deprecated APIs removed
- All magic numbers replaced with named constants
- Consistent error messages across all APIs
- Test infrastructure properly configured for conditional adapter builds

### Performance Improvements

- Batch method registration uses single mutex lock (reduced contention)
- Hash map capacity reservation prevents reallocations during batch operations
- More efficient client count query (returns actual count)

---

## [0.4.0] - Previous Release

- Initial release of DMCP SDK
- Generic MCP protocol implementation
- Doom MCP layer for game state management
- ZDoom adapter for engine integration
- Server-Sent Events (SSE) transport
- JSON-RPC protocol support

**Note**: This is a breaking release with no backward compatibility. All deprecated APIs have been removed.
