cmake_minimum_required(VERSION 3.25)
project(dmcp LANGUAGES C CXX VERSION 0.6.0)

option(DMCP_BUILD_TESTS "Build tests" OFF)
option(DMCP_BUILD_EXAMPLES "Build example servers" ON)
option(DMCP_BUILD_INTEGRATION_TESTS "Build integration tests" OFF)
option(DMCP_BUILD_ADAPTER_FAKE "Build fake adapter (smoke/tests)" ON)
option(DMCP_BUILD_ADAPTER_ZDOOM "Build ZDoom adapter" OFF)
option(DMCP_BUILD_ADAPTER_CRISPY "Build Crispy Doom adapter" OFF)
option(DMCP_BUILD_SHARED "Build shared libraries (.so/.dll)" OFF)
option(DMCP_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
option(DMCP_USE_CCACHE "Use ccache for faster rebuilds" ON)
option(DMCP_INSTALL "Install headers and CMake package" ON)

set(DMCP_ZDOOM_INCLUDE_DIRS "" CACHE STRING "Semicolon-separated include directories for ZDoom SDK headers")
set(DMCP_CRISPY_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/crispy-doom/src" CACHE PATH "Crispy Doom source include directory")
set(DMCP_CRISPY_DOOM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/crispy-doom/src/doom" CACHE PATH "Crispy Doom doom include directory")
set(DMCP_CRISPY_GEN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/crispy-doom/build" CACHE PATH "Crispy Doom generated headers directory (must contain config.h)")

set(DMCP_LIBRARY_TYPE STATIC)
if(DMCP_BUILD_SHARED)
  set(DMCP_LIBRARY_TYPE SHARED)
endif()

if(DMCP_BUILD_SHARED AND DMCP_BUILD_TESTS)
  set(DMCP_BUILD_STATIC_FOR_TESTS ON)
else()
  set(DMCP_BUILD_STATIC_FOR_TESTS OFF)
endif()

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

set(MCP_GENERIC_SOURCES
  src/mcp/http_sse_transport.cpp
  src/mcp/server.cpp
)

set(MCP_JSON_SOURCES
  src/mcp/json/yyjson.cpp
)

set(DMCP_CORE_SOURCES
  src/doom/commands.cpp
  src/doom/commands/parsers.cpp
  src/doom/content.cpp
  src/doom/context.cpp
  src/doom/layers/registry.cpp
  src/doom/layers/input/input.cpp
  src/doom/pool.cpp
  src/doom/screenshot.cpp
  src/doom/serialization.cpp
  src/doom/commands/types/spawn.cpp
  src/doom/commands/types/change_level.cpp
  src/doom/commands/types/give_item.cpp
  src/doom/commands/types/set_health.cpp
  src/doom/commands/types/set_position.cpp
  src/doom/commands/types/pause.cpp
  src/doom/commands/types/timescale.cpp
  src/doom/commands/types/damage.cpp
  src/doom/commands/types/kill.cpp
  src/doom/commands/types/console.cpp
  src/doom/commands/types/player_input.cpp
  src/doom/handlers/common.cpp
  src/doom/handlers/routes.cpp
  src/doom/handlers/methods.cpp
  src/doom/handlers/tools/get_state_sections.cpp
  src/doom/handlers/tools/get_screenshot.cpp
  src/doom/handlers/tools/execute_command.cpp
  src/doom/handlers/tools/get_command_result.cpp
  src/doom/handlers/tools/get_available_content.cpp
  src/doom/handlers/tools/execute_batch.cpp
  src/doom/handlers/tools/get_command_examples.cpp
  src/doom/handlers/tools/input.cpp
  src/doom/handlers/tools/tools_list.cpp
  src/doom/handlers/state_json.cpp
  src/doom/handlers/schema_builders.cpp
  src/doom/layers/orchestrator/orchestrator.cpp
)

add_library(dmcp_generic_objs OBJECT
  ${MCP_GENERIC_SOURCES}
  ${MCP_JSON_SOURCES}
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

target_link_libraries(dmcp_generic_objs
  PRIVATE yyjson uWebSockets
)

dmcp_enable_warnings(dmcp_generic_objs)

if(DMCP_BUILD_SHARED)
  add_library(generic SHARED $<TARGET_OBJECTS:dmcp_generic_objs>)
  add_library(dmcp::generic ALIAS generic)
  target_link_libraries(generic PRIVATE yyjson uWebSockets)
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

add_library(generic_static STATIC $<TARGET_OBJECTS:dmcp_generic_objs>)
target_link_libraries(generic_static PRIVATE yyjson uWebSockets)
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

if(NOT DMCP_BUILD_SHARED)
  add_library(dmcp::generic ALIAS generic_static)
endif()

add_library(dmcp_core_objs OBJECT ${DMCP_CORE_SOURCES})

target_include_directories(dmcp_core_objs
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(dmcp_core_objs PUBLIC dmcp_generic_objs)

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

dmcp_enable_warnings(dmcp_core_objs)

if(DMCP_BUILD_SHARED)
  add_library(core SHARED $<TARGET_OBJECTS:dmcp_core_objs>)
  add_library(dmcp::core ALIAS core)
  target_link_libraries(core PUBLIC dmcp::generic)
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

if(NOT DMCP_BUILD_SHARED)
  add_library(dmcp::core ALIAS core_static)
endif()

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

if(DMCP_BUILD_SHARED)
  dmcp_enable_sanitizers(generic)
  dmcp_enable_sanitizers(core)
else()
  dmcp_enable_sanitizers(generic_static)
  dmcp_enable_sanitizers(core_static)
endif()

if(DMCP_ENABLE_SANITIZERS AND DMCP_BUILD_EXAMPLES AND NOT MSVC)
  target_compile_options(dummy_server PRIVATE -fsanitize=address,undefined)
  target_link_options(dummy_server PRIVATE -fsanitize=address,undefined)
endif()

if(DMCP_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  if(DMCP_BUILD_SHARED)
    install(
      TARGETS generic core
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  else()
    install(
      TARGETS generic_static core_static
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
  endif()

  if(TARGET crispy)
    install(
      TARGETS crispy
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()

  if(TARGET fake)
    install(
      TARGETS fake
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()

  if(TARGET zdoom)
    install(
      TARGETS zdoom
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()

  if(DMCP_BUILD_SHARED)
    install(
      TARGETS generic core
      EXPORT dmcp-targets
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    if(TARGET zdoom)
      install(
        TARGETS zdoom
        EXPORT dmcp-targets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      )
    endif()

    if(TARGET crispy)
      install(
        TARGETS crispy
        EXPORT dmcp-targets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      )
    endif()

    if(TARGET fake)
      install(
        TARGETS fake
        EXPORT dmcp-targets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      )
    endif()

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
endif()

# For static builds, also export targets to build tree for consumers using find_package
if(NOT DMCP_BUILD_SHARED)
  export(TARGETS generic_static core_static FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  if(TARGET zdoom)
    export(TARGETS zdoom FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  endif()
  if(TARGET crispy)
    export(TARGETS crispy FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  endif()
  if(TARGET fake)
    export(TARGETS fake FILE "${CMAKE_CURRENT_BINARY_DIR}/dmcp-targets.cmake" APPEND)
  endif()
endif()

message(STATUS "")
message(STATUS "DMCP ${PROJECT_VERSION}")
message(STATUS "  Examples: ${DMCP_BUILD_EXAMPLES}")
message(STATUS "  Tests: ${DMCP_BUILD_TESTS}")
message(STATUS "  Integration tests: ${DMCP_BUILD_INTEGRATION_TESTS}")
message(STATUS "  Fake Adapter: ${DMCP_BUILD_ADAPTER_FAKE}")
message(STATUS "  ZDoom Adapter: ${DMCP_BUILD_ADAPTER_ZDOOM}")
message(STATUS "  Crispy Adapter: ${DMCP_BUILD_ADAPTER_CRISPY}")
message(STATUS "  Shared libs: ${DMCP_BUILD_SHARED}")
message(STATUS "  Install package: ${DMCP_INSTALL}")
message(STATUS "  Sanitizers: ${DMCP_ENABLE_SANITIZERS}")
message(STATUS "")
