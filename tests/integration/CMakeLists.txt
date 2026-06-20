dmcp_collect_sources(DMCP_INTEGRATION_TEST_SOURCES .)

add_executable(dmcp_integration_tests ${DMCP_INTEGRATION_TEST_SOURCES})

target_include_directories(dmcp_integration_tests
  PRIVATE
    ${PROJECT_SOURCE_DIR}/tests
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(dmcp_integration_tests
  PRIVATE
    Catch2::Catch2WithMain
    dmcp::adapter_fake
    httplib::httplib
)

if(WIN32)
  target_link_libraries(dmcp_integration_tests PRIVATE ws2_32)
endif()

if(NOT MSVC)
  target_compile_options(dmcp_integration_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME dmcp_fake_transport_integration COMMAND dmcp_integration_tests)
set_tests_properties(dmcp_fake_transport_integration PROPERTIES
  LABELS "integration;no_game;fake;transport"
  TIMEOUT 30
)
