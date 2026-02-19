#pragma once

/*
 * Export macros for DMCP shared library support
 *
 * Usage:
 *   - Define DMCP_STATIC when using a static build of the library
 *   - Define DMCP_BUILDING when building the shared library itself
 *   - Use DMCP_API prefix for public API functions
 */

#ifdef DMCP_STATIC
#define DMCP_API
#else
#ifdef _WIN32
#ifdef DMCP_BUILDING
#define DMCP_API __declspec(dllexport)
#else
#define DMCP_API __declspec(dllimport)
#endif
#else
#define DMCP_API __attribute__((visibility("default")))
#endif
#endif
