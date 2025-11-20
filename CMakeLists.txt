cmake_minimum_required(VERSION 3.25)
project(dmcp LANGUAGES C CXX VERSION 0.2.0)

# ============================================================================
# Options
# ============================================================================
option(DMCP_BUILD_EXAMPLES "Build example server" ON)
option(DMCP_BUILD_ADAPTERS "Build all standard adapters (useful for testing)" OFF)
option(DMCP_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
option(DMCP_USE_CCACHE "Use ccache for faster rebuilds" ON)

# ============================================================================
# Build Configuration
# ============================================================================
# Enable compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use ccache if available and enabled
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
# Set CPM cache directory BEFORE including CPM
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cache" CACHE PATH "Directory to download CPM dependencies")
set(CPM_USE_LOCAL_PACKAGES OFF CACHE BOOL "Use local packages if available")

# Download CPM.cmake only if not already cached
set(CPM_DOWNLOAD_VERSION 0.42.0)

if(CPM_SOURCE_CACHE)
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
  set(CPM_DOWNLOAD_LOCATION "${CMAKE_SOURCE_DIR}/.cache/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

# Only download if file doesn't exist
if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
  message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
  file(DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
    ${CPM_DOWNLOAD_LOCATION}
  )
endif()

include(${CPM_DOWNLOAD_LOCATION})

# ============================================================================
# Dependencies
# ============================================================================

# --- nlohmann_json (JSON library) ---
CPMAddPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  VERSION 3.12.0
)

# --- uSockets (Low-level socket library) ---
CPMAddPackage(
  NAME uSockets
  GITHUB_REPOSITORY uNetworking/uSockets
  VERSION 0.8.8
  GIT_SUBMODULES ""  # Don't fetch submodules
)

if(uSockets_ADDED)
  # Use GLOB to get all C source files
  file(GLOB_RECURSE USOCKETS_SOURCES ${uSockets_SOURCE_DIR}/src/*.c)
  
  add_library(uSockets STATIC ${USOCKETS_SOURCES})
  target_include_directories(uSockets PUBLIC ${uSockets_SOURCE_DIR}/src)
  target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL)
  
  # Platform-specific libraries
  if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(uSockets PUBLIC Threads::Threads)
  elseif(WIN32)
    target_link_libraries(uSockets PUBLIC ws2_32)
  endif()
  
  # Suppress warnings from third-party code
  if(NOT MSVC)
    target_compile_options(uSockets PRIVATE -w)
  endif()
endif()

# --- uWebSockets (WebSocket/HTTP server, header-only) ---
CPMAddPackage(
  NAME uWebSockets
  GITHUB_REPOSITORY uNetworking/uWebSockets
  VERSION 20.74.0
  GIT_SUBMODULES ""  # Don't fetch submodules
)

if(uWebSockets_ADDED)
  # uWebSockets needs zlib for compression
  find_package(ZLIB REQUIRED)
  
  # Create uWebSockets as an INTERFACE library (header-only)
  add_library(uWebSockets INTERFACE)
  target_include_directories(uWebSockets INTERFACE ${uWebSockets_SOURCE_DIR}/src)
  target_link_libraries(uWebSockets INTERFACE uSockets ${ZLIB_LIBRARIES})
  
  # Suppress deprecation warnings from uWebSockets
  if(NOT MSVC)
    target_compile_options(uWebSockets INTERFACE -Wno-deprecated-declarations)
  endif()
endif()

# --- readerwriterqueue (Lock-free SPSC queue) ---
CPMAddPackage(
  NAME readerwriterqueue
  GITHUB_REPOSITORY cameron314/readerwriterqueue
  VERSION 1.0.7
)

# --- stb (Image encoding/decoding, header-only) ---
# Note: stb doesn't use versioned releases, master branch is the standard practice
CPMAddPackage(
  NAME stb
  GITHUB_REPOSITORY nothings/stb
  GIT_TAG master
  DOWNLOAD_ONLY YES
)

if(stb_ADDED)
  add_library(Stb INTERFACE)
  target_include_directories(Stb INTERFACE ${stb_SOURCE_DIR})
  add_library(Stb::Stb ALIAS Stb)
endif()

# ============================================================================
# Core Library Target
# ============================================================================
add_library(dmcp STATIC
  src/dmcp.cpp
  src/core/pool.cpp
  src/core/png.cpp
  src/core/server.cpp
)
add_library(dmcp::dmcp ALIAS dmcp)

# --- Compiler Settings ---
target_compile_features(dmcp PUBLIC cxx_std_17)
set_target_properties(dmcp PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

if(MSVC)
  target_compile_options(dmcp PRIVATE /W4 /permissive-)
else()
  target_compile_options(dmcp PRIVATE 
    -Wall -Wextra -Wpedantic 
    -Wconversion -Wshadow
    -Wunused -Wcast-align
  )
endif()

# Sanitizers (optional)
if(DMCP_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(dmcp PRIVATE -fsanitize=address,undefined)
  target_link_options(dmcp PRIVATE -fsanitize=address,undefined)
endif()

# --- Include Directories ---
target_include_directories(dmcp PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_include_directories(dmcp PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  # uWebSockets and uSockets headers are provided via uWebSockets INTERFACE library
)

# --- Linking ---
# All dependencies are PRIVATE because the public C API (dmcp.h) doesn't expose any C++ types
target_link_libraries(dmcp PRIVATE
  nlohmann_json::nlohmann_json
  uWebSockets  # Transitively links uSockets and ZLIB
  readerwriterqueue
  Stb::Stb
)

# ============================================================================
# Adapters (Optional)
# ============================================================================
if(DMCP_BUILD_ADAPTERS)
  add_subdirectory(adapters)
endif()

# ============================================================================
# Examples
# ============================================================================
if(DMCP_BUILD_EXAMPLES AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_executable(dummy_server examples/dummy_server.cpp)
  target_link_libraries(dummy_server PRIVATE dmcp::dmcp)
  
  # Example needs access to internal headers (schema.hpp) for snapshot population
  target_include_directories(dummy_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  
  # Apply sanitizers to example if enabled
  if(DMCP_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(dummy_server PRIVATE -fsanitize=address,undefined)
    target_link_options(dummy_server PRIVATE -fsanitize=address,undefined)
  endif()
endif()