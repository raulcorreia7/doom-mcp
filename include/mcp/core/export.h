#pragma once

/*
 * Export macros for the MCP core foundation API.
 *
 * Define MCP_CORE_STATIC when consuming a static build and
 * MCP_CORE_BUILDING when building the shared library that owns the symbols.
 */

#if defined(MCP_CORE_STATIC)
#define MCP_CORE_API
#elif defined(_WIN32)
#if defined(MCP_CORE_BUILDING)
#define MCP_CORE_API __declspec(dllexport)
#else
#define MCP_CORE_API __declspec(dllimport)
#endif
#else
#define MCP_CORE_API __attribute__((visibility("default")))
#endif
