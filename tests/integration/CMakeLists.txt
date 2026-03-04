# Integration Tests CMakeLists
# E2E tests for DMCP with actual game engines

# Download WAD target
add_custom_target(download_wad
    COMMAND ${CMAKE_COMMAND} -E echo "Downloading DOOM shareware WAD..."
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/download_wad.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Downloading doom1.wad shareware"
)

# Headless test target (run manually after building Crispy Doom)
add_custom_target(test_headless
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_headless.sh
    DEPENDS download_wad dmcp_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running headless Crispy Doom tests"
)

# Add Crispy Doom headless integration test when binary exists
if(EXISTS "${CMAKE_SOURCE_DIR}/crispy-doom/build/src/crispy-doom")
    add_test(
        NAME "Integration: Headless Crispy"
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_headless.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    set_tests_properties("Integration: Headless Crispy" PROPERTIES
        TIMEOUT 120
        ENVIRONMENT "DMCP_PORT=7061;DOOM_ENGINE=crispy"
        RUN_SERIAL TRUE
        LABELS "integration;e2e;headless;crispy"
    )
endif()
