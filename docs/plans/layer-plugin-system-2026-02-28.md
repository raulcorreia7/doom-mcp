# Layer Plugin System for DMCP

**Status**: Complete
**Created**: 2026-02-28
**Updated**: 2026-02-28

## Summary
Create a composable layer/plugin system for DMCP that splits functionality into independent integration layers (Orchestrator, Input) that can be enabled/disabled via configuration, all sharing a single MCP server with namespaced tools.

## Context
- **Why**: Enable multiple integration patterns for agents - some need game orchestration (spawn, change_level), others need human-like input control (player_input). Currently all tools are monolithic.
- **Constraints**: 
  - Single MCP server (shared port)
  - C-compatible public API
  - No breaking changes to existing adapter integrations
- **Architecture**: [ARCHITECTURE.md](../ARCHITECTURE.md)
- **Related**: None

## Key Files
- `include/dmcp/doom/layer.h` — Layer interface and types
- `include/dmcp/doom/config.h` — Layer enable configuration
- `src/doom/layers/registry.cpp` — Layer registry implementation
- `src/doom/layers/orchestrator/` — Orchestrator layer (existing tools)
- `src/doom/layers/input/` — Input layer (player_input)
- `src/doom/context.cpp` — Layer registration integration

## Completed Tasks
- [x] Task 1: Define Layer Interface and Types (088ac33)
- [x] Task 2: Create Layer Registry (fbd1514)
- [x] Task 3: Update Configuration for Layer Selection (feat)
- [x] Task 4: Extract Orchestrator Layer (refactor)
- [x] Task 5: Extract Input Layer (refactor)
- [x] Task 6: Integrate Layers into Context (fbd1514)
- [x] Task 7: Add Unit Tests for Layer System (test)
- [x] Task 8: Update Architecture Documentation (e692b83)

## Parallelization
- **Independent**: [1], [3], [4], [5] can run in parallel
- **Depends on [1]**: [2]
- **Integration point**: [6] depends on [2], [3], [4], [5]
- **Follow integration**: [7], [8]

## Decisions Log
- 2026-02-28: Chose single server/shared port model over separate ports per layer for simpler agent integration
- 2026-02-28: Initial layers: Orchestrator (game management) + Input (human-like control)

## Notes
- Keep backward compatibility: default config enables all layers (current behavior)
- Consider namespacing tools: `orchestrator.spawn_entity` vs `spawn_entity`
- Future layers could include: analytics, replay, multiplayer coordination
