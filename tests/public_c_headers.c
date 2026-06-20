#include "dmcp/adapter/content.h"
#include "dmcp/adapter/entities.h"
#include "dmcp/adapter/utils.h"
#include "dmcp/adapter/validation.h"
#include "dmcp/adapters/crispy.h"
#include "dmcp/adapters/fake.h"
#include "dmcp/adapters/zdoom.h"
#include "dmcp/doom/dmcp.h"
#include "mcp/core/core.h"
#include "mcp/core/string.h"
#include "mcp/game/game.h"
#include "mcp/generic/server.h"
#include "mcp/generic/transport.h"

#include <string.h>

int main(void) {
  mcp_core_api_info_t api;
  memset(&api, 0, sizeof(api));
  api.struct_size = sizeof(api);
  mcp_core_api_get(&api);
  int cmp = mcp_strcmp_ci("DMCP", "dmcp");

  mcp_server_config_t     server_config     = mcp_default_config();
  mcp_game_registration_t game_registration = mcp_game_registration_default();
  dmcp_config_t           doom_config       = dmcp_config_default();
  dmcp_crispy_config_t    crispy_config     = dmcp_crispy_config_default();
  dmcp_fake_config_t      fake_config       = dmcp_fake_config_default();
  dmcp_zdoom_config_t     zdoom_config      = dmcp_zdoom_config_default();

  dmcp_snapshot_t snapshot;
  dmcp_snapshot_clear(&snapshot);

  return (server_config.struct_size == sizeof(server_config) &&
          doom_config.struct_size == sizeof(doom_config) && doom_config.tools.game == true &&
          doom_config.tools.input == true &&
          doom_config.permissions.allow_console_commands == true &&
          doom_config.permissions.allow_cheats == true &&
          game_registration.struct_size == sizeof(game_registration) &&
          crispy_config.struct_size == sizeof(crispy_config) &&
          fake_config.struct_size == sizeof(fake_config) &&
          zdoom_config.struct_size == sizeof(zdoom_config) &&
          api.api_version == MCP_CORE_API_VERSION && cmp == 0)
             ? 0
             : 1;
}
