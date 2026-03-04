# Test Matrix

This matrix defines test scope by execution layer so contributors can pick the
fastest test that still proves behavior.

## Unit

- Scope: Pure SDK logic (protocol, parsing, context/layer behavior).
- Speed: Fast.
- Entry point: `make test-unit`.
- Sources:
  - `tests/unit/test_mcp_server.cpp`
  - `tests/unit/test_mcp_protocol.cpp`
  - `tests/unit/test_doom_context.cpp`
  - `tests/unit/test_layers.cpp`
  - `tests/unit/test_layer_boundaries.cpp`

## Integration

- Scope: DMCP + real Crispy Doom process wiring.
- Speed: Medium (requires engine startup).
- Entry point: `make test-integration`.
- Sources:
  - `tests/integration/run_headless.sh`
  - CTest: `Integration: Headless Crispy`

## E2E

- Scope: Client-facing MCP behavior via HTTP against live game.
- Speed: Medium/slow.
- Entry point: `make test-e2e`.
- Sources:
  - `tests/e2e/test_*.py`
- Fast defaults:
  - `DMCP_E2E_FAST=1` keeps startup/request timeouts short (default).
  - `DMCP_STARTUP_INITIAL_DELAY` controls first probe delay before health polling.

## Smoke

- Scope: Minimal "is it alive and MCP-usable" verification.
- Speed: Fastest engine-backed check.
- Entry point: `make test-smoke`.
- Sources:
  - CTest: `Integration: Headless Crispy` (single smoke flow)

## Coverage Rules

- New behavior must ship with at least one test in its owning layer.
- User-visible protocol/tool changes should include integration or e2e coverage.
- Bug fixes should include a regression assertion that fails without the fix.
