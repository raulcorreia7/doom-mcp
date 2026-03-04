#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dmcp/doom/export.h"
#include "dmcp/doom/config.h"
#include "dmcp/doom/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Engine Kind Enumeration
// ============================================================================

typedef enum { DMCP_ENGINE_CRISPY = 0, DMCP_ENGINE_ZDOOM, DMCP_ENGINE_FAKE } dmcp_engine_kind_t;

// ============================================================================
// Engine-Specific Configuration Structures
// ============================================================================

typedef struct {
  const char* iwad_path;
  const char* pwad_path;
} dmcp_crispy_options_t;

typedef struct {
  void (*log_fn)(void* user, int level, const char* message);
  void* log_user;
  bool (*should_tick_fn)(void* user);
  void* should_tick_user;
} dmcp_zdoom_options_t;

typedef struct {
  uint32_t seed;
  uint32_t max_enemies;
  uint32_t max_entities;
} dmcp_fake_options_t;

// ============================================================================
// Unified Engine Configuration
// ============================================================================

typedef struct {
  uint32_t struct_size;

  dmcp_engine_kind_t kind;
  dmcp_config_t      base;

  uint16_t port_override;

  union {
    dmcp_crispy_options_t crispy;
    dmcp_zdoom_options_t  zdoom;
    dmcp_fake_options_t   fake;
  } engine;

} dmcp_engine_config_t;

// ============================================================================
// Configuration Defaults
// ============================================================================

static inline dmcp_engine_config_t dmcp_engine_config_default(void) {
  dmcp_engine_config_t cfg = {0};
  cfg.struct_size          = sizeof(dmcp_engine_config_t);
  cfg.kind                 = DMCP_ENGINE_FAKE;
  cfg.base                 = dmcp_config_default();
  cfg.port_override        = 0;

  cfg.engine.crispy.iwad_path = NULL;
  cfg.engine.crispy.pwad_path = NULL;

  cfg.engine.zdoom.log_fn           = NULL;
  cfg.engine.zdoom.log_user         = NULL;
  cfg.engine.zdoom.should_tick_fn   = NULL;
  cfg.engine.zdoom.should_tick_user = NULL;

  cfg.engine.fake.seed         = 1337;
  cfg.engine.fake.max_enemies  = DMCP_MAX_ENEMIES;
  cfg.engine.fake.max_entities = DMCP_MAX_ENTITIES;

  return cfg;
}

#ifdef __cplusplus
}
#endif
