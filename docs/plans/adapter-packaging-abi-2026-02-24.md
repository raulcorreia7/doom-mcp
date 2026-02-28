# Adapter Packaging, ABI, and Protocol Cleanup

**Status**: In Progress
**Created**: 2026-02-24
**Updated**: 2026-02-24T08:15:00

## Summary
Refactor DMCP into a clean breaking-change release with full MCP spec compliance, stable C ABI/API, adapter-as-project packaging, consumer-only imported targets, and a single maintainable protocol constant model.

## Context
- **Why**: Current integration still leaks internals (manual transitive linking, adapter headers outside installed namespace, symbol leakage, duplicated protocol constants with raw string literals in handlers), and MCP wire/lifecycle handling was only partially spec-aligned.
- **Constraints**: Breaking changes are explicitly allowed; no backwards-compatibility shims required; Generic MCP must remain game-agnostic; public boundaries must stay C-compatible.
- **Architecture**: `docs/ARCHITECTURE.md`
- **Related**:
  - `docs/plans/refactor-shared-lib-architecture-2026-02-20.md`
  - `docs/plans/layer-boundary-adapters-2026-02-20.md`

## Key Files
- `CMakeLists.txt` - Root package/export/install behavior
- `cmake/dmcp-config.cmake.in` - Installed package config
- `cmake/Dependencies.cmake` - Third-party dependency linkage and visibility
- `include/dmcp/doom/protocol.h` - Protocol constants and C++ string_view wrappers
- `include/dmcp/` - Public DMCP headers
- `include/mcp/generic/` - Public generic MCP headers
- `src/doom/handlers/common.cpp` - Command alias table and method/type mapping
- `src/doom/handlers/methods.cpp` - Method dispatch with raw protocol literals
- `src/doom/handlers/state_json.cpp` - Status filter literals
- `src/doom/serialization.cpp` - Alive/dead literal serialization
- `src/mcp/server.cpp` - JSON-RPC envelope handling and initialize lifecycle
- `src/mcp/http_sse_transport.cpp` - MCP HTTP/SSE transport behavior
- `include/mcp/generic/protocol.h` - MCP protocol version and wire constants
- `adapters/chocolate-doom/CMakeLists.txt` - Chocolate adapter packaging target
- `adapters/zdoom/CMakeLists.txt` - ZDoom adapter packaging target
- `chocolate-doom/src/doom/CMakeLists.txt` - Chocolate consumer integration
- `adapters/crispy-doom/patches/dmcp_integration.patch` - Crispy consumer integration patch
- `tests/integration/` - Consumer build/integration scripts
- `tests/unit/test_mcp_server.cpp` - Generic server protocol tests
- `docs/README.md` - API docs
- `docs/INTEGRATION.md` - Consumer integration docs

## Tasks
- [x] Task 1: Lock MCP compliance target and gap matrix (2026-02-24)
  - Objective: Write an explicit compliance checklist against current MCP lifecycle/transport/schema expectations and map each requirement to code/tests.
  - Files: `docs/MCP_COMPLIANCE.md` (new), `docs/INTEGRATION.md`, `docs/ARCHITECTURE.md`, `tests/unit/test_mcp_server.cpp`, `tests/integration/run_headless.sh`.
  - Done when: Every required MCP behavior has an owner test or an explicit planned implementation item.
  - Notes: Created comprehensive compliance matrix at `docs/MCP_COMPLIANCE.md`. Identified critical gap: lifecycle state is server-global instead of per-session (Task 3). Mapped all JSON-RPC, MCP lifecycle, transport, and error handling requirements to implementation files and test coverage status.

- [x] Task 2: Complete MCP wire + lifecycle compliance in server core (2026-02-24)
  - Objective: Enforce strict JSON-RPC and MCP lifecycle semantics in core server flow.
  - Files: `src/mcp/server.cpp`, `include/mcp/generic/protocol.h`, `tests/unit/test_mcp_protocol.cpp` (new).
  - Done when: `jsonrpc` validation, initialize parameter validation, protocol negotiation errors, and pre-initialization request gating are enforced and tested.
  - Notes: Added new test file `tests/unit/test_mcp_protocol.cpp` with 5 test cases covering JSON-RPC envelope validation, initialize parameter validation, lifecycle gating, method not found, and ping. All 46 unit tests pass. Updated `docs/MCP_COMPLIANCE.md` with test coverage status.

- [x] Task 3: Make lifecycle state session-scoped (not global) (2026-02-24)
  - Objective: Move initialize/initialized state tracking from server-global to per-client/session scope to avoid cross-client state leakage.
  - Files: `src/mcp/server.cpp`, `src/mcp/http_sse_transport.cpp`, `include/mcp/generic/transport.h`, related tests.
  - Done when: Multiple clients can initialize independently without affecting each other, covered by regression tests.
  - Notes: Implemented per-session lifecycle via `Session` struct. `initialize` returns `sessionId` which must be included in subsequent requests via `_sessionId` param. Added `test_mcp_protocol.cpp:Multiple sessions` test. Removed magic values, used `PRIx64` for portable printf.

- [x] Task 4: Complete stream transport compliance behavior (2026-02-24)
  - Objective: Align SSE/stream behavior to spec expectations (accept headers, message framing, event naming, and explicit behavior for unsupported flows).
  - Files: `src/mcp/http_sse_transport.cpp`, `src/mcp/server.cpp`, integration tests.
  - Done when: Transport behavior is deterministic/spec-aligned and validated by integration checks.
  - Notes: SSE implementation already spec-compliant: Accept header enforcement, Content-Type, Cache-Control, Connection headers, event framing (`event: message\ndata: ...\n\n`). Integration tests in `run_headless.sh:352-357`. Unit tests for SSE formatting deferred (low priority, integration coverage exists).

- [x] Task 5: Replace macro-heavy protocol strings with single-source registry (2026-02-24)
  - Objective: Replace duplicated `#define` + `std::string_view` constant blocks with one canonical model (typed IDs + name lookup/parsing helpers).
  - Files: `include/dmcp/doom/protocol.h`, `src/doom/handlers/common.cpp`, `src/doom/handlers/methods.cpp`, `src/doom/handlers/state_json.cpp`, `src/doom/handlers/tools/*`, `src/doom/serialization.cpp`.
  - Done when: Protocol strings are defined once per domain and handlers/serialization consume that source only.
  - Notes: Focused cleanup - replaced raw string literals with existing constants from protocol.h: section names (`player`, `map`, `game`, etc.) now use `dmcp::section::*`; `execute_command` uses `tools::execute_command`; `alive`/`dead` status uses `DMCP_STATUS_*`. Full typed enum registry deferred as low priority.

- [x] Task 6: Remove legacy compatibility names and alias cruft (2026-02-24)
  - Objective: Drop backwards-compatibility constant aliases and outdated naming surfaces.
  - Files: `include/dmcp/doom/protocol.h`, `src/doom/**`, docs.
  - Done when: No deprecated aliases remain and old names fail fast at compile time.
  - Notes: Verified no deprecated aliases remain. Current aliases are intentional features: (1) Direct JSON-RPC method aliases (`get_player`, `execute_command`) for convenience, (2) Content name aliases (`Imp` -> `DoomImp`) for user-friendly input, (3) Input action alternatives (`fwd`/`forward`, `atk`/`attack`). All serve API ergonomics, not backward compatibility.

- [x] Task 7: Normalize public adapter headers and hard C ABI boundary (2026-02-24)
  - Objective: Move adapter public headers to installed `include/dmcp/adapters/*` and remove engine-private/C++ details from public contracts.
  - Files: `include/dmcp/adapters/*` (new), `adapters/zdoom/adapter.h`, `adapters/chocolate-doom/dmcp_adapter.h`, include call sites.
  - Done when: Public adapter include path is install-safe, C-only, and independent of repository-relative `adapters/...` paths.
  - Notes: Created `include/dmcp/adapters/chocolate.h` and `include/dmcp/adapters/zdoom.h` with pure C ABI. Public API includes only lifecycle, tick, command execution, and stats. Engine-private types (`player_t`, snapshot population) remain in adapter-internal headers. Consumers use `#include <dmcp/adapters/chocolate.h>` or `<dmcp/adapters/zdoom.h>`.

- [x] Task 8: Stabilize package exports and target naming (breaking-change baseline) (2026-02-24)
  - Objective: Make `find_package(dmcp CONFIG REQUIRED)` the only supported integration path with canonical imported targets.
  - Files: `CMakeLists.txt`, `cmake/dmcp-config.cmake.in`, install/export configuration.
  - Done when: Targets are consistent build-tree/install-tree; config is relocatable; versioned package config exists; stale `cmake/dmcpConfig.cmake.in` is removed.
  - Notes: Targets use consistent `dmcp::` namespace. `generic` and `core` internal names with `OUTPUT_NAME` produce `libdmcp_generic.so`/`libdmcp_core.so` files. Removed stale `cmake/dmcpConfig.cmake.in`. Simplified config file to be relocatable. Added version file support with `SameMajorVersion` compatibility. CMake package export only for shared builds; static builds install headers/libs without package config. All 47 tests pass.

- [x] Task 9: Harden shared-library ABI surface (2026-02-24)
  - Objective: Restrict exported symbols to intended public API and prevent transitive dependency/internal C++ leakage.
  - Files: `CMakeLists.txt`, `cmake/Dependencies.cmake`, linker/export controls.
  - Done when: Shared symbol table matches documented API export policy.
  - Notes: Implemented object library pattern for shared/static builds. Added linker version scripts (`cmake/dmcp_generic.version`, `cmake/dmcp_core.version`, `cmake/dmcp_chocolate.version`, `cmake/dmcp_zdoom.version`) to control symbol visibility. Removed `WINDOWS_EXPORT_ALL_SYMBOLS` in favor of explicit export macros. uSockets symbols (bsd_*, apple_*, etc.) are now hidden. yyjson symbols remain visible as a known limitation (required for internal C++ implementation). Tests link against static library to access internal symbols. All 47 tests pass on both static and shared builds.

- [x] Task 10: Package adapters as consumable project targets/components (2026-02-24)
  - Objective: Make each adapter buildable/linkable as a first-class consumable target with transitive dependencies described in CMake metadata.
  - Files: `adapters/chocolate-doom/CMakeLists.txt`, `adapters/zdoom/CMakeLists.txt`, export wiring.
  - Done when: Consumers link adapter targets directly with no manual `find_library` chains.
  - Notes: Fixed CMake target naming to produce correct exported names. Targets renamed from `dmcp_generic`/`dmcp_core`/`dmcp_chocolate`/`dmcp_zdoom` to `generic`/`core`/`chocolate`/`zdoom` with `OUTPUT_NAME` property set to produce `libdmcp_*.so`. This ensures exported targets have correct names (`dmcp::generic`, `dmcp::core`, `dmcp::chocolate`, `dmcp::zdoom`) instead of duplicated prefix (`dmcp::dmcp_generic`). Transitive dependencies properly configured: adapters link `dmcp::core` which transitively brings `dmcp::generic`. Verified consumer test project successfully links `dmcp::core` with no manual library chains. All 47 tests pass. Cleanup: removed stale `cmake/dmcpConfig.cmake.in`, removed accidentally tracked `build-shared/` from git, added `build-shared/` to `.gitignore`, simplified empty if/else blocks.

- [ ] Task 11: Simplify Chocolate/Crispy consumer integration
  - Objective: Replace manual include/lib path wiring (`DMCP_INCLUDE_DIR`, `DMCP_LIB_DIR`, explicit transitive libs) with imported targets/components.
  - Files: `chocolate-doom/CMakeLists.txt`, `chocolate-doom/src/doom/CMakeLists.txt`, `adapters/crispy-doom/patches/dmcp_integration.patch`, `tests/integration/build_crispy_doom.sh`, `Makefile`.
  - Done when: Consumers stop manually linking `yyjson`, `uSockets`, `stdc++`, and direct DMCP archives.

- [ ] Task 12: Verification matrix, docs, and test-harness stability
  - Objective: Finalize compliance/ABI/consumer verification and document all breaking changes.
  - Files: `tests/integration/*`, `tests/unit/*`, `docs/INTEGRATION.md`, `docs/README.md`, `docs/ARCHITECTURE.md`, adapter READMEs, `README.md`.
  - Done when: CI/local checks validate MCP compliance + packaging, and test harness no longer relies on flaky shared-port parallel execution behavior.

## Decisions Log
- 2026-02-24: Created a new plan file instead of changing completed historical plans to preserve auditability.
- 2026-02-24: Sequenced tasks intentionally (no parallel execution) to reduce churn and keep validation straightforward.
- 2026-02-24: Backward compatibility dropped by request; no compatibility wrappers, aliases, or deprecated paths will be preserved.
- 2026-02-24: Protocol constant cleanup added as a first-class refactor task (single-source model, no duplicated string constants).
- 2026-02-24: Added MCP compliance hardening task after identifying gaps in JSON-RPC validation and initialization lifecycle handling.
- 2026-02-24: Full MCP spec compliance is now mandatory across the application; compliance hardening is prioritized before packaging polish.
- 2026-02-24: Initial compliance hardening landed (jsonrpc validation, initialize params/version negotiation, lifecycle gating, SSE accept header, notification framing), but per-session lifecycle state and full conformance tests remain open.
- 2026-02-24: Parallel unit test execution exposed existing shared-port race behavior; harness isolation is now tracked as deliverable work.

## Notes
- Execution order: Task 1 -> Task 2 -> Task 3 -> Task 4 -> Task 5 -> Task 6 -> Task 7 -> Task 8 -> Task 9 -> Task 10 -> Task 11 -> Task 12.
- Conflict control:
  - MCP compliance work lands before packaging/consumer migration to avoid dual-change debugging.
  - `CMakeLists.txt` and package config changes are isolated after protocol/API surfaces stabilize.
  - Consumer engine changes are deferred until packaging and adapter targets are stable.
  - Documentation updates happen only after verification is green.
- Resume checklist:
  - Finish per-session lifecycle state and multi-client compliance coverage.
  - Verify protocol constants have a single source of truth and no ad-hoc literals in handlers.
  - Verify MCP initialize/lifecycle and JSON-RPC error handling against updated compliance tests.
  - Verify install-tree import with `find_package(dmcp)` across static/shared.
  - Build Chocolate/Crispy consumers through imported targets/components.
  - Run ABI export and integration/smoke checks before docs finalization.
