function(dmcp_collect_sources out_var)
  set(globs)
  foreach(root IN LISTS ARGN)
    list(APPEND globs
      "${CMAKE_CURRENT_SOURCE_DIR}/${root}/*.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/${root}/*.cc"
      "${CMAKE_CURRENT_SOURCE_DIR}/${root}/*.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/${root}/*.cxx"
    )
  endforeach()

  file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${globs})
  list(SORT sources)
  set(${out_var} "${sources}" PARENT_SCOPE)
endfunction()

dmcp_collect_sources(MCP_CORE_FOUNDATION_SOURCES src/core)
dmcp_collect_sources(MCP_GENERIC_SOURCES src/mcp)
dmcp_collect_sources(MCP_GAME_SOURCES src/game)
dmcp_collect_sources(DMCP_CORE_SOURCES src/doom)
