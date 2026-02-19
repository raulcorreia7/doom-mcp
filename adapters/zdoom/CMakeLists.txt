set(DMCP_ZDOOM_SOURCES
    adapter.cpp
    commands.cpp
)

add_library(dmcp_zdoom ${DMCP_LIBRARY_TYPE} ${DMCP_ZDOOM_SOURCES})
add_library(dmcp::zdoom ALIAS dmcp_zdoom)

if(DMCP_BUILD_SHARED AND WIN32)
    set_target_properties(dmcp_zdoom PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if("${DMCP_ZDOOM_INCLUDE_DIRS}" STREQUAL "")
    message(FATAL_ERROR
        "DMCP_BUILD_ADAPTER_ZDOOM=ON requires DMCP_ZDOOM_INCLUDE_DIRS. "
        "Set it to the ZDoom SDK include paths (semicolon-separated)."
    )
endif()

target_include_directories(dmcp_zdoom
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${DMCP_ZDOOM_INCLUDE_DIRS}
)

target_link_libraries(dmcp_zdoom
    PUBLIC
        dmcp::core
)

dmcp_enable_warnings(dmcp_zdoom)
