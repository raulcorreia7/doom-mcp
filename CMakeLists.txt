cmake_minimum_required(VERSION 3.25)
project(dmcp LANGUAGES C CXX VERSION 0.6.0)

option(DMCP_BUILD_TESTS "Build tests" OFF)
option(DMCP_BUILD_EXAMPLES "Build example servers" ON)
option(DMCP_BUILD_ADAPTER_ZDOOM "Build ZDoom adapter" OFF)
option(DMCP_BUILD_ADAPTER_CHOCOLATE "Build Chocolate Doom adapter" OFF)
option(DMCP_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
option(DMCP_USE_CCACHE "Use ccache for faster rebuilds" ON)

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

file(GLOB MCP_GENERIC_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/mcp/*.cpp"
)

file(GLOB MCP_JSON_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/mcp/json/*.cpp"
)

file(GLOB DMCP_CORE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/doom/*.cpp"
)

file(GLOB DMCP_ZDOOM_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/adapters/zdoom/*.cpp"
)

file(GLOB DMCP_CHOCOLATE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/adapters/chocolate-doom/*.c"
)

add_library(mcp_generic STATIC 
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
  PUBLIC yyjson uWebSockets
)

target_compile_definitions(mcp_generic
  PUBLIC
    DMCP_USE_YYJSON
    MCP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    MCP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    MCP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

set_target_properties(mcp_generic PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

dmcp_enable_warnings(mcp_generic)

add_library(dmcp_core STATIC ${DMCP_CORE_SOURCES})
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

set_target_properties(dmcp_core PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

dmcp_enable_warnings(dmcp_core)

if(DMCP_BUILD_ADAPTER_ZDOOM)
  add_library(dmcp_zdoom STATIC ${DMCP_ZDOOM_SOURCES})
  add_library(dmcp::zdoom ALIAS dmcp_zdoom)
  
  target_include_directories(dmcp_zdoom
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/adapters/zdoom>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
      ${DMCP_ZDOOM_INCLUDE_DIRS}
  )
  
  target_link_libraries(dmcp_zdoom PUBLIC dmcp::core)
  
  if(NOT MSVC)
    target_compile_options(dmcp_zdoom PRIVATE -w)
  endif()
endif()

if(DMCP_BUILD_ADAPTER_CHOCOLATE)
  add_library(dmcp_chocolate STATIC ${DMCP_CHOCOLATE_SOURCES})
  add_library(dmcp::chocolate ALIAS dmcp_chocolate)
  
  target_include_directories(dmcp_chocolate
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/adapters/chocolate-doom>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/chocolate-doom/src
      ${CMAKE_CURRENT_SOURCE_DIR}/chocolate-doom/src/doom
  )
  
  target_link_libraries(dmcp_chocolate PUBLIC dmcp::core)
  
  target_compile_definitions(dmcp_chocolate PRIVATE CHOCOLATE_DOOM_BUILD)
  
  dmcp_enable_warnings(dmcp_chocolate)
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

message(STATUS "")
message(STATUS "DMCP ${PROJECT_VERSION}")
message(STATUS "  Examples: ${DMCP_BUILD_EXAMPLES}")
message(STATUS "  Tests: ${DMCP_BUILD_TESTS}")
message(STATUS "  ZDoom Adapter: ${DMCP_BUILD_ADAPTER_ZDOOM}")
message(STATUS "  Sanitizers: ${DMCP_ENABLE_SANITIZERS}")
message(STATUS "")
