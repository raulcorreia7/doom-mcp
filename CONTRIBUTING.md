# Contributing to DMCP

DMCP is a C/C++ SDK exposing Doom-family game state through the Model Context Protocol. The
stable public boundary is C99; keep engine-specific code in `adapters/` and protocol logic in
`src/`/`include/`. Read `docs/ARCHITECTURE.md` for the layer rules before moving code.

## Prerequisites

- CMake and a C++ toolchain (link-time requirement even for C consumers)
- GNU make (the Makefile wraps the CMake presets in `CMakePresets.json`)
- Optional: clang-format and clang-tidy for `make format` / `make lint`
- Optional: `uv` for the Python agent helper in `examples/agents/python`

## Build and verify

```bash
make check        # configure, build, run unit tests
make validate     # run ./scripts/validate.sh
make run          # build and start the example server (curl localhost:6060/health)
```

SDK validation never launches a game. To drive CMake directly:

```bash
cmake -B build/default -DDMCP_BUILD_TESTS=ON -DDMCP_BUILD_INTEGRATION_TESTS=ON -DDMCP_BUILD_ADAPTER_FAKE=ON
cmake --build build/default --parallel
ctest --test-dir build/default -L "unit|sdk" -LE "requires_game" --output-on-failure
```

Useful targets: `make test` (unit), `make test-integration`, `make format` (clang-format over
`src include adapters`), `make lint` (clang-tidy), `make clean`. Debug builds enable sanitizers.

## Code style

- Keep the public API C99-compatible; C++ helpers stay source-level conveniences.
- Run `make format` before committing; do not reformat unrelated files.
- New tests use the deterministic `fake` adapter; never require a real game in CI.

## Pull request flow

1. Fork the repository and clone your fork.
2. Branch from `master`: `git checkout -b feature/short-name`.
3. Keep the diff scoped; update `docs/CHANGELOG.md` for user-visible changes.
4. Push and open a pull request against `master`; state which targets you ran (`make check`,
   `make validate`).
