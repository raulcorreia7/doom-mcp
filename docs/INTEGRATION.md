# MCP Client Integration

This guide shows how to connect MCP clients to a running DMCP HTTP endpoint.
DMCP SDK docs do not own game startup; start a DMCP-enabled engine from the
consuming engine repository, then point clients at the server URL.

Default URL:

```text
http://localhost:6060/mcp
```

## Generic MCP Configuration

```json
{
  "mcpServers": {
    "doom": {
      "type": "http",
      "url": "http://localhost:6060/mcp"
    }
  }
}
```

## Verify The Endpoint

```bash
curl http://localhost:6060/health

INIT=$(curl -s -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"verify","version":"1.0"}}}')

SESSION_ID=$(printf '%s' "$INIT" | jq -r '.result.sessionId')

curl -s -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: ${SESSION_ID}" \
  -d '{"jsonrpc":"2.0","method":"notifications/initialized","params":{}}' >/dev/null

curl -X POST http://localhost:6060/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2025-11-25" \
  -H "MCP-Session-Id: ${SESSION_ID}" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
```

Strict mode is the default: after `initialize`, include both
`MCP-Protocol-Version` and `MCP-Session-Id` on every `/mcp` request.

## Python Agent Helper

For Codex, OpenCode, Claude Code, and similar CLI agents, prefer the optional
Python helper before raw MCP calls. It keeps repeated commands compact and
avoids dumping tool schemas into the model context.

From the repository root:

```bash
uv run --project examples/agents/python dmcp-agent tools --compact
uv run --project examples/agents/python dmcp-agent --pretty brief
uv run --project examples/agents/python dmcp-agent --pretty shell
```

For one-off execution without creating a persistent project environment:

```bash
uvx --from examples/agents/python dmcp-agent brief
```

The helper expects a running DMCP server at `http://localhost:6060/mcp` by
default. Install `uv` from <https://docs.astral.sh/uv/> if it is not already
available.

## Codex

```toml
[mcp_servers.doom]
url = "http://localhost:6060/mcp"
enabled = true
```

## OpenCode

```json
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "doom": {
      "type": "remote",
      "url": "http://localhost:6060/mcp",
      "enabled": true,
      "timeout": 10000
    }
  }
}
```

## Claude Desktop / Claude Code

```json
{
  "mcpServers": {
    "doom": {
      "type": "http",
      "url": "http://localhost:6060/mcp"
    }
  }
}
```

## VS Code Clients

Most VS Code MCP clients accept the same `mcpServers` object:

```json
{
  "mcpServers": {
    "doom": {
      "type": "http",
      "url": "http://localhost:6060/mcp"
    }
  }
}
```

## Tool Calling Shape

DMCP exposes one MCP input shape for tools: JSON-RPC `tools/call` with
structured `arguments`.

```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "tools/call",
  "params": {
    "name": "get_player",
    "arguments": {}
  }
}
```

Native JSON-RPC aliases such as direct `get_player` methods are not part of the
public MCP surface.

## Useful Tools

- `get_available_content` discovers canonical names accepted by commands.
- `get_player`, `get_enemies`, `get_entities`, `get_items`, and `get_map` read
  focused state.
- `get_state_batch` groups read-only state requests.
- `execute_batch` queues mutating commands in order, except level changes.
- `get_command_result` polls queued command completion.
- `get_command_examples` returns structured payload examples.

Use `change_level` as a separate tool call, wait for completion, then send
follow-up commands.

## SDK Validation

SDK CI validates MCP protocol behavior without launching a game:

```bash
cmake -B build/default \
  -DDMCP_BUILD_TESTS=ON \
  -DDMCP_BUILD_INTEGRATION_TESTS=ON \
  -DDMCP_BUILD_ADAPTER_FAKE=ON
cmake --build build/default --parallel
ctest --test-dir build/default -L "unit|sdk" -LE "requires_game" --output-on-failure
```

Engine-backed validation belongs in the consuming engine repository.

## Troubleshooting

If the server is not found, verify the engine-owned DMCP server is running and
that the client URL matches the configured port.

If tools do not appear, complete the initialize lifecycle, include the returned
session ID on later requests, and restart the MCP client after config changes.
