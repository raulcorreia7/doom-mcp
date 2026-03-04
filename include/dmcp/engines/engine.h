#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dmcp/doom/dmcp.h"
#include "dmcp/engines/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Opaque Engine Handle
// ============================================================================

typedef struct dmcp_engine_s dmcp_engine_t;

// ============================================================================
// Lifecycle Functions
// ============================================================================

/**
 * @brief Create a new DMCP engine instance
 *
 * Creates and initializes an engine instance based on the specified
 * configuration. The engine kind determines which underlying engine
 * implementation (Crispy Doom, ZDoom, or Fake) is instantiated.
 *
 * @param config Engine configuration including kind selector and options
 * @return Engine handle on success, NULL on failure
 *
 * @note The caller owns the returned handle and must destroy it with
 *       dmcp_engine_destroy() when done.
 * @warning Returns NULL if engine initialization fails (e.g., invalid config)
 */
DMCP_API dmcp_engine_t* dmcp_engine_create(const dmcp_engine_config_t* config);

/**
 * @brief Destroy engine instance and free all resources
 *
 * Stops the engine, closes all connections, and releases all allocated
 * resources. After this call, the engine handle becomes invalid.
 *
 * @param engine Engine handle (may be NULL, safe to call)
 *
 * @note Thread-safe - can be called from any thread
 * @note After this call, do not use the engine handle
 */
DMCP_API void dmcp_engine_destroy(dmcp_engine_t* engine);

// ============================================================================
// Runtime Functions
// ============================================================================

/**
 * @brief Process a game tick
 *
 * Call this function at the engine's target frame rate to update DMCP state.
 * This function processes pending commands, updates snapshots, and handles
 * any pending agent inputs.
 *
 * @param engine Engine handle
 * @return Result code indicating success or failure
 *
 * @note Should be called from the engine's main loop/thread
 * @note Snapshot rate is controlled by config.base.target_hz
 */
DMCP_API mcp_result_t dmcp_engine_tick(dmcp_engine_t* engine);

/**
 * @brief Process pending commands from agents
 *
 * Processes any queued commands received from MCP clients. This is typically
 * called automatically during dmcp_engine_tick(), but can be called separately
 * for finer control over command processing timing.
 *
 * @param engine Engine handle
 */
DMCP_API void dmcp_engine_commands_process(dmcp_engine_t* engine);

/**
 * @brief Process pending inputs from agents
 *
 * Processes any queued player inputs received from MCP clients. This is
 * typically called automatically during dmcp_engine_tick(), but can be called
 * separately for finer control over input processing timing.
 *
 * @param engine Engine handle
 */
DMCP_API void dmcp_engine_inputs_process(dmcp_engine_t* engine);

// ============================================================================
// Query Functions
// ============================================================================

/**
 * @brief Check if the engine is running
 *
 * Returns true if the engine is currently active and processing. Returns false
 * if the engine has been stopped or is in the process of shutting down.
 *
 * @param engine Engine handle
 * @return true if engine is running, false otherwise
 *
 * @note Returns false if engine handle is NULL
 */
DMCP_API bool dmcp_engine_is_running(const dmcp_engine_t* engine);

/**
 * @brief Get current engine statistics
 *
 * Retrieves statistics about engine operation, including dropped snapshots,
 * dropped screenshots, and client connections. Useful for monitoring and
 * debugging.
 *
 * @param engine Engine handle
 * @param stats Output structure to receive statistics
 *
 * @note Thread-safe - can be called from any thread
 * @note Stats are cumulative since engine creation
 */
DMCP_API void dmcp_engine_get_stats(dmcp_engine_t* engine, dmcp_stats_t* stats);

/**
 * @brief Get the underlying DMCP context
 *
 * Returns the DMCP context associated with this engine instance. This can
 * be used for advanced operations that require direct context access.
 *
 * @param engine Engine handle
 * @return Context handle, or NULL if engine is NULL
 *
 * @note The returned context is owned by the engine - do not destroy it
 */
DMCP_API dmcp_context_t* dmcp_engine_get_context(dmcp_engine_t* engine);

// ============================================================================
// Command Functions
// ============================================================================

/**
 * @brief Execute a command on the engine
 *
 * Executes a DMCP command immediately on the engine instance. This is
 * typically used for programmatic control of the engine from the host
 * application.
 *
 * @param engine Engine handle
 * @param cmd Command to execute
 * @return true if command executed successfully, false otherwise
 *
 * @note Command execution is synchronous
 * @note Some commands may not be supported by all engine implementations
 */
DMCP_API bool dmcp_engine_command_execute(dmcp_engine_t* engine, const dmcp_command_t* cmd);

#ifdef __cplusplus
}
#endif
