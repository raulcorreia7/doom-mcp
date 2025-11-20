# Doom Model Context Protocol SDK

A zero-allocation sidecar for Doom-family engines that streams real-time game state over SSE/JSON-RPC and serves on-demand PNG screenshots for MCP agents.

## Highlights
- **C++17 SDK + C bridge** – Engine code only touches the bridge header.
- **Zero-alloc snapshots** – Object pool + SPSC queue keep the 35 Hz logic loop free of allocations.
- **Async Architecture** – Network I/O and PNG encoding happen on a background thread.
- **Modern CMake** – Uses `vcpkg` for dependencies and `INTERFACE` libraries for easy integration.

## Building the SDK

**Prerequisites:** CMake 3.15+, C++17 compiler, vcpkg.

```bash
# Configure using vcpkg preset
cmake --preset default

# Build
cmake --build --preset default
```

## Running the Example

The `dummy_server` simulates a Doom engine to validate the protocol.

```bash
./build/dummy_server
```

You can verify the output using the provided test script:
```bash
./test_mcp.sh
```

## Integrating with UZDoom

The SDK provides a ready-to-use adapter for UZDoom/GZDoom.

### 1. CMake Integration
Add the SDK as a subdirectory in your engine's `CMakeLists.txt`:

```cmake
add_subdirectory(dmcp-sdk)

# Link the adapter interface.
# This compiles the adapter source files AS PART OF your engine,
# giving them access to your engine's internal headers.
target_link_libraries(uzdoom PRIVATE dmcp::adapter::uzdoom)
```

### 2. Bootstrap at Startup (`d_main.cpp`)

```cpp
// Declare the adapter hook
extern "C" void dmcp_adapter_setup();
extern "C" void dmcp_adapter_shutdown();

void D_DoomInit() {
  // ... existing init code ...
  
  // Initialize DMCP
  dmcp_adapter_setup();
}

// Clean up on exit
void D_QuitNetGame() {
    dmcp_adapter_shutdown();
    // ...
}
```

### 3. Hook the Logic Loop (`g_game.cpp`)

```cpp
extern "C" void dmcp_adapter_update();

void G_Ticker() {
  // ... core game logic ...
  
  // Capture state and process network events
  dmcp_adapter_update();
}
```

## Protocol Surface

| Endpoint | Description |
| --- | --- |
| `POST /mcp` | Capability handshake (`notifications`, `screenshot`) |
| `GET /sse` | Server-Sent Events stream (`event: state`, `event: screenshot`) |
| `POST /tools/call` | JSON-RPC tool calls; `capture_screenshot` queues a request |
| `GET /screenshot/latest.png` | Latest PNG + headers (`X-Width`, `X-Height`) |

### Snapshot Data (SSE)
Each `event: state` message contains the serialized snapshot:

```json
{
  "player": {
    "health": 85,
    "armor": 25,
    "ammo": 40,
    "position": { "x": 42.0, "y": 18.0 },
    "inventory": [
      { "name": "shells", "amount": 12 },
      { "name": "stimpacks", "amount": 2 }
    ]
  },
  "level": {
    "tic": 2100,
    "name": "MAP01",
    "kill_count": 15,
    "item_count": 3,
    "secret_count": 1
  },
  "enemies": [
    { "id": 1, "type": "imp", "health": 30, "position": { "x": 12.3, "y": 45.6 } }
  ]
}
```

### Screenshot Flow
1. Agent POSTs `{"jsonrpc":"2.0","method":"tools/call","params":{"name":"capture_screenshot"}}`.
2. Engine observes the request, captures the framebuffer, and pushes it to the async encoder.
3. Server thread encodes PNG and broadcasts SSE event: `event: screenshot`.
4. Agent downloads `/screenshot/latest.png`.

## Architecture

*   **Core Library (`libdmcp`)**: Contains the networking logic, memory pools, and thread management. It is engine-agnostic.
*   **Adapter (`adapters/`)**: Contains the engine-specific mapping logic. It uses function composition to map engine structs (e.g., `player_t`) to the DMCP `Snapshot` schema.
