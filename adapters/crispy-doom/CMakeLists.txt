file(GLOB DMCP_CRISPY_SOURCES src/*.c)
file(GLOB DMCP_CRISPY_HEADERS include/*.h)

add_library(crispy ${DMCP_LIBRARY_TYPE} ${DMCP_CRISPY_SOURCES})
add_library(dmcp::crispy ALIAS crispy)

set_target_properties(crispy PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME dmcp_crispy
)

if(NOT WIN32 AND DMCP_BUILD_SHARED)
    target_link_options(crispy PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/dmcp_crispy.version"
    )
endif()

if(NOT EXISTS "${DMCP_CRISPY_GEN_INCLUDE_DIR}/config.h")
    message(FATAL_ERROR
        "DMCP_BUILD_ADAPTER_CRISPY=ON requires generated Crispy Doom headers. "
        "Expected: ${DMCP_CRISPY_GEN_INCLUDE_DIR}/config.h"
    )
endif()

target_include_directories(crispy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${DMCP_CRISPY_SRC_DIR}
        ${DMCP_CRISPY_DOOM_DIR}
        ${DMCP_CRISPY_GEN_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/adapters/common/include
)

target_link_libraries(crispy
    PUBLIC
        dmcp::core
    PRIVATE
        dmcp_adapter_common
)

target_compile_definitions(crispy
    PRIVATE
        CRISPY_DOOM_BUILD
)

dmcp_enable_warnings(crispy)
