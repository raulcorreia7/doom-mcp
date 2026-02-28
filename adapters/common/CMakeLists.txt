cmake_minimum_required(VERSION 3.16)

add_library(dmcp_adapter_common STATIC
    src/dmcp_hooks.c
)

target_include_directories(dmcp_adapter_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(dmcp_adapter_common PRIVATE c_std_11)
