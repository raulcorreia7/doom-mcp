set(DMCP_CHOCOLATE_SOURCES
    dmcp_adapter.c
    dmcp_ascii.c
    state_player.c
    state_level.c
    state_enemies.c
    commands.c
    input.c
)

add_library(dmcp_chocolate ${DMCP_LIBRARY_TYPE} ${DMCP_CHOCOLATE_SOURCES})
add_library(dmcp::chocolate ALIAS dmcp_chocolate)

if(DMCP_BUILD_SHARED AND WIN32)
    set_target_properties(dmcp_chocolate PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(NOT EXISTS "${DMCP_CHOCOLATE_GEN_INCLUDE_DIR}/config.h")
    message(FATAL_ERROR
        "DMCP_BUILD_ADAPTER_CHOCOLATE=ON requires generated Chocolate Doom headers. "
        "Expected: ${DMCP_CHOCOLATE_GEN_INCLUDE_DIR}/config.h"
    )
endif()

target_include_directories(dmcp_chocolate
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${DMCP_CHOCOLATE_SRC_DIR}
        ${DMCP_CHOCOLATE_DOOM_DIR}
        ${DMCP_CHOCOLATE_GEN_INCLUDE_DIR}
)

target_link_libraries(dmcp_chocolate
    PUBLIC
        dmcp::core
)

target_compile_definitions(dmcp_chocolate
    PRIVATE
        CHOCOLATE_DOOM_BUILD
)

dmcp_enable_warnings(dmcp_chocolate)
