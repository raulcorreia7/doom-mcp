#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to the DMCP context
typedef struct dmcp_context_s dmcp_context_t;

// Log levels
typedef enum {
  DMCP_LOG_DEBUG = 0,
  DMCP_LOG_INFO  = 1,
  DMCP_LOG_WARN  = 2,
  DMCP_LOG_ERROR = 3
} dmcp_log_level_t;


// Return codes
typedef enum {
  DMCP_OK                    = 0,
  DMCP_ERROR_INVALID_ARGS    = -1,
  DMCP_ERROR_ENCODING_FAILED = -2,
  DMCP_ERROR_DISABLED        = -3,
  DMCP_ERROR_QUEUE_FULL      = -4
} dmcp_result_t;

// Callback for logging.
// user_data: The pointer passed in dmcp_config_t.
// level: One of dmcp_log_level_t.
// message: Null-terminated log message.
typedef void (*dmcp_log_callback_t)(void* user_data, int level,
                                    const char* message);

// Callback for the game tick logic.
// user_data: The pointer passed in dmcp_config_t.
// snapshot: Pointer to dmcp::Snapshot (C++ object). Cast to void* for C
// compatibility.
typedef void (*dmcp_tick_callback_t)(void* user_data, void* snapshot);

typedef struct {
  // Versioning to ensure ABI compatibility. Set to sizeof(dmcp_config_t).
  uint32_t struct_size;

  uint16_t port;
  uint32_t target_hz;
  size_t   snapshot_pool;
  size_t   queue_slots;
  size_t   enemy_capacity;
  size_t   inventory_capacity;

  struct {
    bool     enable;
    uint32_t width;
    uint32_t height;
  } screenshot;

  // Callback to populate the snapshot
  dmcp_tick_callback_t on_tick;

  // Callback for logging
  dmcp_log_callback_t on_log;

  void* user_data;

} dmcp_config_t;

typedef struct {
  const uint8_t* pixels;
  uint32_t       width;
  uint32_t       height;
  uint32_t       stride;
} dmcp_screenshot_frame_t;

typedef struct {
  uint64_t dropped_snapshots;
  uint64_t dropped_screenshots;
  uint64_t connected_clients;
} dmcp_stats_t;

// Initialize a default config structure
dmcp_config_t dmcp_default_config(void);

// Create a new DMCP context. Returns NULL on failure.
dmcp_context_t* dmcp_create(const dmcp_config_t* config);

// Destroy the context and free all resources.
void dmcp_destroy(dmcp_context_t* ctx);

// Process a game tick. Should be called every frame/tic.
// Internally throttles to target_hz.
void dmcp_update(dmcp_context_t* ctx);

// Check if the server is running and healthy.
bool dmcp_is_running(dmcp_context_t* ctx);

// Check if there is a pending screenshot request.
bool dmcp_has_screenshot_request(dmcp_context_t* ctx);

// Submit a screenshot frame to be served.
// Returns DMCP_OK on success, or a negative error code.
dmcp_result_t dmcp_submit_screenshot(dmcp_context_t*                ctx,
                                     const dmcp_screenshot_frame_t* frame);

// Get current statistics.
void dmcp_get_stats(const dmcp_context_t* ctx, dmcp_stats_t* stats);

#ifdef __cplusplus
}
#endif
