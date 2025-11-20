# --- Bridge: UZDoom ---
add_library(dmcp_bridge_uzdoom INTERFACE)
add_library(dmcp::bridge::uzdoom ALIAS dmcp_bridge_uzdoom)

# The bridge implementation needs the DMCP Core
target_link_libraries(dmcp_bridge_uzdoom INTERFACE dmcp::dmcp)

# The source is added to the interface
target_sources(dmcp_bridge_uzdoom INTERFACE "${CMAKE_CURRENT_LIST_DIR}/bridge.cpp")