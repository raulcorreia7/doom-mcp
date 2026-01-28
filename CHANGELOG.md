# Changelog

All notable changes to the Doom Model Context Protocol (DMCP) SDK will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.5.0] - 2026-01-28

### BREAKING CHANGES

This release includes a comprehensive API redesign introducing a consistent type-oriented naming convention and rich error handling throughout the codebase. See [MIGRATION.md](MIGRATION.md) for detailed migration instructions.

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

- `mcp_result_t` changed from `enum` to `struct` with `code` and `message` fields
- `dmcp_result_t` changed from `enum` to `struct` with `code` and `message` fields
- All error sites now provide descriptive messages
- New convenience macros: `MCP_RESULT_MAKE()`, `MCP_RESULT_OK`, `MCP_ERROR_*`
- Result code constants: `MCP_RESULT_CODE_OK`, `MCP_RESULT_CODE_INVALID_ARGS`, etc.

**Migration:**
```c
// Before
if (mcp_server_register_method(...) != MCP_OK) {
    fprintf(stderr, "Error: %d\n", result);
}

// After
mcp_result_t result = mcp_server_method_register(...);
if (result.code != MCP_RESULT_CODE_OK) {
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

#### Comprehensive Migration Guide
- Created [MIGRATION.md](MIGRATION.md) with detailed migration instructions
- Quick reference table for all 17 breaking changes
- Before/after code examples for each change
- Troubleshooting section with common issues
- Migration checklist

#### API Documentation
- Added JSDoc-style documentation to all public headers
- Documented all parameters, return values, error conditions
- Included breaking change notes and migration examples
- Updated README with new API examples

#### Examples
- Updated dummy_server.cpp to use new API
- Created examples/README.md with migration guidance
- All examples demonstrate type-oriented naming pattern

### Fixed

- All deprecated APIs removed (no backward compatibility)
- All magic numbers replaced with named constants
- Consistent error messages across all APIs
- Test infrastructure properly configured for conditional adapter builds

### Performance Improvements

- Batch method registration uses single mutex lock (reduced contention)
- Hash map capacity reservation prevents reallocations during batch operations
- More efficient client count query (returns actual count, no conversion needed)

### Developer Experience

- Type-oriented naming makes API more discoverable
- Rich error messages reduce debugging time
- Comprehensive test suite provides confidence
- Clear migration path for existing users

---

## [0.4.0] - Previous Release

### Features
- Initial release of DMCP SDK
- Generic MCP protocol implementation
- Doom MCP layer for game state management
- ZDoom adapter for engine integration
- Server-Sent Events (SSE) transport
- JSON-RPC protocol support
- Screenshot management
- Command processing system

### API
- Basic server lifecycle (create, destroy, is_running)
- Method registration
- Event broadcasting
- Statistics tracking
- Snapshot callbacks
- Configuration management

---

### Upgrade Path from v0.4.0 to v0.5.0

1. **Read the Migration Guide**: Start with [MIGRATION.md](MIGRATION.md)
2. **Update Function Calls**: Use the Quick Reference Table to rename all API calls
3. **Update Error Handling**: Change from enum comparison to struct `.code` and `.message` access
4. **Update Configuration**: Change `dmcp_zdoom_config_t.dmcp_config->port` to `cfg.base.port`
5. **Rebuild**: Clean build to ensure all references updated
6. **Run Tests**: Verify your integration works with new API
7. **Check Examples**: Reference updated examples in `examples/` directory

**Note**: This is a breaking release with no backward compatibility. All deprecated APIs have been removed.
