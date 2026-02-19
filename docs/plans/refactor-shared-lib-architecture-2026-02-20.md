# DMCP Refactoring Plan - Clean Architecture & Shared Library Support

**Status**: Planning  
**Created**: 2026-02-20  
**Updated**: 2026-02-20  

## Summary

Refactor DMCP codebase to achieve clean SRP-compliant architecture with proper shared library (.so/.dll) support, remove game-specific pollution from generic MCP layer, and enable adapters to use DMCP as a shared library.

## Context

**Why**: 
- Current code has SRP violations (god objects, monster files)
- Game constants polluting generic MCP layer
- Shared library support is broken (missing exports, wrong visibility)
- Adapters tightly coupled to static library paths

**Constraints**:
- Maintain public API compatibility (don't break consumers)
- Linux shared library support first (Windows later)
- Both Chocolate Doom and ZDoom adapters must work
- All existing tests must pass

**Architecture**: 
- Generic MCP Layer (`include/mcp/generic/`) - Protocol only, game-agnostic
- DMCP Core Layer (`include/dmcp/doom/`) - Game logic, owns game constants  
- Adapter Helpers (`include/dmcp/adapter/`) - Shared utilities (optional)
- Engine Adapters (`adapters/*/`) - Engine-specific integration

**Related**:
- Previous work: Command result tracking, validation (commit 6711b0f)
- Architecture analysis: See task exploration results

## Key Files

### Generic MCP Layer (to clean)
- `include/mcp/generic/constants.h` — Remove game constants
- `include/mcp/generic/protocol.h` — Make server name configurable
- `include/mcp/generic/server.h` — Add export macros
- `include/mcp/generic/*.h` — All public headers need MCP_API

### DMCP Core Layer (to refactor)
- `src/doom/internal/context.hpp` — Split into subsystems
- `src/doom/commands.cpp` — Modularize (803 lines)
- `src/doom/mcp_handlers.cpp` — Split handlers (822 lines)
- `include/dmcp/doom/*.h` — Add DMCP_API exports

### CMake/Build (to fix)
- `CMakeLists.txt` — Fix visibility, add exports, package config
- `cmake/dmcp-config.cmake.in` — Create package config template
- `adapters/chocolate-doom/CMakeLists.txt` — Support shared libs
- `adapters/zdoom/CMakeLists.txt` — Support shared libs

## Tasks

### Phase 1: Shared Library Foundation

- [x] **Task 1.1**: Create export macros header for generic MCP layer
  - Objective: Define MCP_API macro supporting Windows (__declspec) and GCC/Clang (visibility)
  - Files: `include/mcp/generic/export.h` (new)
  - Done when: Header exists with proper platform detection, integrated into build
  - Commit hint: `build(mcp): add MCP_API export macros for shared library support`
  - **Completed**: 4565c01 - Build and tests pass ✓

- [x] **Task 1.2**: Create export macros header for DMCP core layer
  - Objective: Define DMCP_API macro for Doom MCP public API
  - Files: `include/dmcp/doom/export.h` (new)
  - Done when: Header exists, uses same pattern as MCP_API
  - Commit hint: `build(dmcp): add DMCP_API export macros for shared library support`
  - **Completed**: 9bc7057 - Build and tests pass ✓

- [x] **Task 1.3**: Annotate all generic MCP public APIs with MCP_API
  - Objective: Add MCP_API to all mcp_* functions in public headers
  - Files: `include/mcp/generic/server.h`, `include/mcp/generic/protocol.h`, `include/mcp/generic/result.h`, `include/mcp/generic/constants.h`
  - Done when: All public functions have MCP_API, builds successfully
  - Commit hint: `build(mcp): annotate public APIs with MCP_API export macro`
  - **Completed**: ccd115c - 13 functions annotated, all tests pass ✓

- [x] **Task 1.4**: Annotate all DMCP core public APIs with DMCP_API
  - Objective: Add DMCP_API to all dmcp_* functions in public headers
  - Files: `include/dmcp/doom/api.h`, `include/dmcp/doom/commands.h`, `include/dmcp/doom/config.h`, `include/dmcp/doom/dmcp.h`, `include/dmcp/doom/types.h`
  - Done when: All public functions have DMCP_API, builds successfully
  - Commit hint: `build(dmcp): annotate public APIs with DMCP_API export macro`
  - **Completed**: 5fa2ae3 - 20 functions annotated, symbols exported ✓

- [x] **Task 1.5**: Fix CMake visibility logic for shared libraries
  - Objective: Apply hidden visibility to SHARED builds (currently backwards), define MCP_BUILDING/DMCP_BUILDING when building
  - Files: `CMakeLists.txt`
  - Done when: Shared builds have hidden visibility, static builds don't, BUILDING macros defined
  - Commit hint: `build(cmake): fix visibility logic and add BUILDING macros for shared libs`
  - **Completed**: bd33b46 - Visibility fixed, BUILDING macros added ✓

- [x] **Task 1.6**: Fix dependency PIC timing issue
  - Objective: Ensure uSockets and yyjson are compiled with -fPIC before being linked
  - Files: `CMakeLists.txt`, `cmake/Dependencies.cmake`
  - Done when: All dependencies have POSITION_INDEPENDENT_CODE set correctly
  - Commit hint: `build(cmake): fix PIC timing for dependencies in shared builds`
  - **Completed**: 3143a90 - PIC settings moved to Dependencies.cmake ✓

- [x] **Task 1.7**: Create CMake package config for find_package(dmcp) support
  - Objective: Enable find_package(dmcp) for external projects using installed DMCP
  - Files: `cmake/dmcp-config.cmake.in` (new), `CMakeLists.txt`
  - Done when: Config file created, install rules added, exports dmcp::core and dmcp::generic targets
  - Commit hint: `build(cmake): add package config for find_package(dmcp) support`
  - **Completed**: 9f5f080 - find_package(dmcp) works, targets exported ✓

### Phase 2: Clean Generic MCP Layer

- [x] **Task 2.1**: Move game-specific constants from generic to DMCP layer
  - Objective: Remove MCP_MAX_ENEMIES, MCP_MAX_INVENTORY, etc. from generic constants
  - Files: `include/mcp/generic/constants.h`, `include/dmcp/doom/constants.h` (create/expand)
  - Done when: No game concepts in generic layer, DMCP has its own constants
  - Commit hint: `refactor(constants): move game-specific constants to DMCP layer`
  - **Completed**: 8e33ef4 - 6 constants moved, tests pass ✓

- [x] **Task 2.2**: Make server name configurable in protocol
  - Objective: Remove hardcoded "doom-mcp" from protocol.h, add server_name field to config
  - Files: `include/mcp/generic/protocol.h`, `include/mcp/generic/server.h`, `src/mcp/server.cpp`
  - Done when: Server name configurable via mcp_server_config_t, defaults to "doom-mcp"
  - Commit hint: `feat(config): make server name configurable in mcp_server_config_t`
  - **Completed**: 1eca066 - server_name field added, configurable ✓

### Phase 3: Architecture Refactoring (SRP)

- [ ] **Task 3.1**: Split context into subsystem managers
  - Objective: Break god object into server_manager, command_manager, snapshot_manager, screenshot_manager
  - Files: `src/doom/internal/context.hpp`, `src/doom/internal/subsystems/*.hpp` (new), `src/doom/context.cpp`
  - Done when: context only holds pointers to subsystems, each subsystem in its own file
  - Commit hint: `refactor(context): split into subsystem managers for SRP compliance`

- [ ] **Task 3.2**: Modularize commands.cpp - extract queue and parsers
  - Objective: Split commands.cpp (803 lines) into focused modules
  - Files: `src/doom/commands/queue.cpp`, `src/doom/commands/parsers.cpp`, `src/doom/commands/validators.cpp`, `src/doom/commands.cpp` (trimmed)
  - Done when: Queue logic, JSON parsing, and validation in separate files
  - Commit hint: `refactor(commands): modularize queue, parsers, and validators`

- [ ] **Task 3.3**: Modularize commands.cpp - per-command-type parsing
  - Objective: Create separate files for each command type's parsing logic
  - Files: `src/doom/commands/types/spawn.cpp`, `src/doom/commands/types/change_level.cpp`, `src/doom/commands/types/give_item.cpp`, etc.
  - Done when: Each command type has its own parsing file
  - Commit hint: `refactor(commands): split per-command-type parsing into separate files`

- [ ] **Task 3.4**: Modularize mcp_handlers.cpp - extract tool handlers
  - Objective: Split tool handlers into separate files
  - Files: `src/doom/handlers/tools/get_game_state.cpp`, `src/doom/handlers/tools/get_screenshot.cpp`, `src/doom/handlers/tools/execute_command.cpp`, `src/doom/handlers/tools/get_command_result.cpp`
  - Done when: Each tool handler in its own file
  - Commit hint: `refactor(handlers): split tool handlers into separate files`

- [ ] **Task 3.5**: Modularize mcp_handlers.cpp - extract routes and methods
  - Objective: Split route handlers and method handlers
  - Files: `src/doom/handlers/routes.cpp`, `src/doom/handlers/methods.cpp`, `src/doom/handlers/common.cpp`
  - Done when: Routes, methods, and common utilities separated
  - Commit hint: `refactor(handlers): split routes and methods into separate files`

### Phase 4: Adapter Integration

- [ ] **Task 4.1**: Create adapter helpers layer - utilities
  - Objective: Create shared adapter utilities for coordinate conversion, validation
  - Files: `include/dmcp/adapter/utils.h` (new)
  - Done when: Fixed-point conversions, angle conversions, basic validation in shared header
  - Commit hint: `feat(adapter): create shared adapter utilities layer`

- [ ] **Task 4.2**: Create adapter helpers layer - entity mappings
  - Objective: Create shared entity name mappings (Zombieman -> standardized types)
  - Files: `include/dmcp/adapter/entities.h` (new)
  - Done when: Entity class name constants and validation functions
  - Commit hint: `feat(adapter): add shared entity name mappings`

- [ ] **Task 4.3**: Create adapter helpers layer - validation functions
  - Objective: Create shared validation functions (health ranges, position bounds)
  - Files: `include/dmcp/adapter/validation.h` (new)
  - Done when: Validation functions for health, timescale, position
  - Commit hint: `feat(adapter): add shared validation functions`

- [ ] **Task 4.4**: Fix Chocolate Doom CMake for shared library support
  - Objective: Replace hardcoded .a paths with find_library(), support both shared and static
  - Files: `chocolate-doom/src/doom/CMakeLists.txt`, `adapters/chocolate-doom/CMakeLists.txt`
  - Done when: Chocolate Doom builds with DMCP as shared library
  - Commit hint: `build(chocolate): support shared library linking`

- [ ] **Task 4.5**: Add data validation to Chocolate Doom adapter
  - Objective: Use adapter helpers to validate command data (health ranges, positions)
  - Files: `adapters/chocolate-doom/commands.c`, `adapters/chocolate-doom/dmcp_mappings.h`
  - Done when: All commands validate inputs, log warnings for invalid data
  - Commit hint: `feat(chocolate): add command data validation using adapter helpers`

- [ ] **Task 4.6**: Fix ZDoom CMake for shared library support
  - Objective: Add proper find_package support, remove hardcoded paths
  - Files: `adapters/zdoom/CMakeLists.txt`
  - Done when: ZDoom builds with DMCP as shared library
  - Commit hint: `build(zdoom): support shared library linking`

- [ ] **Task 4.7**: Add data validation to ZDoom adapter
  - Objective: Mirror Chocolate Doom validation in ZDoom adapter
  - Files: `adapters/zdoom/adapter.cpp`, `adapters/zdoom/commands.cpp`
  - Done when: All commands validate inputs
  - Commit hint: `feat(zdoom): add command data validation`

### Phase 5: Testing & Validation

- [ ] **Task 5.1**: Verify shared library build on Linux
  - Objective: Build with DMCP_BUILD_SHARED=ON, verify symbol visibility
  - Files: N/A (testing only)
  - Done when: nm -D libdmcp_core.so shows expected symbols, no missing exports
  - Commit hint: `test(build): verify shared library symbol exports`

- [ ] **Task 5.2**: Run all tests with shared library build
  - Objective: Ensure all 36 unit tests and 8 e2e tests pass with shared libraries
  - Files: N/A (testing only)
  - Done when: ctest --test-dir build-shared passes 100%
  - Commit hint: `test(ci): validate shared library builds pass all tests`

- [ ] **Task 5.3**: Integration test - Chocolate Doom with shared DMCP
  - Objective: Build Chocolate Doom against shared DMCP, run integration tests
  - Files: N/A (testing only)
  - Done when: Chocolate Doom links and runs correctly with shared DMCP
  - Commit hint: `test(integration): validate Chocolate Doom with shared DMCP`

## Dependency Table

| Task | Component | Depends On |
|------|-----------|------------|
| 1.1 Export macros (MCP_API) | Generic MCP | None |
| 1.2 Export macros (DMCP_API) | DMCP Core | None |
| 1.3 Annotate MCP APIs | Generic MCP | 1.1 |
| 1.4 Annotate DMCP APIs | DMCP Core | 1.2 |
| 1.5 CMake visibility | Build | None |
| 1.6 Dependency PIC | Build | None |
| 1.7 CMake package config | Build | 1.5, 1.6 |
| 2.1 Move game constants | Constants | None |
| 2.2 Configurable server name | Protocol | None |
| 3.1 Split context | Context | None |
| 3.2 Modularize commands (queue/parsers) | Commands | None |
| 3.3 Modularize commands (per-type) | Commands | 3.2 |
| 3.4 Split tool handlers | Handlers | None |
| 3.5 Split routes/methods | Handlers | 3.4 |
| 4.1 Adapter utils | Adapter | None |
| 4.2 Adapter entities | Adapter | None |
| 4.3 Adapter validation | Adapter | None |
| 4.4 Chocolate CMake | Chocolate | 1.7 |
| 4.5 Chocolate validation | Chocolate | 4.1, 4.3 |
| 4.6 ZDoom CMake | ZDoom | 1.7 |
| 4.7 ZDoom validation | ZDoom | 4.1, 4.3 |
| 5.1 Verify shared build | Testing | 1.1-1.7, 2.1-2.2 |
| 5.2 Run tests (shared) | Testing | 5.1 |
| 5.3 Chocolate integration | Testing | 4.4, 4.5, 5.2 |

## Decisions Log

- 2026-02-20: Adapter helpers layer approved - reduces code duplication between adapters while keeping core clean
- 2026-02-20: Windows support deferred - export macros will include Windows support but not tested yet
- 2026-02-20: Generic MCP layer must remain game-agnostic - all game constants move to DMCP layer
- 2026-02-20: Public API compatibility maintained - internal refactoring won't break consumers
- 2026-02-20: Route registration pattern confirmed correct - DMCP owns its routes, generic layer provides registration API

## Notes

### Parallel Execution Groups

**Group A (Independent, can run in parallel):**
- Tasks 1.1, 1.2, 1.5, 1.6, 2.1, 2.2, 3.1, 3.2, 3.4, 4.1, 4.2, 4.3

**Group B (Depends on Group A):**
- Tasks 1.3, 1.4, 1.7, 3.3, 3.5

**Group C (Depends on Group B):**
- Tasks 4.4, 4.5, 4.6, 4.7, 5.1, 5.2, 5.3

### Patterns to Follow

1. **Export macros**: Use same pattern in both MCP_API and DMCP_API
2. **Subsystem pattern**: Each subsystem has create/destroy/tick methods
3. **Command pattern**: Each command type has parse/validate/execute functions
4. **Handler pattern**: Each tool/route/method handler in separate file with consistent naming
5. **Adapter helpers**: Static inline functions for performance, no dependencies on engine headers

### Risk Areas

- **Context refactoring**: High touch surface, requires careful state management
- **CMake changes**: Affects all builds, must test both static and shared
- **Export annotations**: Missing exports = runtime failures, must be exhaustive

### Resume Context

If interrupted, resume from:
1. Check which phase is complete (1-5)
2. Within phase, check task dependency table
3. Run `git status` to see current work state
4. Run `cmake --build build && ctest --test-dir build` to verify current state
Amendment: Single Shared Library Design

Decision: Combine libmcp_generic and libdmcp_core into single libdmcp.so

Rationale:
- Simpler distribution (one file instead of two)
- No need for users to link multiple libraries
- Cleaner CMake target

Updated Tasks:
- Task 1.5: Single library approach
- Task 1.7: Export single dmcp::dmcp target
## Build Cleanup Task

### Phase 6: Build Cleanup (New)

- [x] **Task 6.1**: Clean and fix all build errors, warnings, and issues
  - Objective: Address all compiler warnings, build errors, and CMake issues across all configurations
  - Check: Static build (-Wall -Wextra), Shared build, Debug build, Release build
  - Files: All source files with warnings
  - Done when: Zero warnings with -Wall -Wextra on GCC/Clang, no CMake warnings
  - Commit hint: `build(cleanup): fix all compiler warnings and build issues`
  - **Completed**: 7853612 - Zero warnings with -Wall -Wextra ✓
