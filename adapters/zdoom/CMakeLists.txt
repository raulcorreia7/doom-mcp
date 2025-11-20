# --- Adapter: ZDoom ---
add_library(dmcp_adapter_zdoom STATIC 
  "${CMAKE_CURRENT_LIST_DIR}/adapter.cpp"
)
add_library(dmcp::adapter::zdoom ALIAS dmcp_adapter_zdoom)

# The adapter implementation needs the DMCP Core
target_link_libraries(dmcp_adapter_zdoom PUBLIC dmcp::dmcp)

# Include directories - need access to core headers and adapter headers
# Use the project source directory to get the correct paths
target_include_directories(dmcp_adapter_zdoom PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/adapters
)
