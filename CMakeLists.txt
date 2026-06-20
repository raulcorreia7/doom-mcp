cmake_minimum_required(VERSION 3.25)
project(dmcp LANGUAGES C CXX VERSION 0.7.2)

option(DMCP_BUILD_TESTS             "Build tests"                                       OFF)
option(DMCP_BUILD_EXAMPLES          "Build example servers"                             OFF)
option(DMCP_BUILD_INTEGRATION_TESTS "Build integration tests"                           OFF)
option(DMCP_BUILD_ADAPTER_FAKE      "Build fake adapter (smoke/tests)"                  OFF)
option(DMCP_BUILD_ADAPTER_ZDOOM     "Build zdoom adapter"                               OFF)
option(DMCP_BUILD_ADAPTER_CRISPY    "Build crispy-doom adapter"                         OFF)
option(DMCP_CRISPY_ENABLE_FRAME_CAPTURE "Enable crispy-doom framebuffer capture"         OFF)
option(DMCP_BUILD_SHARED            "Build shared libraries (.so/.dll)"                 OFF)
option(DMCP_BUILD_SINGLE_DLL        "Build unified libdmcp artifact (dmcp.so/dmcp.dll)" ON)
option(DMCP_ENABLE_SANITIZERS       "Enable AddressSanitizer and UBSan"                 OFF)
option(DMCP_USE_CCACHE              "Use ccache for faster rebuilds"                    ON)

set(DMCP_ZDOOM_INCLUDE_DIRS "" CACHE STRING "Semicolon-separated include directories for zdoom adapter headers")
set(DMCP_CRISPY_SRC_DIR "" CACHE PATH
    "crispy-doom source include directory")
set(DMCP_CRISPY_DOOM_DIR "" CACHE PATH
    "crispy-doom doom include directory")
set(DMCP_CRISPY_GEN_INCLUDE_DIR "" CACHE PATH
    "crispy-doom generated headers directory (must contain config.h)")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(DMCP_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/mcp_core_version_config.h.in"
  "${DMCP_GENERATED_INCLUDE_DIR}/mcp/core/version_config.h"
  @ONLY
)

if(DMCP_USE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CPM)
include(Dependencies)
include(CompilerWarnings)
include(Sanitizers)

if(TARGET yyjson)
  get_target_property(DMCP_YYJSON_SOURCE_DIR yyjson SOURCE_DIR)
  get_target_property(DMCP_YYJSON_INCLUDE_DIRS yyjson INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT DMCP_YYJSON_INCLUDE_DIRS)
    set(DMCP_YYJSON_INCLUDE_DIRS "${DMCP_YYJSON_SOURCE_DIR}/src")
  endif()
  set(DMCP_YYJSON_SOURCE "${DMCP_YYJSON_SOURCE_DIR}/src/yyjson.c")
  if(NOT EXISTS "${DMCP_YYJSON_SOURCE}")
    message(FATAL_ERROR "yyjson source not found: ${DMCP_YYJSON_SOURCE}")
  endif()
endif()

# ------------------------------------------------------------------------------
# Derived build layout + helper functions
# ------------------------------------------------------------------------------
include(DMCPBuildConfig)

if(NOT DMCP_BUILD_SHARED)
  add_compile_definitions(MCP_CORE_STATIC MCP_STATIC MCP_GAME_STATIC DMCP_STATIC)
endif()

# ------------------------------------------------------------------------------
# Source discovery
# ------------------------------------------------------------------------------
include(DMCPSourceLists)

# ------------------------------------------------------------------------------
# MCP core foundation (version + API + status contracts)
# ------------------------------------------------------------------------------
add_library(dmcp_core_foundation_objs OBJECT
  ${MCP_CORE_FOUNDATION_SOURCES}
)

target_include_directories(dmcp_core_foundation_objs
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

set_target_properties(dmcp_core_foundation_objs PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  POSITION_INDEPENDENT_CODE ON
)

if(DMCP_BUILD_SHARED)
  target_compile_definitions(dmcp_core_foundation_objs PRIVATE MCP_CORE_BUILDING)
endif()

dmcp_enable_warnings(dmcp_core_foundation_objs)

if(DMCP_BUILD_SHARED_SPLIT)
  add_library(core_foundation SHARED $<TARGET_OBJECTS:dmcp_core_foundation_objs>)
  target_include_directories(core_foundation
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_compile_definitions(core_foundation
    PRIVATE
      MCP_CORE_BUILDING
  )
  set_target_properties(core_foundation PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME mcp_core
  )
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_options(core_foundation PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmcp_core_foundation.version"
    )
  endif()
  dmcp_enable_warnings(core_foundation)
endif()

if(NOT DMCP_BUILD_SHARED OR DMCP_BUILD_STATIC_FOR_TESTS OR DMCP_BUILD_SHARED_SPLIT)
  add_library(core_foundation_static STATIC $<TARGET_OBJECTS:dmcp_core_foundation_objs>)
  target_include_directories(core_foundation_static
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_compile_definitions(core_foundation_static
    PUBLIC
      MCP_CORE_STATIC
  )
  set_target_properties(core_foundation_static PROPERTIES
    OUTPUT_NAME mcp_core
    POSITION_INDEPENDENT_CODE ON
  )
  dmcp_enable_warnings(core_foundation_static)
endif()

# ------------------------------------------------------------------------------
# Generic MCP layer (protocol + HTTP/SSE transport)
# ------------------------------------------------------------------------------
add_library(dmcp_generic_objs OBJECT
  ${MCP_GENERIC_SOURCES}
)

target_include_directories(dmcp_generic_objs
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
if(DMCP_YYJSON_INCLUDE_DIRS)
  target_include_directories(dmcp_generic_objs SYSTEM PRIVATE ${DMCP_YYJSON_INCLUDE_DIRS})
endif()

target_compile_definitions(dmcp_generic_objs
  PUBLIC
    DMCP_USE_YYJSON
)

set_target_properties(dmcp_generic_objs PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  POSITION_INDEPENDENT_CODE ON
)

if(DMCP_BUILD_SHARED)
  target_compile_definitions(dmcp_generic_objs PRIVATE MCP_BUILDING)
endif()

dmcp_enable_warnings(dmcp_generic_objs)

if(TARGET httplib::httplib)
  get_target_property(DMCP_HTTPLIB_INCLUDE_DIRS httplib::httplib INTERFACE_INCLUDE_DIRECTORIES)
  if(DMCP_HTTPLIB_INCLUDE_DIRS)
    target_include_directories(dmcp_generic_objs SYSTEM PRIVATE ${DMCP_HTTPLIB_INCLUDE_DIRS})
  endif()
endif()

if(DMCP_BUILD_SHARED_SPLIT)
  add_library(generic SHARED $<TARGET_OBJECTS:dmcp_generic_objs>)
  target_link_libraries(generic PUBLIC core_foundation PRIVATE yyjson)
  target_include_directories(generic
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(generic
    PUBLIC
      DMCP_USE_YYJSON
    PRIVATE
      MCP_BUILDING
  )
  set_target_properties(generic PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME dmcp_generic
  )
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_options(generic PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmcp_generic.version"
    )
  endif()
  dmcp_enable_warnings(generic)
endif()

if(DMCP_BUILD_GENERIC_STATIC)
  add_library(generic_static STATIC
    $<TARGET_OBJECTS:dmcp_generic_objs>
    ${DMCP_YYJSON_SOURCE}
  )
  target_link_libraries(generic_static PUBLIC core_foundation_static)
  if(DMCP_YYJSON_INCLUDE_DIRS)
    target_include_directories(generic_static SYSTEM PRIVATE ${DMCP_YYJSON_INCLUDE_DIRS})
  endif()
  target_include_directories(generic_static
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(generic_static
    PUBLIC
      DMCP_USE_YYJSON
      MCP_STATIC
  )
  set_target_properties(generic_static PROPERTIES
    OUTPUT_NAME dmcp_generic
    POSITION_INDEPENDENT_CODE ON
  )
  dmcp_enable_warnings(generic_static)
endif()

# ------------------------------------------------------------------------------
# Generic game integration layer (game-agnostic registration helpers)
# ------------------------------------------------------------------------------
add_library(dmcp_game_objs OBJECT
  ${MCP_GAME_SOURCES}
)

target_include_directories(dmcp_game_objs
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(dmcp_game_objs PUBLIC dmcp_generic_objs)

set_target_properties(dmcp_game_objs PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  POSITION_INDEPENDENT_CODE ON
)

if(DMCP_BUILD_SHARED)
  target_compile_definitions(dmcp_game_objs PRIVATE MCP_GAME_BUILDING)
endif()

dmcp_enable_warnings(dmcp_game_objs)

if(DMCP_BUILD_SHARED_SPLIT)
  add_library(game SHARED $<TARGET_OBJECTS:dmcp_game_objs>)
  target_link_libraries(game PUBLIC generic)
  target_include_directories(game
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(game PRIVATE MCP_GAME_BUILDING)
  set_target_properties(game PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME mcp_game
  )
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_options(game PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmcp_game.version"
    )
  endif()
  dmcp_enable_warnings(game)
endif()

if(DMCP_BUILD_GENERIC_STATIC)
  add_library(game_static STATIC $<TARGET_OBJECTS:dmcp_game_objs>)
  target_link_libraries(game_static PUBLIC generic_static)
  target_include_directories(game_static
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(game_static PUBLIC MCP_GAME_STATIC)
  set_target_properties(game_static PROPERTIES
    OUTPUT_NAME mcp_game
    POSITION_INDEPENDENT_CODE ON
  )
  dmcp_enable_warnings(game_static)
endif()

# ------------------------------------------------------------------------------
# Doom MCP layer (game-specific handlers and orchestration)
# ------------------------------------------------------------------------------
add_library(dmcp_core_objs OBJECT ${DMCP_CORE_SOURCES})

target_include_directories(dmcp_core_objs
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(dmcp_core_objs PUBLIC dmcp_game_objs)

set_target_properties(dmcp_core_objs PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  POSITION_INDEPENDENT_CODE ON
)

if(DMCP_BUILD_SHARED_SINGLE)
  # The single shared runtime embeds the lower MCP layers in libdmcp.
  target_compile_definitions(dmcp_core_objs PRIVATE DMCP_BUILDING MCP_STATIC)
elseif(DMCP_BUILD_SHARED_SPLIT)
  target_compile_definitions(dmcp_core_objs PRIVATE DMCP_BUILDING)
endif()

dmcp_enable_warnings(dmcp_core_objs)

if(DMCP_BUILD_SHARED_SPLIT)
  add_library(core SHARED $<TARGET_OBJECTS:dmcp_core_objs>)
  target_link_libraries(core PUBLIC game)
  target_include_directories(core
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(core
    PRIVATE
      DMCP_BUILDING
  )
  set_target_properties(core PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME dmcp_core
  )
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_options(core PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmcp_core.version"
    )
  endif()
  dmcp_enable_warnings(core)
endif()

if(DMCP_BUILD_CORE_STATIC)
  add_library(core_static STATIC $<TARGET_OBJECTS:dmcp_core_objs>)
  target_link_libraries(core_static PUBLIC game_static)
  target_include_directories(core_static
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(core_static
    PUBLIC
      DMCP_STATIC
  )
  set_target_properties(core_static PROPERTIES
    OUTPUT_NAME dmcp_core
    POSITION_INDEPENDENT_CODE ON
  )
  dmcp_enable_warnings(core_static)
endif()

# ------------------------------------------------------------------------------
# Unified runtime surface (single libdmcp)
# ------------------------------------------------------------------------------
if(DMCP_BUILD_SINGLE_DLL)
  if(DMCP_BUILD_SHARED_SINGLE)
    add_library(single SHARED
      $<TARGET_OBJECTS:dmcp_core_foundation_objs>
      $<TARGET_OBJECTS:dmcp_generic_objs>
      $<TARGET_OBJECTS:dmcp_game_objs>
      $<TARGET_OBJECTS:dmcp_core_objs>
    )
    add_library(dmcp::single ALIAS single)
    target_link_libraries(single PRIVATE yyjson)
    target_include_directories(single
      PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
      PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_compile_definitions(single
      PUBLIC
        DMCP_USE_YYJSON
      PRIVATE
        MCP_CORE_BUILDING
        MCP_BUILDING
        DMCP_BUILDING
    )
    set_target_properties(single PROPERTIES
      CXX_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN ON
      POSITION_INDEPENDENT_CODE ON
      OUTPUT_NAME dmcp
    )
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
      target_link_options(single PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmcp_single.version"
      )
    endif()
    dmcp_enable_warnings(single)
  endif()

  if(DMCP_BUILD_SINGLE_STATIC)
    add_library(single_static STATIC
      $<TARGET_OBJECTS:dmcp_core_foundation_objs>
      $<TARGET_OBJECTS:dmcp_generic_objs>
      $<TARGET_OBJECTS:dmcp_game_objs>
      $<TARGET_OBJECTS:dmcp_core_objs>
      ${DMCP_YYJSON_SOURCE}
    )
    if(NOT TARGET dmcp::single)
      add_library(dmcp::single ALIAS single_static)
    endif()
    if(DMCP_YYJSON_INCLUDE_DIRS)
      target_include_directories(single_static SYSTEM PRIVATE ${DMCP_YYJSON_INCLUDE_DIRS})
    endif()
    target_include_directories(single_static
      PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
      PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_compile_definitions(single_static
      PUBLIC
        DMCP_USE_YYJSON
        MCP_CORE_STATIC
        MCP_STATIC
        DMCP_STATIC
    )
    set_target_properties(single_static PROPERTIES
      OUTPUT_NAME dmcp
      POSITION_INDEPENDENT_CODE ON
    )
    dmcp_enable_warnings(single_static)
  endif()
endif()

# Add generated public headers after all core targets exist. BEFORE prevents a
# stale source-tree header from shadowing generated release metadata.
foreach(dmcp_public_target IN ITEMS
    dmcp_core_foundation_objs core_foundation core_foundation_static
    dmcp_generic_objs generic generic_static
    dmcp_game_objs game game_static
    dmcp_core_objs core core_static
    single single_static)
  if(TARGET ${dmcp_public_target})
    target_include_directories(${dmcp_public_target} BEFORE PUBLIC
      $<BUILD_INTERFACE:${DMCP_GENERATED_INCLUDE_DIR}>
    )
  endif()
endforeach()

# ------------------------------------------------------------------------------
# Public target aliases
# ------------------------------------------------------------------------------
if(DMCP_BUILD_SHARED)
  if(DMCP_BUILD_SINGLE_DLL)
    add_library(dmcp::generic ALIAS single)
    add_library(dmcp::core ALIAS single)
  else()
    add_library(dmcp::generic ALIAS generic)
    add_library(dmcp::core ALIAS core)
  endif()
else()
  add_library(dmcp::generic ALIAS generic_static)
  add_library(dmcp::core ALIAS core_static)
endif()

if(NOT TARGET dmcp::single)
  if(TARGET single)
    add_library(dmcp::single ALIAS single)
  elseif(TARGET single_static)
    add_library(dmcp::single ALIAS single_static)
  elseif(TARGET core)
    add_library(dmcp::single ALIAS core)
  elseif(TARGET core_static)
    add_library(dmcp::single ALIAS core_static)
  endif()
endif()

if(DMCP_BUILD_SHARED)
  if(DMCP_BUILD_SINGLE_DLL)
    add_library(mcp::core ALIAS single)
    add_library(mcp::generic ALIAS single)
    add_library(mcp::game ALIAS single)
    add_library(dmcp::doom ALIAS single)
    add_library(dmcp::runtime ALIAS single)
  else()
    add_library(mcp::core ALIAS core_foundation)
    add_library(mcp::generic ALIAS generic)
    add_library(mcp::game ALIAS game)
    add_library(dmcp::doom ALIAS core)
    add_library(dmcp::runtime ALIAS core)
  endif()
else()
  add_library(mcp::core ALIAS core_foundation_static)
  add_library(mcp::generic ALIAS generic_static)
  add_library(mcp::game ALIAS game_static)
  add_library(dmcp::doom ALIAS core_static)
  if(TARGET single_static)
    add_library(dmcp::runtime ALIAS single_static)
  else()
    add_library(dmcp::runtime ALIAS core_static)
  endif()
endif()

# ------------------------------------------------------------------------------
# Optional adapters
# ------------------------------------------------------------------------------
set(DMCP_BUILD_ANY_ADAPTER OFF)
if(DMCP_BUILD_ADAPTER_ZDOOM OR DMCP_BUILD_ADAPTER_CRISPY OR DMCP_BUILD_ADAPTER_FAKE)
  set(DMCP_BUILD_ANY_ADAPTER ON)
endif()

if(DMCP_BUILD_ANY_ADAPTER)
  add_subdirectory(adapters/common)

  if(DMCP_BUILD_ADAPTER_ZDOOM)
    add_subdirectory(adapters/zdoom)
  endif()

  if(DMCP_BUILD_ADAPTER_CRISPY)
    add_subdirectory(adapters/crispy-doom)
  endif()

  if(DMCP_BUILD_ADAPTER_FAKE)
    add_subdirectory(adapters/fake)
  endif()
endif()

if(DMCP_BUILD_EXAMPLES AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_executable(dummy_server examples/dummy_server.cpp)
  target_link_libraries(dummy_server PRIVATE dmcp::core)

  if(NOT MSVC)
    target_compile_options(dummy_server PRIVATE -Wall -Wextra)
  endif()
endif()

if(DMCP_BUILD_TESTS AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  enable_testing()
  add_subdirectory(tests)
endif()

# ------------------------------------------------------------------------------
# Sanitizers and executable hardening
# ------------------------------------------------------------------------------
foreach(dmcp_sanitize_target IN ITEMS
    core_foundation generic game core single
    core_foundation_static generic_static game_static core_static single_static)
  if(TARGET ${dmcp_sanitize_target})
    dmcp_enable_sanitizers(${dmcp_sanitize_target})
  endif()
endforeach()

if(DMCP_ENABLE_SANITIZERS AND DMCP_BUILD_EXAMPLES AND NOT MSVC)
  target_compile_options(dummy_server PRIVATE -fsanitize=address,undefined)
  target_link_options(dummy_server PRIVATE -fsanitize=address,undefined)
endif()

message(STATUS "")
message(STATUS "DMCP ${PROJECT_VERSION}")
message(STATUS "  Examples: ${DMCP_BUILD_EXAMPLES}")
message(STATUS "  Tests: ${DMCP_BUILD_TESTS}")
message(STATUS "  Integration tests: ${DMCP_BUILD_INTEGRATION_TESTS}")
message(STATUS "  Fake Adapter: ${DMCP_BUILD_ADAPTER_FAKE}")
message(STATUS "  zdoom adapter: ${DMCP_BUILD_ADAPTER_ZDOOM}")
message(STATUS "  crispy-doom adapter: ${DMCP_BUILD_ADAPTER_CRISPY}")
message(STATUS "  crispy-doom frame capture: ${DMCP_CRISPY_ENABLE_FRAME_CAPTURE}")
message(STATUS "  Shared libs: ${DMCP_BUILD_SHARED}")
message(STATUS "  Single libdmcp: ${DMCP_BUILD_SINGLE_DLL}")
if(DMCP_BUILD_SHARED)
  if(DMCP_BUILD_SHARED_SINGLE)
    message(STATUS "  Shared layout: single-surface (libdmcp only)")
  else()
    message(STATUS "  Shared layout: split (libdmcp_generic + libdmcp_core)")
  endif()
endif()
message(STATUS "  Sanitizers: ${DMCP_ENABLE_SANITIZERS}")
message(STATUS "")
