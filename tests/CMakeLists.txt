cmake_minimum_required(VERSION 3.25)

# Unit tests
dmcp_collect_sources(UNIT_TEST_SOURCES unit)

if(NOT DMCP_BUILD_ADAPTER_ZDOOM)
  list(FILTER UNIT_TEST_SOURCES EXCLUDE REGEX "/test_zdoom_adapter\\.cpp$")
endif()

if(NOT TARGET adapter_fake AND NOT TARGET dmcp::adapter_fake)
  list(FILTER UNIT_TEST_SOURCES EXCLUDE REGEX "/test_fake_adapter\\.cpp$")
endif()

add_executable(dmcp_tests ${UNIT_TEST_SOURCES})

if(DMCP_BUILD_SHARED)
  target_link_libraries(dmcp_tests
    PRIVATE
      core_static
      generic_static
      Catch2::Catch2WithMain
      httplib::httplib
  )
else()
  target_link_libraries(dmcp_tests
    PRIVATE
      dmcp::core
      Catch2::Catch2WithMain
      httplib::httplib
  )
endif()

if(TARGET dmcp::adapter_fake)
  target_link_libraries(dmcp_tests PRIVATE dmcp::adapter_fake)
endif()

if(WIN32)
  target_link_libraries(dmcp_tests PRIVATE ws2_32)
endif()

target_include_directories(dmcp_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)

if(TARGET dmcp_adapter_common)
  target_include_directories(dmcp_tests PRIVATE ${PROJECT_SOURCE_DIR}/adapters/common/include)
  target_link_libraries(dmcp_tests PRIVATE dmcp_adapter_common)
endif()

target_compile_definitions(dmcp_tests
  PRIVATE
    DMCP_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

if(NOT MSVC)
  target_compile_options(dmcp_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

set(DMCP_TEST_RUNTIME_DIRS)
if(WIN32 AND DMCP_BUILD_SHARED)
  if(TARGET single)
    list(APPEND DMCP_TEST_RUNTIME_DIRS "$<TARGET_FILE_DIR:single>")
  endif()
  if(TARGET adapter_fake)
    list(APPEND DMCP_TEST_RUNTIME_DIRS "$<TARGET_FILE_DIR:adapter_fake>")
  endif()
endif()

set(DMCP_TEST_PATH_MODIFICATIONS)
foreach(dmcp_test_runtime_dir IN LISTS DMCP_TEST_RUNTIME_DIRS)
  list(APPEND DMCP_TEST_PATH_MODIFICATIONS "PATH=path_list_prepend:${dmcp_test_runtime_dir}")
endforeach()

function(dmcp_set_test_runtime_path test_name)
  if(DMCP_TEST_PATH_MODIFICATIONS)
    set_tests_properties("${test_name}" PROPERTIES
      ENVIRONMENT_MODIFICATION "${DMCP_TEST_PATH_MODIFICATIONS}"
    )
  endif()
endfunction()

set(DMCP_CATCH_DISCOVERY_ARGS PROPERTIES LABELS "unit")
if(DMCP_TEST_RUNTIME_DIRS)
  list(APPEND DMCP_CATCH_DISCOVERY_ARGS DL_PATHS ${DMCP_TEST_RUNTIME_DIRS})
endif()
catch_discover_tests(dmcp_tests ${DMCP_CATCH_DISCOVERY_ARGS})

add_executable(public_c_headers public_c_headers.c)
target_include_directories(public_c_headers PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(public_c_headers PRIVATE c_std_99)
target_link_libraries(public_c_headers PRIVATE dmcp::core)
if(WIN32)
  target_link_libraries(public_c_headers PRIVATE ws2_32)
endif()
add_test(NAME public_c_headers COMMAND public_c_headers)
set_tests_properties(public_c_headers PROPERTIES LABELS "unit")
dmcp_set_test_runtime_path(public_c_headers)

# Integration tests use the deterministic fake adapter and do not launch engines.
if(DMCP_BUILD_INTEGRATION_TESTS)
  if(NOT TARGET dmcp::adapter_fake)
    message(FATAL_ERROR "DMCP_BUILD_INTEGRATION_TESTS requires DMCP_BUILD_ADAPTER_FAKE=ON")
  endif()
  add_subdirectory(integration)
endif()
