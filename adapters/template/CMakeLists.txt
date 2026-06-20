# Copy this directory to adapters/<engine>, rename the target/symbol prefix, then
# add that adapter to the SDK build. The template itself is intentionally not
# included by the top-level DMCP build.

cmake_minimum_required(VERSION 3.25)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(dmcp_adapter_template LANGUAGES C CXX)

  set(DMCP_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH
      "Path to the DMCP SDK source tree")
  get_filename_component(DMCP_SDK_ROOT "${DMCP_SDK_ROOT}" ABSOLUTE)

  set(DMCP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(DMCP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(DMCP_BUILD_ADAPTER_FAKE OFF CACHE BOOL "" FORCE)
  set(DMCP_BUILD_ADAPTER_ZDOOM OFF CACHE BOOL "" FORCE)
  set(DMCP_BUILD_ADAPTER_CRISPY OFF CACHE BOOL "" FORCE)
  set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm-cache" CACHE PATH
      "CPM dependency cache for the standalone adapter template build" FORCE)

  add_subdirectory("${DMCP_SDK_ROOT}" "${CMAKE_BINARY_DIR}/dmcp-sdk")
endif()

if(NOT DEFINED DMCP_SDK_ROOT)
  get_filename_component(DMCP_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

if(NOT DEFINED DMCP_LIBRARY_TYPE)
  if(DMCP_BUILD_SHARED)
    set(DMCP_LIBRARY_TYPE SHARED)
  else()
    set(DMCP_LIBRARY_TYPE STATIC)
  endif()
endif()

if(NOT COMMAND dmcp_collect_sources)
  function(dmcp_collect_sources out_var)
    set(globs)
    foreach(root IN LISTS ARGN)
      list(APPEND globs
          "${CMAKE_CURRENT_SOURCE_DIR}/${root}/*.c"
          "${CMAKE_CURRENT_SOURCE_DIR}/${root}/*.cc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${root}/*.cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/${root}/*.cxx")
    endforeach()

    file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${globs})
    list(SORT sources)
    set(${out_var} "${sources}" PARENT_SCOPE)
  endfunction()
endif()

if(NOT TARGET dmcp_adapter_common)
  add_subdirectory("${DMCP_SDK_ROOT}/adapters/common"
                   "${CMAKE_BINARY_DIR}/dmcp-adapter-common")
endif()

dmcp_collect_sources(DMCP_TEMPLATE_SOURCES src)

add_library(dmcp_adapter_template ${DMCP_LIBRARY_TYPE} ${DMCP_TEMPLATE_SOURCES})
add_library(dmcp::adapter_template ALIAS dmcp_adapter_template)

set_target_properties(dmcp_adapter_template PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME dmcp_template
)

target_compile_features(dmcp_adapter_template PRIVATE cxx_std_17)

target_include_directories(dmcp_adapter_template
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${DMCP_SDK_ROOT}/include>
        $<BUILD_INTERFACE:${DMCP_SDK_ROOT}/adapters/common/include>
)

target_link_libraries(dmcp_adapter_template
    PUBLIC
        dmcp::core
    PRIVATE
        dmcp_adapter_common
)

if(DMCP_BUILD_SHARED)
  target_compile_definitions(dmcp_adapter_template PRIVATE DMCP_BUILDING)

  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_options(dmcp_adapter_template PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/dmcp_template.version")
  endif()
endif()
