file(GLOB DMCP_FAKE_SOURCES src/*.cpp)

add_library(fake ${DMCP_LIBRARY_TYPE} ${DMCP_FAKE_SOURCES})
add_library(dmcp::fake ALIAS fake)

set_target_properties(fake PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME dmcp_fake
)

if(NOT WIN32 AND DMCP_BUILD_SHARED)
    target_link_options(fake PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/dmcp_fake.version"
    )
endif()

target_include_directories(fake
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(fake
    PUBLIC
        dmcp::core
)

dmcp_enable_warnings(fake)
