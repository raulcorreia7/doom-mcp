#pragma once

/*
 * Export macros for shared library support
 *
 * Usage:
 *   - Define MCP_STATIC when using a static build of the library
 *   - Define MCP_BUILDING when building the shared library itself
 *   - Use MCP_API prefix for public API functions
 */

#ifdef MCP_STATIC
#define MCP_API
#else
#ifdef _WIN32
#ifdef MCP_BUILDING
#define MCP_API __declspec(dllexport)
#else
#define MCP_API __declspec(dllimport)
#endif
#else
#define MCP_API __attribute__((visibility("default")))
#endif
#endif
