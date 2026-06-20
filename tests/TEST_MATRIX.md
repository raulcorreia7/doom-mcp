# Test Matrix

This matrix defines SDK test scope by execution layer. DMCP tests must not launch
a game process or require runtime game data.

## Unit

- Scope: protocol, parsing, Doom context/tool groups, content catalogs, and
  layer boundary rules.
- Entry point: `make test-unit`.
- CI: `SDK build/test` across static/shared linkage on Linux, macOS, and Windows.
- Runtime: no game launch; tests that do not need live HTTP/SSE set
  `start_transport=false`.
- Sources:
  - `tests/public_c_headers.c`
  - `tests/unit/test_mcp_server.cpp`
  - `tests/unit/test_mcp_protocol.cpp`
  - `tests/unit/test_doom_context.cpp`
  - `tests/unit/test_layer_boundaries.cpp`

## Integration

- Scope: DMCP + deterministic fake adapter + MCP HTTP/SSE transport.
- Entry point: `make test-integration`.
- Sources:
  - `tests/integration/test_fake_mcp_transport.cpp`
  - `tests/support/mcp_http_client.hpp`
  - `tests/support/network.hpp`
- Runtime: no engine launch and no game data download.
- CTest labels: `integration`, `sdk`, `fake`, `transport`.

## Smoke

- Scope: fastest transport-backed “SDK is MCP-usable without a game” check.
- Entry point: `make test-smoke`.
- Runtime: no engine launch and no game data download.

## Adapter Checks

- The fake adapter is built in SDK CI because it has no engine dependency.
- `crispy-doom` and `zdoom` adapter examples are documented under
  `adapters/crispy-doom/` and `adapters/zdoom/`.
- Runtime or engine-header validation for real adapters belongs in the consuming
  engine repository or an explicit local adapter build.

## Coverage Rules

- New behavior must ship with at least one test in its owning layer.
- User-visible protocol/tool changes should include SDK integration coverage.
- Bug fixes should include a regression assertion that fails without the fix.
