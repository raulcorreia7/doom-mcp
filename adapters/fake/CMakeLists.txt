dmcp_collect_sources(ADAPTER_FAKE_SOURCES src)

add_library(adapter_fake ${DMCP_LIBRARY_TYPE} ${ADAPTER_FAKE_SOURCES})
add_library(dmcp::adapter_fake ALIAS adapter_fake)

set_target_properties(adapter_fake PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME dmcp_fake
)

if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND DMCP_BUILD_SHARED)
    target_link_options(adapter_fake PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/dmcp_fake.version"
    )
endif()

target_include_directories(adapter_fake
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)

target_link_libraries(adapter_fake
    PUBLIC
        dmcp::core
)

if(DMCP_BUILD_SHARED)
    target_compile_definitions(adapter_fake PRIVATE DMCP_BUILDING)
endif()

dmcp_enable_warnings(adapter_fake)
