# Test Matrix

This matrix defines test scope by execution layer so contributors can pick the
fastest test that still proves behavior.

## Unit

- Scope: Pure SDK logic (protocol, parsing, context/tool-group behavior).
- Speed: Fast.
- Entry point: `make test-unit`.
- CI: `SDK build/test`, across static/shared linkage on Linux, macOS, and Windows.
- Runtime: no Doom engine launch; tests that do not need live HTTP/SSE set
  `start_transport=false`.
- Sources:
  - `tests/public_c_headers.c`
  - `tests/unit/test_mcp_server.cpp`
  - `tests/unit/test_mcp_protocol.cpp`
  - `tests/unit/test_doom_context.cpp`
  - `tests/unit/test_layer_boundaries.cpp`
- C API check: `tests/public_c_headers.c` compiles the installed public headers
  as C99 without launching an engine.

## Adapter Link Checks

- Scope: SDK adapter build/link validation without launching an engine.
- Speed: Fast/medium.
- CI: `Adapter build/link checks`.
- Coverage:
  - Fake adapter builds and links with unit tests.
  - Crispy Doom adapter builds against generated Crispy headers without WADs.
  - ZDoom adapter verifies its SDK-header configuration guard.
- Out of scope: engine startup, WAD download, and real-engine HTTP/e2e client behavior.

## Integration

- Scope: DMCP + deterministic fake adapter + MCP HTTP/SSE transport.
- Speed: Medium, but still game-free.
- Entry point: `make test-integration`.
- Sources:
  - `tests/integration/test_fake_mcp_transport.cpp`
  - `tests/support/mcp_http_client.hpp`
  - `tests/support/network.hpp`
- Runtime: no Doom engine launch; no WAD/IWAD download.
- CTest labels: `integration`, `no_game`, `fake`, `transport`.

## E2E

- Scope: Client-facing MCP behavior via HTTP against a live game engine.
- Speed: Medium/slow.
- Entry point: `make test-e2e`.
- Sources:
  - Optional engine e2e targets/scripts such as `tests/integration/run_headless.sh`.
- Runtime: may require Crispy Doom and an IWAD. This is not the default CI/test
  path.

## Smoke

- Scope: Minimal "is the SDK MCP-usable without a game" verification.
- Speed: Fastest transport-backed check.
- Entry point: `make test-smoke`.
- Sources:
  - Fake-adapter MCP transport test path.
- Runtime: no Doom engine launch; no WAD/IWAD download.

## Coverage Rules

- New behavior must ship with at least one test in its owning layer.
- User-visible protocol/tool changes should include no-game integration coverage;
  add optional engine e2e only when the behavior depends on real engine wiring.
- Bug fixes should include a regression assertion that fails without the fix.
