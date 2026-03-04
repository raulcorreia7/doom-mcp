# AGENTS.md

Repository-specific guidance for autonomous coding agents working in `doom-mcp`.

## Project Snapshot

- Name: `DMCP` (Doom Model Context Protocol SDK), version `0.6.0`
- Languages: `C11`, `C++17`, plus Python for e2e tests
- Build system: `CMake` with a convenience `Makefile`
- Core architecture: `Engine -> Adapter -> Doom MCP -> Generic MCP`

## Source Map

- Public Generic MCP API: `include/mcp/generic/`
- Public Doom MCP API: `include/dmcp/doom/`
- Generic MCP implementation: `src/mcp/`
- Doom-specific implementation: `src/doom/`
- Engine adapters: `adapters/zdoom/`, `adapters/crispy-doom/`
- Unit tests (Catch2): `tests/unit/`
- Integration scripts: `tests/integration/`
- Python e2e tests: `tests/e2e/`
- Design docs: `docs/ARCHITECTURE.md`, `docs/README.md`

## Canonical Commands

- Build: `make dmcp`
- Debug + sanitizers: `make debug`
- Unit tests: `make test`
- Build + tests: `make check`
- Validation matrix: `make validate`
- Run example server: `make run`
- Format C/C++ sources: `make format`
- Headless integration flow: `make headless`

Always use all CPUs for compilation (e.g., `cmake --build <dir> --parallel`, `make -j"$(nproc)"`).

Equivalent direct CMake flow:

```bash
cmake -B build/default -DDMCP_BUILD_TESTS=ON
cmake --build build/default --parallel
ctest --test-dir build/default --output-on-failure
```

## Adapter and E2E Workflow

- `tests/integration/run_headless.sh` expects `crispy-doom/build/src/crispy-doom` and `assets/wads/doom1.wad`.
- Download shareware WAD with `tests/integration/download_wad.sh` (or `make download-wad`).
- Build Crispy Doom with DMCP enabled from repo root:

```bash
cmake -B build/default -DDMCP_BUILD_TESTS=ON
cmake --build build/default --parallel
cmake -S crispy-doom -B crispy-doom/build \
  -DDMCP_ENABLE=ON \
  -DDMCP_ROOT="$PWD"
cmake --build crispy-doom/build --parallel
```

- Python e2e suite requires `pytest` and `requests`; run with `pytest -q tests/e2e`.

## Editing Guardrails

- Keep public APIs C-compatible (`include/*` headers).
- Preserve layer boundaries: Generic MCP must stay game-agnostic; Doom logic stays in `src/doom`; engine specifics stay in adapters.
- Follow existing type-oriented naming patterns (`mcp_*`, `dmcp_*`).
- Prefer minimal targeted diffs; avoid broad refactors in `crispy-doom/` unless the task explicitly requires engine hook changes.
- Do not commit generated/runtime artifacts such as `build/`, `tests/e2e/doom.log`, `tests/e2e/__pycache__/`, or downloaded WAD files.
