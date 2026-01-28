cmake_minimum_required(VERSION 3.25)
project(dmcp LANGUAGES C CXX VERSION 0.5.0)

# ============================================================================
# Options
# ============================================================================
option(DMCP_BUILD_TESTS "Build tests" OFF)
option(DMCP_BUILD_EXAMPLES "Build example servers" ON)
option(DMCP_BUILD_ADAPTER_ZDOOM "Build ZDoom adapter" OFF)
option(DMCP_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
option(DMCP_USE_CCACHE "Use ccache for faster rebuilds" ON)

# JSON library selection
set(DMCP_JSON_LIBRARY "yyjson" CACHE STRING "JSON library (yyjson or nlohmann)")
set_property(CACHE DMCP_JSON_LIBRARY PROPERTY STRINGS yyjson nlohmann)

# ============================================================================
# Build Configuration
# ============================================================================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Use ccache if available
if(DMCP_USE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif()
endif()

# ============================================================================
# CPM Package Manager
# ============================================================================
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cache" CACHE PATH "CPM cache directory")

set(CPM_DOWNLOAD_VERSION 0.42.0)
if(CPM_SOURCE_CACHE)
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
  set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
  file(DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
    ${CPM_DOWNLOAD_LOCATION}
  )
endif()
include(${CPM_DOWNLOAD_LOCATION})

# ============================================================================
# Dependencies
# ============================================================================

# JSON Library
if(DMCP_JSON_LIBRARY STREQUAL "yyjson")
  message(STATUS "DMCP: Using yyjson (fast)")
  CPMAddPackage(
    NAME yyjson
    GITHUB_REPOSITORY ibireme/yyjson
    VERSION 0.10.0
    GIT_TAG 0.10.0
    OPTIONS "YYJSON_BUILD_TESTS OFF" "YYJSON_BUILD_MISC OFF"
  )
  set(DMCP_JSON_TARGET yyjson)
  set(DMCP_JSON_DEFINE "DMCP_USE_YYJSON")
  
elseif(DMCP_JSON_LIBRARY STREQUAL "nlohmann")
  message(STATUS "DMCP: Using nlohmann/json (compatibility)")
  CPMFindPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    VERSION 3.12.0
  )
  set(DMCP_JSON_TARGET nlohmann_json::nlohmann_json)
  set(DMCP_JSON_DEFINE "DMCP_USE_NLOHMANN")
else()
  message(FATAL_ERROR "Invalid DMCP_JSON_LIBRARY: ${DMCP_JSON_LIBRARY}. Use 'yyjson' or 'nlohmann'")
endif()

# uSockets - Low-level networking
CPMFindPackage(
  NAME uSockets
  GITHUB_REPOSITORY uNetworking/uSockets
  VERSION 0.8.8
  GIT_SUBMODULES ""
)

if(uSockets_ADDED)
  file(GLOB_RECURSE USOCKETS_SOURCES ${uSockets_SOURCE_DIR}/src/*.c)
  add_library(uSockets STATIC ${USOCKETS_SOURCES})
  target_include_directories(uSockets PUBLIC ${uSockets_SOURCE_DIR}/src)
  target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL)
  
  if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(uSockets PUBLIC Threads::Threads)
  elseif(WIN32)
    target_link_libraries(uSockets PUBLIC ws2_32)
  endif()
  
  if(NOT MSVC)
    target_compile_options(uSockets PRIVATE -w)
  endif()
endif()

# uWebSockets - HTTP/WebSocket server (header-only)
CPMFindPackage(
  NAME uWebSockets
  GITHUB_REPOSITORY uNetworking/uWebSockets
  VERSION 20.74.0
  GIT_SUBMODULES ""
)

if(uWebSockets_ADDED)
  find_package(ZLIB REQUIRED)
  add_library(uWebSockets INTERFACE)
  target_include_directories(uWebSockets INTERFACE ${uWebSockets_SOURCE_DIR}/src)
  target_link_libraries(uWebSockets INTERFACE uSockets ${ZLIB_LIBRARIES})

  if(NOT MSVC)
    target_compile_options(uWebSockets INTERFACE -Wno-deprecated-declarations)
  endif()
endif()

# Catch2 - Testing framework (only if tests are enabled)
if(DMCP_BUILD_TESTS)
  CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.7.1
    GIT_TAG v3.7.1
    OPTIONS "CATCH_INSTALL_DOCS OFF" "CATCH_INSTALL_EXTRAS OFF"
  )
endif()

# ============================================================================
# Generic MCP Layer (mcp::generic)
# ============================================================================
set(MCP_SOURCES
  src/mcp/server.cpp
  src/mcp/sse_transport.cpp
  src/mcp/json/${DMCP_JSON_LIBRARY}.cpp
)

add_library(mcp_generic STATIC ${MCP_SOURCES})
add_library(mcp::generic ALIAS mcp_generic)

target_include_directories(mcp_generic
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(mcp_generic
  PUBLIC
    ${DMCP_JSON_TARGET}
    uWebSockets
)

target_compile_definitions(mcp_generic
  PUBLIC
    ${DMCP_JSON_DEFINE}
    MCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    MCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    MCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

set_target_properties(mcp_generic PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

if(NOT MSVC)
  target_compile_options(mcp_generic PRIVATE
    -Wall -Wextra -Wpedantic
    -Wconversion -Wshadow
    -Wunused
  )
endif()

# ============================================================================
# Doom MCP Layer (dmcp::core)
# ============================================================================
add_library(dmcp_core STATIC
  src/doom/context.cpp
  src/doom/commands.cpp
)
add_library(dmcp::core ALIAS dmcp_core)

target_include_directories(dmcp_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(dmcp_core
  PUBLIC
    mcp::generic
)

target_compile_definitions(dmcp_core
  PUBLIC
    DMCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    DMCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    DMCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

set_target_properties(dmcp_core PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

if(NOT MSVC)
  target_compile_options(dmcp_core PRIVATE
    -Wall -Wextra -Wpedantic
  )
endif()

# ============================================================================
# ZDoom Adapter (dmcp::zdoom)
# ============================================================================
if(DMCP_BUILD_ADAPTER_ZDOOM)
  add_library(dmcp_zdoom STATIC
    adapters/zdoom/adapter.cpp
    adapters/zdoom/commands.cpp
  )
  add_library(dmcp::zdoom ALIAS dmcp_zdoom)
  
  target_include_directories(dmcp_zdoom
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/adapters/zdoom>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
      ${DMCP_ZDOOM_INCLUDE_DIRS}
  )
  
  target_link_libraries(dmcp_zdoom
    PUBLIC
      dmcp::core
  )
  
  if(NOT MSVC)
    target_compile_options(dmcp_zdoom PRIVATE -w)
  endif()
endif()

# ============================================================================
# Examples
# ============================================================================
if(DMCP_BUILD_EXAMPLES AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_executable(dummy_server examples/dummy_server.cpp)
  target_link_libraries(dummy_server PRIVATE dmcp::core)

  if(NOT MSVC)
    target_compile_options(dummy_server PRIVATE -Wall -Wextra)
  endif()
endif()

# ============================================================================
# Tests
# ============================================================================
if(DMCP_BUILD_TESTS AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  enable_testing()
  add_subdirectory(tests)
endif()

# ============================================================================
# Sanitizers
# ============================================================================
if(DMCP_ENABLE_SANITIZERS AND NOT MSVC)
  foreach(target mcp_generic dmcp_core)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endforeach()
  
  if(DMCP_BUILD_EXAMPLES)
    target_compile_options(dummy_server PRIVATE -fsanitize=address,undefined)
    target_link_options(dummy_server PRIVATE -fsanitize=address,undefined)
  endif()
endif()

# ============================================================================
# Summary
# ============================================================================
message(STATUS "")
message(STATUS "DMCP Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  JSON Library: ${DMCP_JSON_LIBRARY}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build examples: ${DMCP_BUILD_EXAMPLES}")
message(STATUS "  Build ZDoom adapter: ${DMCP_BUILD_ADAPTER_ZDOOM}")
message(STATUS "  Sanitizers: ${DMCP_ENABLE_SANITIZERS}")
message(STATUS "")
