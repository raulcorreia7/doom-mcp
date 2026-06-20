#include "mcp/core/api.h"

extern "C" {

void mcp_core_api_get(mcp_core_api_info_t* out_info) {
  if (!out_info) {
    return;
  }

  out_info->struct_size = sizeof(mcp_core_api_info_t);
  out_info->api_version = MCP_CORE_API_VERSION;
  out_info->status_size = sizeof(mcp_status_t);
  out_info->reserved    = 0;
}
}
