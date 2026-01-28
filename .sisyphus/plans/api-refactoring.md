# Work Plan: Doom-MCP Public API Refactoring

## Executive Summary

This work plan addresses critical API inconsistencies and architectural improvements needed to make doom-mcp "clean, modular, organized, intuitive, and easy to integrate." Based on comprehensive architectural analysis, the codebase has solid foundational architecture (B+ grade) but suffers from API naming inconsistencies, missing error context, and incomplete implementations.

**Current Grade**: B+ (Good foundation, needs refinement)  
**Target Grade**: A (Production-ready, intuitive APIs)

**Estimated Effort**: Medium (3-4 days of focused work)  
**Parallel Execution**: YES - Tasks grouped into 5 waves  
**Risk Level**: LOW (additive changes, backward compatibility maintained)

---

## Context

### Current Architecture (Strengths)
- **4-layer design**: Engine → Adapter → Doom MCP → Generic MCP
- **Clean dependency flow**: No back-references, strict layering
- **Good separation of concerns**: Each layer has single responsibility
- **C API at boundaries**: All public APIs are C-compatible
- **Opaque handle pattern**: Perfect information hiding

### Critical Issues Identified

1. **API Naming Inconsistencies** (HIGH PRIORITY)
   - `mcp_server_create()` vs `dmcp_create()` (missing "context" in name)
   - Inconsistent naming breaks mental model

2. **Missing Error Context** (HIGH PRIORITY)
   - Only error codes returned, no error messages
   - Hard to debug failures

3. **Configuration Pattern Inconsistencies** (MEDIUM PRIORITY)
   - ZDoom adapter uses nested config pointer while others are flat
   - Creates confusion for users

4. **Limited Method Registration** (MEDIUM PRIORITY)
   - No batch registration (inefficient for many methods)
   - No pre-parsed JSON handler option

5. **No Runtime Transport Selection** (MEDIUM PRIORITY)
   - Transport selected at compile time only
   - No way to switch transports without rebuild

6. **Code Quality Issues** (MEDIUM PRIORITY)
   - 15+ instances of duplicated string copy patterns
   - Magic numbers throughout (buffer sizes, ports, limits)
   - No unit tests (critical gap)

7. **Incomplete Features** (MEDIUM PRIORITY)
   - Screenshot endpoint not implemented (TODO)
   - PNG encoding not implemented (TODO)
   - Command acknowledgment not implemented (TODO)

---

## Work Objectives

### Core Objective
Standardize and enhance all public APIs to be consistent, intuitive, well-documented, and production-ready while maintaining backward compatibility.

### Concrete Deliverables
1. **Standardized API naming** across all layers (`{namespace}_{type}_{action}`)
2. **Enhanced error handling** with context messages
3. **Unified configuration patterns** with clear documentation
4. **Batch method registration** support
5. **Runtime transport registry** for pluggable transports
6. **Constants header** eliminating magic numbers
7. **Unit test framework** with core functionality tests
8. **Complete TODO items** (screenshot endpoint, PNG encoding)
9. **Comprehensive documentation** for all public APIs
10. **Updated examples** showing best practices

### Definition of Done
- [ ] All public APIs follow consistent naming convention
- [ ] Error messages provide actionable context
- [ ] Configuration patterns are documented and consistent
- [ ] Batch registration works for 10+ methods efficiently
- [ ] Runtime transport selection works (stdio vs HTTP)
- [ ] Zero magic numbers in code (all defined as constants)
- [ ] Unit tests achieve >70% coverage of public APIs
- [ ] All TODO items resolved or documented as known limitations
- [ ] All public APIs have JSDoc-style documentation
- [ ] Examples compile and pass integration tests

### Must Have
- Backward compatibility (old APIs deprecated but functional)
- No breaking changes to existing integrations
- All changes compile without warnings
- Tests pass on all supported platforms

### Must NOT Have (Guardrails)
- No changes to internal implementation logic (focus on APIs only)
- No modifications to game state serialization (separate concern)
- No removal of existing functionality (only additions)
- No changes to CMake build system structure

---

## Verification Strategy

### Test Infrastructure Assessment
- **Current**: No unit tests (tests/ directory empty)
- **Framework to add**: Catch2 (header-only, C++ friendly)
- **Test approach**: TDD for new APIs, regression tests for existing

### TDD Workflow
Each TODO follows RED-GREEN-REFACTOR:

1. **RED**: Write failing test first
   - Test file: `tests/test_{feature}.cpp`
   - Test command: `cmake --build build && ctest -R {test_name}`
   - Expected: FAIL (test exists, implementation doesn't)

2. **GREEN**: Implement minimum code to pass
   - Command: `cmake --build build && ctest -R {test_name}`
   - Expected: PASS

3. **REFACTOR**: Clean up while keeping green
   - Command: `cmake --build build && ctest -R {test_name}`
   - Expected: PASS (still)

### Test Setup Task
- [ ] 0. Setup Test Infrastructure
  - Install: Add Catch2 via CPM in CMakeLists.txt
  - Config: Create `tests/CMakeLists.txt`
  - Verify: `cmake --build build && ctest` shows test runner
  - Example: Create `tests/test_example.cpp` with simple test
  - Verify: Test passes

### Manual Execution Verification

**For API changes:**
- [ ] Using C++ test files:
  - Compile: `cmake --build build`
  - Run: `./build/tests/dmcp_tests`
  - Verify: All tests pass (N tests, 0 failures)

**For examples:**
- [ ] Using shell commands:
  - Build: `cmake --build build`
  - Run: `./build/examples/dummy_server &`
  - Test: `curl http://localhost:6060/health`
  - Verify: Returns `{"status":"ok"}`
  - Kill: `pkill dummy_server`

**For integration:**
- [ ] Using test script:
  - Run: `./test_mcp.sh`
  - Verify: All checks pass

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Foundation):
├── Task 1: Setup test infrastructure
├── Task 2: Create constants header
└── Task 3: Standardize API naming

Wave 2 (Core Improvements):
├── Task 4: Enhanced error handling
├── Task 5: Unified configuration
└── Task 6: Batch method registration

Wave 3 (Advanced Features):
├── Task 7: Transport registry
└── Task 8: Complete TODO items

Wave 4 (Quality):
├── Task 9: Comprehensive documentation
└── Task 10: Update examples

Wave 5 (Final):
└── Task 11: Integration testing and cleanup

Critical Path: Task 1 → Task 2 → Task 3 → Task 4 → Task 11
Parallel Speedup: ~50% faster than sequential
```

### Dependency Matrix

| Task | Depends On | Blocks | Can Parallelize With |
|------|------------|--------|---------------------|
| 1 (Tests) | None | 2, 3, 4, 5, 6 | None |
| 2 (Constants) | None | 3, 4, 6, 8 | 1 |
| 3 (Naming) | 1, 2 | 4, 11 | None |
| 4 (Errors) | 1, 2, 3 | 11 | 5, 6 |
| 5 (Config) | 1, 2 | 11 | 4, 6 |
| 6 (Batch) | 1, 2, 3 | 11 | 4, 5 |
| 7 (Transport) | 1, 2 | 11 | 4, 5, 6 |
| 8 (TODOs) | 2 | 11 | 4, 5, 6, 7 |
| 9 (Docs) | 3, 4, 5 | 11 | 6, 7, 8 |
| 10 (Examples) | 3, 4, 5, 6 | 11 | 7, 8, 9 |
| 11 (Final) | 3-10 | None | None |

---

## TODOs

### Wave 1: Foundation

- [ ] 1. Setup Test Infrastructure

  **What to do**:
  - Add Catch2 testing framework via CPM
  - Create tests/CMakeLists.txt
  - Write first test to verify setup
  - Integrate with CI if present

  **Must NOT do**:
  - Don't write tests for existing code yet (that comes later)
  - Don't modify source code

  **Recommended Agent Profile**:
  - **Category**: `unspecified-low`
  - **Skills**: None required (straightforward CMake work)

  **Parallelization**:
  - **Can Run In Parallel**: NO (blocks other tasks)
  - **Parallel Group**: Wave 1 (solo)
  - **Blocks**: Tasks 2-11
  - **Blocked By**: None

  **References**:
  - `CMakeLists.txt` - Add Catch2 dependency
  - `tests/` - Create test directory structure
  - Example: Any CMake project with Catch2

  **Acceptance Criteria**:
  - [ ] `cmake --build build` compiles tests
  - [ ] `ctest` runs and shows test results
  - [ ] At least one example test passes
  - [ ] CI passes (if applicable)

  **Commit**: YES
  - Message: `test: Add Catch2 testing framework`
  - Files: `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/test_main.cpp`

- [ ] 2. Create Constants Header

  **What to do**:
  - Create `include/mcp/generic/constants.h`
  - Define all magic numbers as constants:
    - Buffer sizes (8192, 16384, 256, etc.)
    - Default port (6060)
    - Array limits (DMCP_MAX_ENEMIES, DMCP_MAX_INVENTORY)
    - Protocol strings ("/mcp", "/sse", "/health")
    - JSON-RPC version ("2.0")
  - Update all source files to use constants

  **Must NOT do**:
  - Don't change any logic, only replace literals with constants
  - Don't modify public API signatures

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: None required (mechanical refactoring)

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Task 1)
  - **Parallel Group**: Wave 1
  - **Blocks**: Tasks 3, 4, 6, 8
  - **Blocked By**: None

  **References**:
  - `src/mcp/server.cpp` - Lines with 8192, 16384
  - `src/mcp/sse_transport.cpp` - Lines with 6060, 8192, 256
  - `include/dmcp/doom/types.h` - Lines with 256, 64, 128

  **Acceptance Criteria**:
  - [ ] Constants header created with all magic numbers
  - [ ] All source files updated to use constants
  - [ ] No magic numbers remain (verify with grep)
  - [ ] Build passes without warnings

  **Commit**: YES
  - Message: `refactor: Extract magic numbers to constants.h`
  - Files: `include/mcp/generic/constants.h`, `src/**/*.cpp`, `include/**/*.h`

- [ ] 3. Standardize API Naming

  **What to do**:
  - Add new functions with consistent naming:
    - `dmcp_context_create()` (wraps `dmcp_create()`)
    - `dmcp_context_destroy()` (wraps `dmcp_destroy()`)
    - `dmcp_context_tick()` (wraps `dmcp_tick()`)
    - `dmcp_context_is_running()` (wraps `dmcp_is_running()`)
  - Mark old functions as deprecated with comments
  - Update internal usage to new names
  - Ensure backward compatibility (old names still work)

  **Must NOT do**:
  - Don't remove old functions (breaks compatibility)
  - Don't change function signatures (only names)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-low`
  - **Skills**: None required

  **Parallelization**:
  - **Can Run In Parallel**: NO (must wait for Tasks 1, 2)
  - **Parallel Group**: Wave 1 (sequential)
  - **Blocks**: Tasks 4, 9, 10, 11
  - **Blocked By**: Tasks 1, 2

  **References**:
  - `include/dmcp/doom/api.h` - Current API
  - Pattern: `mcp_server_*` naming in `include/mcp/generic/server.h`

  **Acceptance Criteria**:
  - [ ] New functions added with `_context_` infix
  - [ ] Old functions marked deprecated but functional
  - [ ] Internal code uses new names
  - [ ] Tests verify both old and new names work
  - [ ] Examples updated to use new names

  **Commit**: YES
  - Message: `feat(api): Add consistent dmcp_context_* naming`
  - Files: `include/dmcp/doom/api.h`, `src/doom/context.cpp`, `examples/*.cpp`

### Wave 2: Core Improvements

- [ ] 4. Enhanced Error Handling

  **What to do**:
  - Add error message storage to context/server structures
  - Implement `mcp_get_last_error()` function
  - Implement `dmcp_get_last_error()` function
  - Update error sites to set meaningful messages
  - Add thread-local storage for error messages

  **Must NOT do**:
  - Don't change existing error code values
  - Don't break existing error handling code

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high` (thread safety concerns)
  - **Skills**: None required (but careful with thread-safety)

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 5, 6)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 11
  - **Blocked By**: Tasks 1, 2, 3

  **References**:
  - `include/mcp/generic/protocol.h` - Result codes
  - `src/mcp/server.cpp` - Error sites (lines with error returns)
  - `src/doom/context.cpp` - Error sites

  **Acceptance Criteria**:
  - [ ] Error message getter functions implemented
  - [ ] All error sites set descriptive messages
  - [ ] Thread-safe implementation (use thread-local)
  - [ ] Tests verify error messages are set correctly

  **Commit**: YES
  - Message: `feat(api): Add error message context to result codes`
  - Files: `include/mcp/generic/protocol.h`, `src/mcp/server.cpp`, `include/dmcp/doom/api.h`, `src/doom/context.cpp`

- [ ] 5. Unified Configuration

  **What to do**:
  - Document ZDoom adapter's nested config pattern
  - Add helper macro or function to flatten config
  - Ensure all configs use `struct_size` pattern consistently
  - Add validation functions for configs
  - Create config builder pattern (optional but nice)

  **Must NOT do**:
  - Don't change existing config structures (breaking change)
  - Don't remove nested pointer in ZDoom config

  **Recommended Agent Profile**:
  - **Category**: `unspecified-low`
  - **Skills**: None required

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 4, 6)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 11
  - **Blocked By**: Tasks 1, 2

  **References**:
  - `include/dmcp/doom/types.h` - dmcp_config_t
  - `adapters/zdoom/adapter.h` - dmcp_zdoom_config_t
  - `include/mcp/generic/protocol.h` - mcp_server_config_t

  **Acceptance Criteria**:
  - [ ] Config validation functions added
  - [ ] Documentation explains nested vs flat patterns
  - [ ] Helper for merging configs (if needed)
  - [ ] Tests validate all config patterns work

  **Commit**: YES
  - Message: `feat(api): Add configuration validation and documentation`
  - Files: `include/dmcp/doom/types.h`, `adapters/zdoom/adapter.h`, `include/mcp/generic/protocol.h`

- [ ] 6. Batch Method Registration

  **What to do**:
  - Add `mcp_method_registration_t` struct
  - Add `mcp_server_register_methods()` function
  - Implement batch registration (single lock for all)
  - Add convenience handler with pre-parsed JSON (optional)
  - Optimize internal registration (reserve hash map space)

  **Must NOT do**:
  - Don't change existing single registration API
  - Don't break existing method handlers

  **Recommended Agent Profile**:
  - **Category**: `unspecified-low`
  - **Skills**: None required

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 4, 5)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 11
  - **Blocked By**: Tasks 1, 2, 3

  **References**:
  - `include/mcp/generic/server.h` - Current registration API
  - `src/mcp/server.cpp` - Registration implementation

  **Acceptance Criteria**:
  - [ ] Batch registration function implemented
  - [ ] Performance test shows improvement for 10+ methods
  - [ ] Tests verify batch registration works
  - [ ] Existing single registration still works

  **Commit**: YES
  - Message: `feat(api): Add batch method registration support`
  - Files: `include/mcp/generic/server.h`, `src/mcp/server.cpp`

### Wave 3: Advanced Features

- [ ] 7. Transport Registry

  **What to do**:
  - Add transport registry structure
  - Implement `mcp_register_transport()` function
  - Implement `mcp_get_transport()` function
  - Add transport name to server config
  - Refactor server creation to use registry
  - Keep SSE as default transport

  **Must NOT do**:
  - Don't remove existing transport abstraction
  - Don't break existing SSE transport usage

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high` (architectural change)
  - **Skills**: None required (but careful with design)

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 4, 5, 6)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 11
  - **Blocked By**: Tasks 1, 2

  **References**:
  - `include/mcp/generic/transport.h` - Transport interface
  - `src/mcp/sse_transport.cpp` - SSE implementation
  - `src/mcp/server.cpp` - Server creation

  **Acceptance Criteria**:
  - [ ] Transport registry implemented
  - [ ] Multiple transports can be registered
  - [ ] Server can select transport by name
  - [ ] Default SSE transport works as before
  - [ ] Tests verify transport selection

  **Commit**: YES
  - Message: `feat(api): Add runtime transport registry`
  - Files: `include/mcp/generic/transport.h`, `src/mcp/server.cpp`, `include/mcp/generic/server.h`

- [ ] 8. Complete TODO Items

  **What to do**:
  - Implement screenshot endpoint in SSE transport
  - Implement PNG encoding for screenshots
  - Implement command acknowledgment (if feasible)
  - Add tests for new functionality
  - Document any limitations

  **Must NOT do**:
  - Don't spend excessive time on complex features
  - Document if feature is too complex for current scope

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high` (complex implementations)
  - **Skills**: None required (but may need image encoding library)

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 4, 5, 6, 7)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 11
  - **Blocked By**: Task 2 (constants needed)

  **References**:
  - `src/mcp/sse_transport.cpp:151` - Screenshot endpoint TODO
  - `src/doom/context.cpp:485` - PNG encoding TODO
  - `adapters/zdoom/commands.cpp:229` - Command acknowledgment TODO

  **Acceptance Criteria**:
  - [ ] Screenshot endpoint serves PNG images
  - [ ] PNG encoding works (use stb_image_write or similar)
  - [ ] Command acknowledgment implemented (or documented why not)
  - [ ] Tests verify functionality

  **Commit**: YES (may be multiple commits)
  - Messages:
    - `feat(transport): Implement screenshot endpoint`
    - `feat(doom): Add PNG encoding for screenshots`
    - `feat(commands): Add command acknowledgment`
  - Files: `src/mcp/sse_transport.cpp`, `src/doom/context.cpp`, `adapters/zdoom/commands.cpp`

### Wave 4: Quality

- [ ] 9. Comprehensive Documentation

  **What to do**:
  - Add JSDoc-style comments to all public APIs
  - Document parameters, return values, error conditions
  - Add usage examples in comments
  - Create API reference document
  - Update README with new APIs

  **Must NOT do**:
  - Don't modify implementation logic
  - Don't remove existing documentation

  **Recommended Agent Profile**:
  - **Category**: `writing`
  - **Skills**: None required

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 6, 7, 8)
  - **Parallel Group**: Wave 4
  - **Blocks**: Task 11
  - **Blocked By**: Tasks 3, 4, 5

  **References**:
  - All header files in `include/`
  - Pattern: Look at well-documented C libraries

  **Acceptance Criteria**:
  - [ ] All public functions have documentation comments
  - [ ] API reference document generated (if using Doxygen)
  - [ ] README updated with new features
  - [ ] Examples documented

  **Commit**: YES
  - Message: `docs: Add comprehensive API documentation`
  - Files: `include/**/*.h`, `README.md`

- [ ] 10. Update Examples

  **What to do**:
  - Update dummy_server.cpp to use new APIs
  - Create minimal_server.c example
  - Create custom_transport.c example (if transport registry done)
  - Add comments explaining best practices
  - Ensure all examples compile and run

  **Must NOT do**:
  - Don't remove existing dummy_server.cpp
  - Don't make examples too complex

  **Recommended Agent Profile**:
  - **Category**: `unspecified-low`
  - **Skills**: None required

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 7, 8, 9)
  - **Parallel Group**: Wave 4
  - **Blocks**: Task 11
  - **Blocked By**: Tasks 3, 4, 5, 6

  **References**:
  - `examples/dummy_server.cpp` - Current example
  - New examples should follow same structure

  **Acceptance Criteria**:
  - [ ] dummy_server.cpp updated to use new APIs
  - [ ] minimal_server.c created (pure C example)
  - [ ] All examples compile without warnings
  - [ ] All examples run successfully (test manually)

  **Commit**: YES
  - Message: `examples: Update to use new APIs and add minimal example`
  - Files: `examples/dummy_server.cpp`, `examples/minimal_server.c`

### Wave 5: Final

- [ ] 11. Integration Testing and Cleanup

  **What to do**:
  - Run full test suite
  - Run integration test script (test_mcp.sh)
  - Verify all examples work
  - Check for memory leaks (Valgrind/ASan if available)
  - Review all changes for consistency
  - Remove any temporary files
  - Update CHANGELOG if present

  **Must NOT do**:
  - Don't skip any tests
  - Don't leave temporary files

  **Recommended Agent Profile**:
  - **Category**: `code-review` (self-review)
  - **Skills**: None required

  **Parallelization**:
  - **Can Run In Parallel**: NO (final verification)
  - **Parallel Group**: Wave 5 (solo)
  - **Blocks**: None
  - **Blocked By**: Tasks 3-10

  **References**:
  - `test_mcp.sh` - Integration test script
  - All source files changed in previous tasks

  **Acceptance Criteria**:
  - [ ] All unit tests pass
  - [ ] Integration tests pass
  - [ ] Examples run successfully
  - [ ] No memory leaks (if checked)
  - [ ] Code review completed
  - [ ] CHANGELOG updated

  **Commit**: YES (if any cleanup needed)
  - Message: `chore: Final cleanup and integration verification`
  - Files: Any remaining cleanup

---

## Commit Strategy

| After Task | Message | Files | Verification |
|------------|---------|-------|--------------|
| 1 | `test: Add Catch2 testing framework` | CMakeLists.txt, tests/ | ctest passes |
| 2 | `refactor: Extract magic numbers to constants.h` | constants.h, src/**/*.cpp | grep shows no magic numbers |
| 3 | `feat(api): Add consistent dmcp_context_* naming` | api.h, context.cpp, examples/ | Tests pass |
| 4 | `feat(api): Add error message context` | protocol.h, server.cpp, api.h, context.cpp | Tests pass |
| 5 | `feat(api): Add configuration validation` | types.h, adapter.h, protocol.h | Tests pass |
| 6 | `feat(api): Add batch method registration` | server.h, server.cpp | Tests pass |
| 7 | `feat(api): Add runtime transport registry` | transport.h, server.cpp, server.h | Tests pass |
| 8 | `feat(*): Complete TODO items` | sse_transport.cpp, context.cpp, commands.cpp | Tests pass |
| 9 | `docs: Add comprehensive API documentation` | include/**/*.h, README.md | Documentation review |
| 10 | `examples: Update examples` | examples/ | Examples compile and run |
| 11 | `chore: Final cleanup` | Various | All tests pass |

---

## Success Criteria

### Verification Commands
```bash
# Build everything
cmake -B build -S .
cmake --build build

# Run unit tests
ctest --test-dir build

# Run integration tests
./test_mcp.sh

# Check for magic numbers (should return nothing)
grep -rn "8192\|16384\|6060" src/ include/ || echo "No magic numbers found"

# Run example
./build/examples/dummy_server &
sleep 1
curl http://localhost:6060/health
pkill dummy_server
```

### Expected Outputs
- Build: No errors or warnings
- Unit tests: 100% pass rate (at least 20 tests)
- Integration tests: All checks pass
- Magic number check: "No magic numbers found"
- Example: Returns `{"status":"ok"}`

### Final Checklist
- [ ] All public APIs follow `{namespace}_{type}_{action}` naming
- [ ] Error messages provide actionable context
- [ ] Configuration patterns documented and consistent
- [ ] Batch registration works efficiently
- [ ] Runtime transport selection works
- [ ] Zero magic numbers in code
- [ ] Unit tests achieve >70% coverage
- [ ] All TODO items resolved or documented
- [ ] All public APIs documented
- [ ] Examples compile and run
- [ ] Integration tests pass
- [ ] Backward compatibility maintained

---

## Risks and Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Breaking changes | High | Low | Maintain old APIs as deprecated wrappers |
| Build failures | Medium | Low | Test after each commit |
| Test flakiness | Medium | Medium | Use deterministic tests, avoid timing-dependent tests |
| Scope creep | Medium | High | Stick to task list, document future work |
| Memory leaks in new code | High | Low | Use Valgrind/ASan, review all allocations |
| Platform compatibility | Medium | Low | Test on Linux, macOS if possible |

---

## Future Work (Out of Scope)

These items are NOT part of this work plan but should be considered for future iterations:

1. **WebSocket transport** - Add WebSocket as alternative to SSE
2. **Resource implementation** - Add static MCP resources (not just SSE events)
3. **Prompts** - Add MCP prompts support
4. **Authentication** - Add API key or token-based auth
5. **Rate limiting per client** - Currently global only
6. **Compression** - Add gzip for SSE streams
7. **Metrics endpoint** - Add Prometheus-compatible metrics
8. **Plugin system** - Dynamic loading of tool modules
9. **C++ wrapper** - Modern C++ API (RAII, exceptions)
10. **Python bindings** - Expose to Python for AI/ML workflows

---

*Plan generated by Prometheus on 2026-01-28*  
*Based on comprehensive architectural analysis of doom-mcp codebase*
