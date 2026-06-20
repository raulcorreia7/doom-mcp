#include "dmcp_hooks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

dmcp_engine_config_t dmcp_engine_config_from_argv(int argc, char** argv) {
  dmcp_engine_config_t cfg;
  int                  port;

  cfg = dmcp_engine_config_default();

  port = dmcp_engine_port_from_argv(argc, argv, "dmcp_port");
  if (port > 0) {
    cfg.port = port;
  }

  cfg.allow_cheats = dmcp_engine_flag_from_argv(argc, argv, "dmcp_allow_cheats");
  cfg.allow_console_commands =
      cfg.allow_cheats || dmcp_engine_flag_from_argv(argc, argv, "dmcp_allow_console");
  cfg.screenshot_enabled = dmcp_engine_flag_from_argv(argc, argv, "dmcp_enable_screenshots");

  return cfg;
}

int dmcp_engine_port_from_argv(int argc, char** argv, const char* flag) {
  char needle[64];
  int  i;

  if (!argv || !flag || argc < 2) {
    return 0;
  }

  snprintf(needle, sizeof(needle), "-%s", flag);

  for (i = 1; i < argc - 1; ++i) {
    if (strcmp(argv[i], needle) == 0 || strcmp(argv[i], flag) == 0) {
      char* end  = NULL;
      long  port = strtol(argv[i + 1], &end, 10);
      if (end && *end == '\0' && port >= 1 && port <= 65535) {
        return (int)port;
      }
      return -1;
    }
  }
  return 0;
}

bool dmcp_engine_flag_from_argv(int argc, char** argv, const char* flag) {
  char needle[64];
  int  i;

  if (!argv || !flag || argc < 2) {
    return false;
  }

  snprintf(needle, sizeof(needle), "-%s", flag);

  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], needle) == 0 || strcmp(argv[i], flag) == 0) {
      return true;
    }
  }

  return false;
}
