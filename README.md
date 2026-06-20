# DMCP - Doom Model Context Protocol SDK

DMCP is a C/C++ SDK for exposing Doom-family game state and commands through the
Model Context Protocol (MCP). It keeps engine-specific hooks separate from the
agent-facing protocol:

```text
Engine <-> Engine Adapter <-> Doom MCP <-> Generic MCP <-> Core API/runtime
```

The stable public boundary is C99-compatible. C++ helpers are source-level
conveniences over the C API.

## What It Provides

- State tools for player, enemies, entities, items, map, inventory, game info,
  available content, and opt-in screenshots.
- Command tools for spawning, giving items, changing levels, moving the player,
  player input, and batched commands.
- HTTP/SSE MCP transport with JSON-RPC lifecycle handling.
- SDK tests using a deterministic fake adapter.
- Adapter source for `crispy-doom`, `zdoom`, and the test-only `fake` adapter.

## Quick Verify

```bash
make check
make validate
```

Run the SDK example server:

```bash
make run
curl http://localhost:6060/health
```

Use the optional Python helper when a DMCP-enabled engine is already running:

```bash
uv run --project examples/agents/python dmcp-agent tools --compact
uv run --project examples/agents/python dmcp-agent --pretty brief
uv run --project examples/agents/python dmcp-agent --pretty shell
```

The helper uses `uv` and the official MCP Python SDK. It is the preferred
interface for CLI agents because it wraps verbose MCP schemas in compact,
structured commands. See [examples/agents/python/README.md](examples/agents/python/README.md).

SDK validation never launches a game:

```bash
cmake -B build/default \
  -DDMCP_BUILD_TESTS=ON \
  -DDMCP_BUILD_INTEGRATION_TESTS=ON \
  -DDMCP_BUILD_ADAPTER_FAKE=ON
cmake --build build/default --parallel
ctest --test-dir build/default -L "unit|sdk" -LE "requires_game" --output-on-failure
```

## Generic Engine Hook Shape

Engine adapters should keep the engine-side hook surface small:

```c
void D_DoomMain(void) {
  dmcp_engine_config_t config = DMCP_ParseArgs(myargc, myargv);
  DMCP_Init(config);
}

void G_Ticker(void) {
  DMCP_Tick();
}

void D_Display(void) {
  DMCP_CaptureFrame();
}

void I_Quit(void) {
  DMCP_Shutdown();
}
```

The `crispy-doom` and `zdoom` adapters expose this `DMCP_*` hook shape through
adapter-local `engine_hooks.h` headers.

## Adapters

Adapter-specific examples and build requirements live next to the adapter source:

| Adapter | Docs | Purpose |
|---------|------|---------|
| `crispy-doom` | [adapters/crispy-doom/README.md](adapters/crispy-doom/README.md) | C hook layer and minimal engine integration |
| `zdoom` | [adapters/zdoom/README.md](adapters/zdoom/README.md) | C++ adapter lifecycle and command bridge |
| `fake` | [adapters/README.md](adapters/README.md) | Deterministic SDK tests |

Consuming engine repositories own game builds, release packaging, and runtime
assets. DMCP CI builds and packages the SDK only.

Static SDK archives expose the same C99 API, but the implementation is C++ and
therefore requires a C++ toolchain at link time. The packaged CMake target
declares that requirement for consumers.

## Documentation

| Document | Purpose |
|----------|---------|
| [docs/QUICKSTART.md](docs/QUICKSTART.md) | Build and validate the SDK |
| [docs/README.md](docs/README.md) | API reference |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Layer boundaries |
| [docs/INTEGRATION.md](docs/INTEGRATION.md) | MCP client setup |
| [docs/USAGE.md](docs/USAGE.md) | Tool usage patterns |
| [docs/FEATURES.md](docs/FEATURES.md) | Feature overview |
| [docs/MCP_COMPLIANCE.md](docs/MCP_COMPLIANCE.md) | Protocol compliance |
| [docs/CHANGELOG.md](docs/CHANGELOG.md) | Version history |
