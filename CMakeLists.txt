cmake_minimum_required(VERSION 3.25)
project(dmcp LANGUAGES C CXX VERSION 0.6.0)

option(DMCP_BUILD_TESTS "Build tests" OFF)
option(DMCP_BUILD_EXAMPLES "Build example servers" ON)
option(DMCP_BUILD_INTEGRATION_TESTS "Build integration tests" OFF)
option(DMCP_BUILD_ADAPTER_ZDOOM "Build ZDoom adapter" OFF)
option(DMCP_BUILD_ADAPTER_CHOCOLATE "Build Chocolate Doom adapter" OFF)
option(DMCP_BUILD_SHARED "Build shared libraries (.so/.dll)" OFF)
option(DMCP_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
option(DMCP_USE_CCACHE "Use ccache for faster rebuilds" ON)
option(DMCP_INSTALL "Install headers and CMake package" ON)

set(DMCP_ZDOOM_INCLUDE_DIRS "" CACHE STRING "Semicolon-separated include directories for ZDoom SDK headers")
set(DMCP_CHOCOLATE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/chocolate-doom/src" CACHE PATH "Chocolate Doom source include directory")
set(DMCP_CHOCOLATE_DOOM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/chocolate-doom/src/doom" CACHE PATH "Chocolate Doom doom include directory")
set(DMCP_CHOCOLATE_GEN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/chocolate-doom/build" CACHE PATH "Chocolate Doom generated headers directory (must contain config.h)")

set(DMCP_LIBRARY_TYPE STATIC)
if(DMCP_BUILD_SHARED)
  set(DMCP_LIBRARY_TYPE SHARED)
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
  src/doom/context.cpp
  src/doom/mcp_handlers.cpp
  src/doom/pool.cpp
  src/doom/screenshot.cpp
  src/doom/serialization.cpp
)

add_library(mcp_generic ${DMCP_LIBRARY_TYPE}
  ${MCP_GENERIC_SOURCES}
  ${MCP_JSON_SOURCES}
)
add_library(mcp::generic ALIAS mcp_generic)

target_include_directories(mcp_generic
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(mcp_generic
  PRIVATE yyjson uWebSockets
)

target_compile_definitions(mcp_generic
  PUBLIC
    DMCP_USE_YYJSON
    MCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    MCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    MCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Always use hidden visibility
set_target_properties(mcp_generic PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  POSITION_INDEPENDENT_CODE ON
)

if(DMCP_BUILD_SHARED AND WIN32)
  set_target_properties(mcp_generic PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Define BUILDING macro when compiling the library
target_compile_definitions(mcp_generic PRIVATE MCP_BUILDING)

dmcp_enable_warnings(mcp_generic)

add_library(dmcp_core ${DMCP_LIBRARY_TYPE} ${DMCP_CORE_SOURCES})
add_library(dmcp::core ALIAS dmcp_core)

target_include_directories(dmcp_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(dmcp_core PUBLIC mcp::generic)

target_compile_definitions(dmcp_core
  PUBLIC
    DMCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    DMCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    DMCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Always use hidden visibility
set_target_properties(dmcp_core PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  POSITION_INDEPENDENT_CODE ON
)

if(DMCP_BUILD_SHARED AND WIN32)
  set_target_properties(dmcp_core PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Define BUILDING macro when compiling the library
target_compile_definitions(dmcp_core PRIVATE DMCP_BUILDING)

dmcp_enable_warnings(dmcp_core)

if(DMCP_BUILD_ADAPTER_ZDOOM)
  add_subdirectory(adapters/zdoom)
endif()

if(DMCP_BUILD_ADAPTER_CHOCOLATE)
  add_subdirectory(adapters/chocolate-doom)
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

dmcp_enable_sanitizers(mcp_generic)
dmcp_enable_sanitizers(dmcp_core)

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

  # Install targets and create export (only for shared builds)
  if(DMCP_BUILD_SHARED)
    install(
      TARGETS mcp_generic dmcp_core
      EXPORT dmcp-targets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    if(TARGET dmcp_zdoom)
      install(
        TARGETS dmcp_zdoom
        EXPORT dmcp-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      )
    endif()

    if(TARGET dmcp_chocolate)
      install(
        TARGETS dmcp_chocolate
        EXPORT dmcp-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      )
    endif()
  endif()

  # Export targets and install package config (only for shared builds)
  if(DMCP_BUILD_SHARED)
    # Install export file
    install(
      EXPORT dmcp-targets
      FILE dmcp-targets.cmake
      NAMESPACE dmcp::
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
    )

    # Generate and install package config
    configure_package_config_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmcp-config.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/dmcp-config.cmake
      INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
    )

    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/dmcp-config.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
    )
  endif()
endif()

message(STATUS "")
message(STATUS "DMCP ${PROJECT_VERSION}")
message(STATUS "  Examples: ${DMCP_BUILD_EXAMPLES}")
message(STATUS "  Tests: ${DMCP_BUILD_TESTS}")
message(STATUS "  Integration tests: ${DMCP_BUILD_INTEGRATION_TESTS}")
message(STATUS "  ZDoom Adapter: ${DMCP_BUILD_ADAPTER_ZDOOM}")
message(STATUS "  Chocolate Adapter: ${DMCP_BUILD_ADAPTER_CHOCOLATE}")
message(STATUS "  Shared libs: ${DMCP_BUILD_SHARED}")
message(STATUS "  Install package: ${DMCP_INSTALL}")
message(STATUS "  Sanitizers: ${DMCP_ENABLE_SANITIZERS}")
message(STATUS "")
