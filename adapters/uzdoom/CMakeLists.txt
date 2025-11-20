add_library(dmcp_adapter_uzdoom INTERFACE)
add_library(dmcp::adapter::uzdoom ALIAS dmcp_adapter_uzdoom)

target_sources(dmcp_adapter_uzdoom INTERFACE "${CMAKE_CURRENT_LIST_DIR}/adapter.cpp")
target_link_libraries(dmcp_adapter_uzdoom INTERFACE dmcp::dmcp)
