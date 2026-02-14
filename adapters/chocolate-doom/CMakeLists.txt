# Chocolate Doom Adapter CMakeLists
cmake_minimum_required(VERSION 3.25)

# Chocolate Doom adapter library
add_library(dmcp_chocolate STATIC
    adapter.c
)

target_include_directories(dmcp_chocolate
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CHOCOLATE_DOOM_SRC_DIR}
)

target_link_libraries(dmcp_chocolate
    PUBLIC
        dmcp::core
)

# Compile definitions for Chocolate Doom integration
target_compile_definitions(dmcp_chocolate
    PRIVATE
        CHOCOLATE_DOOM_BUILD
)

if(NOT MSVC)
    target_compile_options(dmcp_chocolate PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Alias for consumers
add_library(dmcp::chocolate ALIAS dmcp_chocolate)
