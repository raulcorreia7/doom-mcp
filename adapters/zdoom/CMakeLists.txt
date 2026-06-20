dmcp_collect_sources(ADAPTER_ZDOOM_SOURCES src)

add_library(adapter_zdoom ${DMCP_LIBRARY_TYPE} ${ADAPTER_ZDOOM_SOURCES})
add_library(dmcp::adapter_zdoom ALIAS adapter_zdoom)

set_target_properties(adapter_zdoom PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME dmcp_zdoom
)

if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND DMCP_BUILD_SHARED)
    target_link_options(adapter_zdoom PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/dmcp_zdoom.version"
    )
endif()

if("${DMCP_ZDOOM_INCLUDE_DIRS}" STREQUAL "")
    message(FATAL_ERROR
        "DMCP_BUILD_ADAPTER_ZDOOM=ON requires DMCP_ZDOOM_INCLUDE_DIRS. "
        "Set it to the ZDoom SDK include paths (semicolon-separated)."
    )
endif()

target_include_directories(adapter_zdoom
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${DMCP_ZDOOM_INCLUDE_DIRS}
)

target_link_libraries(adapter_zdoom
    PUBLIC
        dmcp::core
    PRIVATE
        dmcp_adapter_common
)

if(DMCP_BUILD_SHARED)
    target_compile_definitions(adapter_zdoom PRIVATE DMCP_BUILDING)
endif()

dmcp_enable_warnings(adapter_zdoom)
