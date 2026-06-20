#pragma once

#include "mcp/generic/export.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Protocol Versions
// ============================================================================

/**
 * @brief JSON-RPC protocol version
 *
 * Identifies the JSON-RPC protocol version this implementation follows.
 * Used in protocol handshake for compatibility.
 */
#define MCP_JSONRPC_VERSION "2.0"

// ============================================================================
// Buffer Sizes
// ============================================================================

/**
 * @brief Default buffer size for I/O operations
 *
 * Default size for buffers used in HTTP responses, JSON parsing,
 * and general I/O operations.
 */
#define MCP_BUFFER_SIZE_DEFAULT 16384

/**
 * @brief Maximum JSON payload size
 *
 * Maximum allowed size for JSON payloads in requests and responses.
 * Larger payloads will be rejected.
 */
#define MCP_MAX_JSON_SIZE 16384

/**
 * @brief Health check buffer size
 *
 * Size of buffer allocated for health check responses.
 */
#define MCP_HEALTH_BUFFER_SIZE 256

/**
 * @brief Maximum HTTP request payload size
 *
 * Maximum size in bytes for HTTP request bodies (1MB).
 * Requests exceeding this limit are rejected with error.
 */
#define MCP_MAX_PAYLOAD_SIZE (1024 * 1024)

// ============================================================================
// Default Server Configuration
// ============================================================================

/**
 * @brief Default TCP port for MCP server
 *
 * Default port that the MCP server listens on for connections.
 * Can be overridden in mcp_server_config_t.
 */
#define MCP_DEFAULT_PORT 6060

/**
 * @brief Server startup timeout
 *
 * Maximum seconds to wait for server to start before
 * considering startup failed.
 */
#define MCP_STARTUP_TIMEOUT_SECONDS 2

// ============================================================================
// HTTP Endpoints
// ============================================================================

/**
 * @brief MCP protocol endpoint
 *
 * Unified endpoint for MCP communication:
 * - HTTP POST for JSON-RPC method calls
 * - HTTP GET for SSE stream
 */
#define MCP_ENDPOINT_MCP "/mcp"

/**
 * @brief Health check endpoint
 *
 * HTTP GET endpoint for health/status checks.
 * Returns {"status":"ok"} when server is running.
 */
#define MCP_ENDPOINT_HEALTH "/health"

// ============================================================================
// Health Check Response
// ============================================================================

/**
 * @brief Health check success response
 *
 * JSON response returned by /health endpoint when server is running.
 * Used by load balancers and health monitors.
 */
#define MCP_HEALTH_RESPONSE "{\"status\":\"ok\"}"

#ifdef __cplusplus
}
#endif
