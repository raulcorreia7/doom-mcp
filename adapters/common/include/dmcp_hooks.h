#ifndef DMCP_HOOKS_H
#define DMCP_HOOKS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dmcp_hooks dmcp_hooks_t;

typedef void (*dmcp_init_fn)(dmcp_hooks_t* hooks, int port);
typedef void (*dmcp_tick_fn)(void);
typedef void (*dmcp_shutdown_fn)(void);

struct dmcp_hooks {
  dmcp_init_fn     init;
  dmcp_tick_fn     tick;
  dmcp_shutdown_fn shutdown;
  void*            ctx;
};

typedef struct {
  int  port;
  int  target_hz;
  bool screenshot_enabled;
} dmcp_engine_config_t;

static inline dmcp_engine_config_t dmcp_engine_config_default(void) {
  dmcp_engine_config_t cfg = {0};
  cfg.port                 = 6060;
  cfg.target_hz            = 35;
  cfg.screenshot_enabled   = false;
  return cfg;
}

int dmcp_engine_port_from_argv(int argc, char** argv, const char* flag);

#ifdef __cplusplus
}
#endif

#endif
