cmake_minimum_required(VERSION 3.25)

# Unit tests
set(UNIT_TEST_SOURCES
  unit/test_mcp_server.cpp
  unit/test_doom_context.cpp
)

if(DMCP_BUILD_ADAPTER_ZDOOM)
  list(APPEND UNIT_TEST_SOURCES unit/test_zdoom_adapter.cpp)
endif()

add_executable(dmcp_tests ${UNIT_TEST_SOURCES})

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

# Integration tests (optional, requires network)
option(DMCP_BUILD_INTEGRATION_TESTS "Build integration tests" OFF)

if(DMCP_BUILD_INTEGRATION_TESTS)
  add_executable(dmcp_integration_tests
    integration/test_e2e_mcp.cpp
  )

  target_link_libraries(dmcp_integration_tests
    PRIVATE
      dmcp::core
      Catch2::Catch2WithMain
  )

  target_include_directories(dmcp_integration_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  if(NOT MSVC)
    target_compile_options(dmcp_integration_tests PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  catch_discover_tests(dmcp_integration_tests)
endif()
