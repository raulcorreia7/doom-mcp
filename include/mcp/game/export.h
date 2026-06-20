#pragma once

/*
 * Export macros for the generic game integration API.
 */

#if defined(MCP_GAME_STATIC)
#define MCP_GAME_API
#elif defined(_WIN32)
#if defined(MCP_GAME_BUILDING)
#define MCP_GAME_API __declspec(dllexport)
#else
#define MCP_GAME_API __declspec(dllimport)
#endif
#else
#define MCP_GAME_API __attribute__((visibility("default")))
#endif
