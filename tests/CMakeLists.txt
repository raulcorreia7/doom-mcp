cmake_minimum_required(VERSION 3.25)

# Unit tests
set(UNIT_TEST_SOURCES
  unit/test_mcp_server.cpp
  unit/test_mcp_protocol.cpp
  unit/test_doom_context.cpp
  unit/test_screenshot.cpp
  unit/test_layer_boundaries.cpp
  unit/test_layers.cpp
)

if(DMCP_BUILD_ADAPTER_ZDOOM)
  list(APPEND UNIT_TEST_SOURCES unit/test_zdoom_adapter.cpp)
endif()

add_executable(dmcp_tests ${UNIT_TEST_SOURCES})

if(DMCP_BUILD_SHARED)
  target_link_libraries(dmcp_tests
    PRIVATE
      core_static
      generic_static
      Catch2::Catch2WithMain
  )
else()
  target_link_libraries(dmcp_tests
    PRIVATE
      dmcp::core
      Catch2::Catch2WithMain
  )
endif()

target_include_directories(dmcp_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(dmcp_tests
  PRIVATE
    DMCP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

if(NOT MSVC)
  target_compile_options(dmcp_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(dmcp_tests)

# Integration tests (optional, requires network and WAD)
if(DMCP_BUILD_INTEGRATION_TESTS)
  # Add integration test subdirectory
  add_subdirectory(integration)
endif()
