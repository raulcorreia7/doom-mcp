dmcp_collect_sources(DMCP_ADAPTER_COMMON_SOURCES src)

add_library(dmcp_adapter_common STATIC ${DMCP_ADAPTER_COMMON_SOURCES})

target_include_directories(dmcp_adapter_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(dmcp_adapter_common PRIVATE c_std_99)
