#include "dmcp_hooks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
