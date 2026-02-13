cmake_minimum_required(VERSION 3.25)

set(TEST_SOURCES
  unit/test_mcp_server.cpp
  unit/test_doom_context.cpp
)

if(DMCP_BUILD_ADAPTER_ZDOOM)
  list(APPEND TEST_SOURCES unit/test_zdoom_adapter.cpp)
endif()

add_executable(dmcp_tests ${TEST_SOURCES})

target_link_libraries(dmcp_tests
  PRIVATE
    dmcp::core
    Catch2::Catch2WithMain
)

target_include_directories(dmcp_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(NOT MSVC)
  target_compile_options(dmcp_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(dmcp_tests)
