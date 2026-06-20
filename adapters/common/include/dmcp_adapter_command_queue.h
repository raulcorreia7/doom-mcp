#ifndef DMCP_ADAPTER_COMMAND_QUEUE_H
#define DMCP_ADAPTER_COMMAND_QUEUE_H

#include <stdbool.h>
#include <stddef.h>

#include "dmcp/doom/commands.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*dmcp_adapter_command_execute_fn)(void* adapter_ctx, const dmcp_command_t* cmd,
                                                char* out_message, size_t out_message_size);
typedef bool (*dmcp_adapter_input_execute_fn)(void* adapter_ctx, const dmcp_command_t* input_cmd,
                                              char* out_message, size_t out_message_size);

// Process queued command requests and complete each command sequence.
//
// max_items:
// - 0  => drain full queue
// - >0 => process up to max_items commands
//
// Returns number of processed commands.
static inline int dmcp_adapter_process_command_queue(dmcp_context_t* dmcp_ctx, void* adapter_ctx,
                                                     dmcp_adapter_command_execute_fn execute_fn,
                                                     size_t                          max_items) {
  dmcp_command_t cmd;
  int            processed;

  if (!dmcp_ctx || !execute_fn) {
    return 0;
  }

  processed = 0;
  while ((max_items == 0 || (size_t)processed < max_items) && dmcp_command_pop(dmcp_ctx, &cmd)) {
    char result_message[256] = {0};
    bool success = execute_fn(adapter_ctx, &cmd, result_message, sizeof(result_message));

    dmcp_command_result_complete(dmcp_ctx, &cmd, success,
                                 result_message[0] ? result_message : NULL);
    processed++;
  }

  return processed;
}

// Process queued player input requests and complete each input sequence.
//
// max_items:
// - 0  => drain full queue
// - >0 => process up to max_items inputs
//
// Returns number of processed inputs.
static inline int dmcp_adapter_process_input_queue(dmcp_context_t* dmcp_ctx, void* adapter_ctx,
                                                   dmcp_adapter_input_execute_fn execute_fn,
                                                   size_t                        max_items) {
  dmcp_command_t input_cmd;
  int            processed;

  if (!dmcp_ctx || !execute_fn) {
    return 0;
  }

  processed = 0;
  while ((max_items == 0 || (size_t)processed < max_items) &&
         dmcp_input_pop(dmcp_ctx, &input_cmd)) {
    char result_message[256] = {0};
    bool success = execute_fn(adapter_ctx, &input_cmd, result_message, sizeof(result_message));

    dmcp_command_result_complete(dmcp_ctx, &input_cmd, success,
                                 result_message[0] ? result_message : NULL);
    processed++;
  }

  return processed;
}

#ifdef __cplusplus
}
#endif

#endif  // DMCP_ADAPTER_COMMAND_QUEUE_H
