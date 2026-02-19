# Integration Tests CMakeLists
# E2E tests for DMCP with actual game engines

# Download WAD target
add_custom_target(download_wad
    COMMAND ${CMAKE_COMMAND} -E echo "Downloading DOOM shareware WAD..."
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/download_wad.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Downloading doom1.wad shareware"
)

# Headless test target (run manually after building Chocolate Doom)
add_custom_target(test_headless
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_headless.sh
    DEPENDS download_wad dmcp_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running headless Chocolate Doom tests"
)

# Add to ctest only if Chocolate Doom binary exists
if(EXISTS "${CMAKE_SOURCE_DIR}/chocolate-doom/build/src/chocolate-doom")
    add_test(
        NAME "Integration: Headless Doom"
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_headless.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    set_tests_properties("Integration: Headless Doom" PROPERTIES
        TIMEOUT 120
        ENVIRONMENT "DMCP_PORT=7060"
        RUN_SERIAL TRUE
        LABELS "integration;e2e;headless"
    )
endif()
