#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Protocol Versions
// ============================================================================
#define MCP_JSONRPC_VERSION "2.0"

// ============================================================================
// Buffer Sizes
// ============================================================================
#define MCP_BUFFER_SIZE_DEFAULT 8192
#define MCP_MAX_JSON_SIZE       16384
#define MCP_HEALTH_BUFFER_SIZE  256
#define MCP_MAX_PAYLOAD_SIZE    (1024 * 1024)

// ============================================================================
// Default Server Configuration
// ============================================================================
#define MCP_DEFAULT_PORT               6060
#define MCP_DEFAULT_TARGET_HZ          10
#define MCP_DEFAULT_SNAPSHOT_POOL_SIZE 16
#define MCP_DEFAULT_QUEUE_SLOTS        4
#define MCP_STARTUP_TIMEOUT_SECONDS    2

// ============================================================================
// Screenshot Defaults
// ============================================================================
#define MCP_DEFAULT_SCREENSHOT_WIDTH  640
#define MCP_DEFAULT_SCREENSHOT_HEIGHT 480

// ============================================================================
// HTTP Endpoints
// ============================================================================
#define MCP_ENDPOINT_MCP        "/mcp"
#define MCP_ENDPOINT_SSE        "/sse"
#define MCP_ENDPOINT_HEALTH     "/health"
#define MCP_ENDPOINT_SCREENSHOT "/screenshot/latest.png"

// ============================================================================
// Array Limits
// ============================================================================
#define MCP_MAX_ENEMIES    256
#define MCP_MAX_INVENTORY  64
#define MCP_MAX_ITEM_NAME  64
#define MCP_MAX_ENEMY_TYPE 128
#define MCP_MAX_LEVEL_ID   32
#define MCP_MAX_LEVEL_NAME 96

// ============================================================================
// Health Check Response
// ============================================================================
#define MCP_HEALTH_RESPONSE "{\"status\":\"ok\"}"

#ifdef __cplusplus
}
#endif
