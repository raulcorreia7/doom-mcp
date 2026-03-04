# DMCPSourceLists.cmake
# Source discovery for core libraries.
# We intentionally use CONFIGURE_DEPENDS so adding/removing .cpp files updates
# the build graph without manually editing CMakeLists.txt.

file(
  GLOB_RECURSE MCP_GENERIC_SOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/mcp/*.cpp"
)
list(SORT MCP_GENERIC_SOURCES)

file(
  GLOB_RECURSE DMCP_CORE_SOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/doom/*.cpp"
)
list(SORT DMCP_CORE_SOURCES)
