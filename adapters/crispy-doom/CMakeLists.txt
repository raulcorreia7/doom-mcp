dmcp_collect_sources(ADAPTER_CRISPY_SOURCES src)

add_library(adapter_crispy ${DMCP_LIBRARY_TYPE} ${ADAPTER_CRISPY_SOURCES})
add_library(dmcp::adapter_crispy ALIAS adapter_crispy)

set_target_properties(adapter_crispy PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME dmcp_crispy
)

if(NOT WIN32 AND DMCP_BUILD_SHARED)
    target_link_options(adapter_crispy PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/dmcp_crispy.version"
    )
endif()

if(NOT EXISTS "${DMCP_CRISPY_GEN_INCLUDE_DIR}/config.h")
    message(FATAL_ERROR
        "DMCP_BUILD_ADAPTER_CRISPY=ON requires generated Crispy Doom headers. "
        "Expected: ${DMCP_CRISPY_GEN_INCLUDE_DIR}/config.h"
    )
endif()

target_include_directories(adapter_crispy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${DMCP_CRISPY_SRC_DIR}
        ${DMCP_CRISPY_DOOM_DIR}
        ${DMCP_CRISPY_GEN_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)

target_link_libraries(adapter_crispy
    PUBLIC
        dmcp::core
    PRIVATE
        dmcp_adapter_common
)

target_compile_definitions(adapter_crispy
    PRIVATE
        CRISPY_DOOM_BUILD
)

dmcp_enable_warnings(adapter_crispy)
