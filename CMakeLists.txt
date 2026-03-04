cmake_minimum_required(VERSION 3.25)
project(dmcp LANGUAGES C CXX VERSION 0.6.0)

option(DMCP_BUILD_TESTS             "Build tests"                                       OFF)
option(DMCP_BUILD_EXAMPLES          "Build example servers"                             OFF)
option(DMCP_BUILD_INTEGRATION_TESTS "Build integration tests"                           OFF)
option(DMCP_BUILD_ADAPTERS          "Enable bundled adapter projects"                   OFF)
option(DMCP_BUILD_ADAPTER_FAKE      "Build fake adapter (smoke/tests)"                  ${DMCP_BUILD_ADAPTERS})
option(DMCP_BUILD_ADAPTER_ZDOOM     "Build ZDoom adapter"                               ${DMCP_BUILD_ADAPTERS})
option(DMCP_BUILD_ADAPTER_CRISPY    "Build Crispy Doom adapter"                         ${DMCP_BUILD_ADAPTERS})
option(DMCP_BUILD_SHARED            "Build shared libraries (.so/.dll)"                 OFF)
option(DMCP_BUILD_SINGLE_DLL        "Build unified libdmcp artifact (dmcp.so/dmcp.dll)" ON)
option(DMCP_ENABLE_SANITIZERS       "Enable AddressSanitizer and UBSan"                 OFF)
option(DMCP_USE_CCACHE              "Use ccache for faster rebuilds"                    ON)
option(DMCP_INSTALL                 "Install headers and CMake package"                 ON)

set(DMCP_ZDOOM_INCLUDE_DIRS "" CACHE STRING "Semicolon-separated include directories for ZDoom SDK headers")
set(DMCP_CRISPY_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/crispy-doom/src" CACHE PATH
    "Crispy Doom source include directory")
set(DMCP_CRISPY_DOOM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/crispy-doom/src/doom" CACHE PATH
    "Crispy Doom doom include directory")
set(DMCP_CRISPY_GEN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/crispy-doom/build" CACHE PATH
    "Crispy Doom generated headers directory (must contain config.h)")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(DMCP_USE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CPM)
include(Dependencies)
include(CompilerWarnings)
include(Sanitizers)

# ------------------------------------------------------------------------------
# Derived build layout + helper functions
# ------------------------------------------------------------------------------
include(DMCPBuildConfig)

# ------------------------------------------------------------------------------
# Source discovery
# ------------------------------------------------------------------------------
include(DMCPSourceLists)

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

target_compile_definitions(dmcp_generic_objs
  PUBLIC
    DMCP_USE_YYJSON
    MCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    MCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    MCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

set_target_properties(dmcp_generic_objs PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  POSITION_INDEPENDENT_CODE ON
)

if(DMCP_BUILD_SHARED)
  target_compile_definitions(dmcp_generic_objs PRIVATE MCP_BUILDING)
endif()

target_link_libraries(dmcp_generic_objs
  PRIVATE yyjson
)

dmcp_enable_warnings(dmcp_generic_objs)

if(TARGET httplib::httplib)
  get_target_property(DMCP_HTTPLIB_INCLUDE_DIRS httplib::httplib INTERFACE_INCLUDE_DIRECTORIES)
  if(DMCP_HTTPLIB_INCLUDE_DIRS)
    target_include_directories(dmcp_generic_objs SYSTEM PRIVATE ${DMCP_HTTPLIB_INCLUDE_DIRS})
  endif()
endif()

if(DMCP_BUILD_SHARED_SPLIT)
  add_library(generic SHARED $<TARGET_OBJECTS:dmcp_generic_objs>)
  target_link_libraries(generic PRIVATE yyjson)
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
      MCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
      MCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
      MCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
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
  add_library(generic_static STATIC $<TARGET_OBJECTS:dmcp_generic_objs>)
  target_link_libraries(generic_static PRIVATE yyjson)
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
      MCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
      MCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
      MCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
      MCP_STATIC
  )
  set_target_properties(generic_static PROPERTIES
    OUTPUT_NAME dmcp_generic
    POSITION_INDEPENDENT_CODE ON
  )
  dmcp_enable_warnings(generic_static)
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

target_link_libraries(dmcp_core_objs PUBLIC dmcp_generic_objs PRIVATE yyjson)

target_compile_definitions(dmcp_core_objs
  PUBLIC
    DMCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    DMCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    DMCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

set_target_properties(dmcp_core_objs PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  POSITION_INDEPENDENT_CODE ON
)

if(DMCP_BUILD_SHARED)
  # Core links generic_static into the same shared object; force direct
  # symbol references instead of importing mcp symbols from dmcp_generic.dll.
  target_compile_definitions(dmcp_core_objs PRIVATE DMCP_BUILDING MCP_STATIC)
endif()

dmcp_enable_warnings(dmcp_core_objs)

if(DMCP_BUILD_SHARED_SPLIT)
  add_library(core SHARED $<TARGET_OBJECTS:dmcp_core_objs>)
  target_link_libraries(core PRIVATE generic_static yyjson)
  target_include_directories(core
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(core
    PUBLIC
      DMCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
      DMCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
      DMCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
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
  target_link_libraries(core_static PUBLIC generic_static)
  target_include_directories(core_static
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(core_static
    PUBLIC
      DMCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
      DMCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
      DMCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
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
      $<TARGET_OBJECTS:dmcp_generic_objs>
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
        MCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        MCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        MCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        DMCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        DMCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        DMCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
      PRIVATE
        MCP_BUILDING
        DMCP_BUILDING
    )
    set_target_properties(single PROPERTIES
      CXX_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN ON
      POSITION_INDEPENDENT_CODE ON
      OUTPUT_NAME dmcp
    )
    dmcp_enable_warnings(single)
  endif()

  if(DMCP_BUILD_SINGLE_STATIC)
    add_library(single_static STATIC
      $<TARGET_OBJECTS:dmcp_generic_objs>
      $<TARGET_OBJECTS:dmcp_core_objs>
    )
    if(NOT TARGET dmcp::single)
      add_library(dmcp::single ALIAS single_static)
    endif()
    target_link_libraries(single_static PRIVATE yyjson)
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
        MCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        MCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        MCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        DMCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        DMCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        DMCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
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
foreach(dmcp_sanitize_target IN ITEMS generic core single generic_static core_static single_static)
  if(TARGET ${dmcp_sanitize_target})
    dmcp_enable_sanitizers(${dmcp_sanitize_target})
  endif()
endforeach()

if(DMCP_ENABLE_SANITIZERS AND DMCP_BUILD_EXAMPLES AND NOT MSVC)
  target_compile_options(dummy_server PRIVATE -fsanitize=address,undefined)
  target_link_options(dummy_server PRIVATE -fsanitize=address,undefined)
endif()

# ------------------------------------------------------------------------------
# Install/export package configuration
# ------------------------------------------------------------------------------
if(DMCP_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  set(dmcp_install_targets "")
  if(DMCP_BUILD_SHARED)
    dmcp_append_existing_targets(dmcp_install_targets single generic core)
  else()
    dmcp_append_existing_targets(dmcp_install_targets single_static core_static generic_static)
  endif()
  dmcp_append_existing_targets(dmcp_install_targets adapter_crispy adapter_fake adapter_zdoom)

  foreach(dmcp_target IN LISTS dmcp_install_targets)
    install(
      TARGETS ${dmcp_target}
      EXPORT dmcp-targets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endforeach()

  install(
    EXPORT dmcp-targets
    FILE dmcp-targets.cmake
    NAMESPACE dmcp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
  )

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmcp-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/dmcp-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
  )

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/dmcp-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/dmcp-config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/dmcp-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
  )
endif()

# Export build-tree targets for static-only local consumers.
# Shared builds are consumed from install tree.
if(NOT DMCP_BUILD_SHARED)
  if(TARGET generic_static)
    export(TARGETS generic_static FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  endif()
  if(TARGET core_static)
    export(TARGETS core_static FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  endif()
  if(TARGET single_static)
    export(TARGETS single_static FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  endif()
  if(TARGET adapter_zdoom)
    export(TARGETS adapter_zdoom FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  endif()
  if(TARGET adapter_crispy)
    export(TARGETS adapter_crispy FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  endif()
  if(TARGET adapter_fake)
    export(TARGETS adapter_fake FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  endif()
endif()

message(STATUS "")
message(STATUS "DMCP ${PROJECT_VERSION}")
message(STATUS "  Examples: ${DMCP_BUILD_EXAMPLES}")
message(STATUS "  Tests: ${DMCP_BUILD_TESTS}")
message(STATUS "  Integration tests: ${DMCP_BUILD_INTEGRATION_TESTS}")
message(STATUS "  Bundled adapters: ${DMCP_BUILD_ADAPTERS}")
message(STATUS "  Fake Adapter: ${DMCP_BUILD_ADAPTER_FAKE}")
message(STATUS "  ZDoom Adapter: ${DMCP_BUILD_ADAPTER_ZDOOM}")
message(STATUS "  Crispy Adapter: ${DMCP_BUILD_ADAPTER_CRISPY}")
message(STATUS "  Shared libs: ${DMCP_BUILD_SHARED}")
message(STATUS "  Single libdmcp: ${DMCP_BUILD_SINGLE_DLL}")
if(DMCP_BUILD_SHARED)
  if(DMCP_BUILD_SHARED_SINGLE)
    message(STATUS "  Shared layout: single-surface (libdmcp only)")
  else()
    message(STATUS "  Shared layout: split (libdmcp_generic + libdmcp_core)")
  endif()
endif()
message(STATUS "  Install package: ${DMCP_INSTALL}")
message(STATUS "  Sanitizers: ${DMCP_ENABLE_SANITIZERS}")
message(STATUS "")
