# DMCP Quick Start Guide

Get Doom MCP running in 5 minutes.

## What is DMCP?

DMCP (Doom Model Context Protocol SDK) is a C/C++ library that exposes Doom game state to AI agents via the Model Context Protocol (MCP). It lets AI assistants like Claude, Cline, or Opencode:

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

## Prerequisites

- CMake 3.25+
- C++17 compiler (GCC 9+, Clang 10+)
- SDL2 (for engine adapters)
- Python 3 + pytest (for e2e tests)

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

## Verify Installation

```bash
# Health check
curl http://localhost:6060/health
# {"status": "ok", "clients": 0}

# List MCP tools
curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/list"}'
```

## Next Steps

- [Usage Examples](USAGE.md) - Common patterns and workflows
- [Integration Guide](INTEGRATION.md) - Connect to Claude, Cline, etc.
- [API Reference](README.md) - Full API documentation
- [Architecture](ARCHITECTURE.md) - System design and layers

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Port 6060 in use | `./build/dummy_server` or change port in code |
| Build fails | Check CMake version: `cmake --version` |
| Tests fail | Run with verbose: `make test-verbose` |
