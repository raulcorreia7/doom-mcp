#pragma once

#include "dmcp/doom/api.h"
#include "dmcp/doom/commands.h"
#include "dmcp/doom/config.h"
#include "dmcp/doom/content.h"
#include "dmcp/doom/protocol.h"
#include "dmcp/doom/types.h"
#include "mcp/core/version.h"

#ifndef DMCP_VERSION_MAJOR
#define DMCP_VERSION_MAJOR MCP_CORE_VERSION_MAJOR
#endif
#ifndef DMCP_VERSION_MINOR
#define DMCP_VERSION_MINOR MCP_CORE_VERSION_MINOR
#endif
#ifndef DMCP_VERSION_PATCH
#define DMCP_VERSION_PATCH MCP_CORE_VERSION_PATCH
#endif
#ifndef DMCP_VERSION
#define DMCP_VERSION MCP_CORE_VERSION
#endif
