# DMCP Quick Start Guide

Get Doom MCP running in 5 minutes.

## What is DMCP?

DMCP (Doom Model Context Protocol SDK) is a C/C++ library that exposes Doom game state to AI agents via the Model Context Protocol (MCP). It lets AI assistants like Codex, Claude, or OpenCode:

- Read player health, position, inventory
- Query enemy and entity positions
- Execute commands (spawn enemies, change levels, give items)
- Control player movement tick-by-tick
- Capture screenshots

## Why DMCP?

| Use Case | Benefit |
|----------|---------|
| AI game playing | Agents can read state and send inputs |
| Automated testing | Headless game control and verification |
| Research | Low-latency state access for ML training |
| Streaming | Real-time game state via HTTP/SSE |

## Layers

DMCP keeps the agent-facing protocol separate from engine hooks:

```text
Crispy/Engine -> Adapter -> Doom MCP -> Generic MCP -> Core API/runtime
```

The public integration boundary is a C99-compatible API. C++ consumers can use
source-level protocol-name wrappers from `include/dmcp/doom/protocol.h`.

## Prerequisites

- CMake 3.25+
- C99 compiler for public C headers and adapter glue
- C++17 compiler for the SDK implementation
- SDL2 (for engine adapters)
- `jq` (for the quick MCP session verification command)

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  pkg-config \
  git \
  curl \
  jq \
  libsdl2-dev \
  libpng-dev \
  libsamplerate0-dev
```

macOS with Homebrew:

```bash
brew install cmake ninja pkg-config jq sdl2 libpng libsamplerate
```

Windows:

Use Visual Studio Build Tools 2022 or newer plus CMake. For adapter
dependencies, use the repository CMake presets with vcpkg or install SDL2,
libpng, and libsamplerate through your package manager.

## Quick Install

```bash
# Clone
git clone https://github.com/anomalyco/doom-mcp.git
cd doom-mcp

# Build + test
make check

# Run example server
make run
```

For CI and local validation that must not launch a game process, use no-game
validation:

```bash
cmake -B build/default \
  -DDMCP_BUILD_TESTS=ON \
  -DDMCP_BUILD_INTEGRATION_TESTS=ON \
  -DDMCP_BUILD_ADAPTER_FAKE=ON
cmake --build build/default --parallel
ctest --test-dir build/default -L "unit|no_game" -LE "requires_game|headless|e2e" --output-on-failure
```

Default validation uses C/C++ unit tests plus the fake-adapter MCP transport
integration path. Real-engine headless checks are optional e2e checks, not the
default path, because they can require Crispy Doom and an IWAD.

## Verify Installation

```bash
# Health check
curl http://localhost:6060/health
# {"status": "ok", "clients": 0}

# Initialize MCP session
INIT=$(curl -s -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"quickstart","version":"1.0"}}}')

SESSION_ID=$(printf '%s' "$INIT" | jq -r '.result.sessionId')
curl -s -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{"jsonrpc":"2.0","method":"notifications/initialized","params":{}}' >/dev/null

# List MCP tools (strict lifecycle/session mode)
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: $SESSION_ID" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
```

## Next Steps

- [Usage Examples](USAGE.md) - Common patterns and workflows
- [Integration Guide](INTEGRATION.md) - Connect to Codex, OpenCode, Claude, etc.
- [API Reference](README.md) - Full API documentation
- [Architecture](ARCHITECTURE.md) - System design and layers

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Port 6060 in use | `./build/default/dummy_server` or change port in code |
| Build fails | Check CMake version: `cmake --version` |
| Tests fail | Run with verbose: `make test-verbose` |
