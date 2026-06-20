#ifndef DMCP_HOOKS_H
#define DMCP_HOOKS_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int  port;
  int  target_hz;
  bool screenshot_enabled;
  bool allow_console_commands;
  bool allow_cheats;
} dmcp_engine_config_t;

static inline dmcp_engine_config_t dmcp_engine_config_default(void) {
  dmcp_engine_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.port                   = 6060;
  cfg.target_hz              = 35;
  cfg.screenshot_enabled     = false;
  cfg.allow_console_commands = true;
  cfg.allow_cheats           = true;
  return cfg;
}

dmcp_engine_config_t dmcp_engine_config_from_argv(int argc, char** argv);
int                  dmcp_engine_port_from_argv(int argc, char** argv, const char* flag);
bool                 dmcp_engine_flag_from_argv(int argc, char** argv, const char* flag);

#ifdef __cplusplus
}
#endif

#endif
