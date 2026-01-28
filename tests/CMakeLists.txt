cmake_minimum_required(VERSION 3.25)

# ============================================================================
# Test Executable
# ============================================================================
set(TEST_SOURCES
  test_main.cpp
  test_api.cpp
  test_doom.cpp
)

# Add adapter tests only if adapter is being built
if(DMCP_BUILD_ADAPTER_ZDOOM)
  list(APPEND TEST_SOURCES test_adapter.cpp)
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

# Catch2 configuration
target_compile_features(dmcp_tests PRIVATE cxx_std_17)

# Compiler warnings for tests
if(NOT MSVC)
  target_compile_options(dmcp_tests PRIVATE
    -Wall -Wextra -Wpedantic
  )
endif()

# ============================================================================
# Register tests with CTest
# ============================================================================
include(CTest)
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(dmcp_tests)
