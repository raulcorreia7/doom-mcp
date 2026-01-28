#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Generic Result Type
// ============================================================================

/**
 * @brief Generic result struct with error code and message
 *
 * This struct is used across all DMCP APIs to return operation results with
 * detailed error information. All result types should include this struct and
 * use the provided macros for convenience.
 *
 * The code field follows a simple convention:
 * - 0 = Success
 * - Negative values = Error codes (defined in respective layers)
 * - Positive values = Reserved for future use
 *
 * @note The message field points to static string literals and does not need
 *       to be freed. Thread-safe as it uses compile-time string literals.
 *
 * Breaking Changes:
 * - Previously mcp_result_t was an enum (int)
 * - Now mcp_result_t is a struct with code and message fields (v0.5.0)
 *
 * Migration:
 * @code
 * // Before (enum comparison)
 * mcp_result_t result = mcp_server_method_register(...);
 * if (result != MCP_OK) { ... }
 *
 * // After (struct comparison)
 * mcp_result_t result = mcp_server_method_register(...);
 * if (result.code != MCP_RESULT_CODE_OK) {
 *     printf("Error: %s\n", result.message);
 * }
 * @endcode
 */
typedef struct {
  int32_t code;  ///< Error code (0 = success, negative = error)
  const char*
      message;  ///< Static error message string (thread-local if needed)
} mcp_result_generic_t;

/**
 * @brief Helper macro to create a result with code and message
 *
 * Use this macro to create result structs inline. The resulting struct is a
 * compound literal and can be returned directly from functions.
 *
 * Example:
 * @code
 * return MCP_RESULT_MAKE(-1, "Invalid argument");
 * @endcode
 */
#define MCP_RESULT_MAKE(code, msg) ((mcp_result_generic_t){(code), (msg)})

/**
 * @brief Helper macro to create success result
 *
 * Creates a result with code=0 and the provided or default message.
 *
 * @param message Optional success message (defaults to "Success" if NULL)
 *
 * Example:
 * @code
 * return MCP_OK;
 * return MCP_RESULT_OK("Operation completed successfully");
 * @endcode
 */
#define MCP_RESULT_OK(message) \
  MCP_RESULT_MAKE(0, (message) ? (message) : "Success")

/**
 * @brief Helper macro to create error result
 *
 * Creates a result with the specified error code and message.
 *
 * @param code Error code (negative integer)
 * @param message Error message (defaults to "Unknown error" if NULL)
 *
 * Example:
 * @code
 * return MCP_ERROR_INVALID_ARGS;
 * return MCP_RESULT_ERROR(-1, "Invalid parameter");
 * @endcode
 */
#define MCP_RESULT_ERROR(code, message) \
  MCP_RESULT_MAKE((code), (message) ? (message) : "Unknown error")

#ifdef __cplusplus
}
#endif
