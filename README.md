# Doom Model Context Protocol SDK

A zero-allocation sidecar for Doom-family engines that streams real-time game
state over SSE/JSON-RPC and serves on-demand PNG screenshots for MCP agents.

## Highlights
- **C++17 SDK + C bridge** – Engine code only touches the bridge header.
- **Zero-alloc snapshots** – Object pool + SPSC queue keep the 35 Hz logic loop
  free of allocations.
- **uWebSockets SSE server** – `/sse` streams JSON frames, `/tools/call`
  receives JSON-RPC commands.
- **PNG screenshot pipeline** – Agents call `capture_screenshot`; the engine
  captures RGBA buffers and `dmcp` encodes + serves `/screenshot/latest.png`.

## Building the SDK

```bash
cmake -S doom-mcp -B doom-mcp/build
cmake --build doom-mcp/build
```

Dependencies are fetched via CPM (nlohmann/json, tl::expected, uSockets,
uWebSockets, stb_image_write header).

`dmcp_config_t.target_hz` controls how often snapshots are emitted. Set it to
10 Hz by default to reduce bandwidth (even if you call `dmcp_process_tick()` at
35 Hz, the SDK only forwards every 100 ms).

## Integrating with UZDoom

1. **Add the SDK**
   ```cmake
   add_subdirectory(doom-mcp)
   target_link_libraries(uzdoom PRIVATE dmcp)
   target_sources(uzdoom PRIVATE doom-mcp/adapters/dmcp_uzdoom.cpp)
   ```

2. **Bootstrap at startup**
   ```cpp
   #include "dmcp/dmcp.h"

   extern "C" void dmcp_adapter_setup();

   void D_DoomInit() {
     dmcp_config_t cfg = dmcp_default_config();
     if (dmcp_init(&cfg) == 0) {
       dmcp_adapter_setup();
     }
   }
   ```

3. **Hook the logic loop (default stream rate: 10 Hz)**
   ```cpp
   void G_Ticker() {
     // ... core game logic ...
     dmcp_process_tick();  // throttled internally to cfg.target_hz (default 10)

     if (dmcp_consume_screenshot_request()) {
       dmcp_screenshot_frame_t shot{
           .pixels = framebuffer_rgba,
           .width = SCREENWIDTH,
           .height = SCREENHEIGHT,
           .stride = SCREENWIDTH * 4};
       dmcp_submit_screenshot(&shot);
     }
   }
   ```

4. **Expose the adapter**
   `adapters/dmcp_uzdoom.cpp` is the single translation unit that touches the
   engine headers. Use the `dmcp::Binder` helpers to map engine state to the
   `dmcp::Snapshot` schema.

## Protocol Surface

| Endpoint | Description |
| --- | --- |
| `POST /mcp` | Capability handshake (`notifications`, `screenshot`) |
| `GET /sse` | Server-Sent Events stream (`event: state`, `event: screenshot`) |
| `POST /tools/call` | JSON-RPC tool calls; `capture_screenshot` queues a request |
| `GET /screenshot/latest.png` | Latest PNG + headers (`X-Width`, `X-Height`) |

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

`screenshot` events broadcast `{ "uri": "/screenshot/latest.png", "width": 640,
"height": 480, "captured_at": "2025-01-01T12:00:00Z" }`.

## Screenshot Flow
1. Agent POSTs `{"jsonrpc":"2.0","method":"tools/call","params":{"name":"capture_screenshot"}}`.
2. Engine observes `dmcp_consume_screenshot_request() == true` on the next
   `G_Ticker` and captures an RGBA buffer.
3. Engine submits via `dmcp_submit_screenshot`, which encodes to PNG and bumps
   the SSE `screenshot` event.
4. Agent downloads `/screenshot/latest.png`.

## Next Steps
- Add unit/integration tests for the SSE flow.
- Wire the adapter into the actual UZDoom build.
- Extend the snapshot schema with more engine data as needed.
