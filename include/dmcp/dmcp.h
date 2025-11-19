#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint16_t port;
  uint32_t target_hz;
  size_t snapshot_pool;
  size_t queue_slots;
  size_t enemy_capacity;
  size_t inventory_capacity;
  struct {
    bool enable;
    uint32_t width;
    uint32_t height;
  } screenshot;
} dmcp_config_t;

typedef struct {
  const uint8_t* pixels;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
} dmcp_screenshot_frame_t;

dmcp_config_t dmcp_default_config(void);

int dmcp_init(const dmcp_config_t* config);
void dmcp_shutdown(void);
void dmcp_process_tick(void);

bool dmcp_is_running(void);

bool dmcp_consume_screenshot_request(void);
int dmcp_submit_screenshot(const dmcp_screenshot_frame_t* frame);
uint64_t dmcp_dropped_snapshot_count(void);

#ifdef __cplusplus
}
#endif
