# Verify Layer Boundaries and Adapter Integration

**Status**: Complete
**Created**: 2026-02-20
**Updated**: 2026-02-20

## Summary
Verify and harden DMCP layer boundaries (Generic MCP, Doom core, adapters), then refactor integration hotspots for maintainability.

## Context
- **Why**: Current layering has boundary leaks and integration friction that increase maintenance risk and can hide build/runtime defects.
- **Constraints**: Preserve public C API compatibility; avoid broad engine refactors; keep Generic MCP game-agnostic; ship focused, low-conflict diffs.
- **Architecture**: `docs/ARCHITECTURE.md`
- **Related**: `docs/plans/refactor-shared-lib-architecture-2026-02-20.md`

## Key Files
- `include/mcp/generic/server.h` - Generic MCP public server contract
- `include/mcp/generic/transport.h` - Generic transport abstraction
- `include/dmcp/doom/config.h` - Doom core configuration surface
- `include/dmcp/adapter/content.h` - Content API currently consumed by core and adapters
- `src/doom/handlers/common.cpp` - Large mixed handler logic and command bridging
- `src/doom/content.cpp` - Content restrictions, aliases, and policy logic
- `src/doom/internal/json_types.hpp` - Doom wrapper over internal JSON types
- `src/mcp/json/json.hpp` - Internal JSON abstraction currently imported across layer boundary
- `adapters/zdoom/adapter.cpp` - ZDoom lifecycle and snapshot extraction
- `adapters/zdoom/commands.cpp` - ZDoom command execution and result completion
- `adapters/chocolate-doom/commands.c` - Chocolate command execution and content mapping

## Tasks
- [x] Task 1: Separate core and adapter content contracts (c352994)
  - Objective: Move ownership of content availability APIs to the Doom core boundary and keep adapter helpers utility-only.
  - Files: `include/dmcp/adapter/content.h`, `include/dmcp/doom/*`, `src/doom/content.cpp`, core call sites.
  - Done when: Doom core no longer depends on adapter-scoped content APIs and contracts are clearly owned by the DMCP Doom layer.

- [x] Task 2: Stabilize JSON boundary between generic and Doom core (4a1baeb)
  - Objective: Remove direct Doom-layer dependence on private/internal generic JSON headers.
  - Files: `src/mcp/json/json.hpp`, `src/doom/internal/json_types.hpp`, Doom handler/command includes.
  - Done when: Doom core uses a stable boundary header and no longer imports private generic JSON internals directly.

- [x] Task 3: Split handler common logic and remove dead subsystem placeholders (3ee210a)
  - Objective: Break `handlers/common.cpp` into focused modules and resolve unused subsystem abstractions under `src/doom/internal/subsystems/`.
  - Files: `src/doom/handlers/common.cpp`, new focused handler helper files, `src/doom/internal/subsystems/*.hpp`, `src/doom/internal/context.hpp`.
  - Done when: Handler code is modular by responsibility and subsystem abstractions are either implemented or removed.

- [x] Task 4: Fix ZDoom adapter integration correctness (5b8fd1d)
  - Objective: Make ZDoom adapter implementation coherent across compilation units and API contracts.
  - Files: `adapters/zdoom/adapter.h`, `adapters/zdoom/adapter.cpp`, `adapters/zdoom/commands.cpp`, related tests.
  - Done when: ZDoom adapter builds cleanly with `DMCP_BUILD_ADAPTER_ZDOOM=ON` and command/tick paths are type-safe and contract-consistent.

- [x] Task 5: Deduplicate Chocolate adapter mapping and utility logic (c850a36)
  - Objective: Remove duplicate conversion/string helper logic and centralize command/content mappings.
  - Files: `adapters/chocolate-doom/dmcp_mappings.h`, `adapters/chocolate-doom/commands.c`, `include/dmcp/adapter/utils.h`.
  - Done when: Conversion and mapping logic has a single source of truth and behavior remains backward compatible.

- [x] Task 6: Add layer and adapter regression gates (3c2eee2)
  - Objective: Add verification that blocks forbidden include directions and adapter integration regressions.
  - Files: `tests/unit/*`, `tests/e2e/*` (targeted), `CMakeLists.txt`, `tests/CMakeLists.txt`.
  - Done when: CI/local checks fail on boundary violations and cover adapter command/result flows for both adapters.

- [x] Task 7: Align architecture and adapter documentation with code (a8f242f)
  - Objective: Update architecture and adapter docs to match real file layout, contracts, and examples.
  - Files: `docs/ARCHITECTURE.md`, `docs/README.md`, `adapters/zdoom/README.md`, `adapters/chocolate-doom/README.md`, `README.md`.
  - Done when: Documentation matches current structure and examples compile against the current public API.

## Decisions Log
- 2026-02-20: Created a new plan file instead of modifying the completed refactor plan to keep historical closure and make this verification pass auditable.
- 2026-02-20: Prioritized boundary clarification and adapter correctness before new features.

## Notes
- Parallel execution groups:
  - Group A (independent): Task 1, Task 2, Task 4
  - Group B (depends on A): Task 3, Task 5
  - Group C (depends on B): Task 6
  - Group D (depends on A/B/C): Task 7
- Conflict-awareness:
  - Task 3 is the main hotspot (`src/doom/handlers/*`) and should run alone to avoid merge conflicts.
  - Task 4 and Task 5 can run in parallel because they touch separate adapter directories.
- Resume checklist:
  - Re-run build matrix with and without adapter flags.
  - Re-check include dependency direction after each boundary-related change.
