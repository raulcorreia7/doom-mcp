cmake_minimum_required(VERSION 3.15)
project(dmcp LANGUAGES CXX VERSION 1.0.0)

# --- Options ---
option(DMCP_BUILD_EXAMPLES "Build example server" ON)
option(DMCP_BUILD_BRIDGES "Build all standard bridges (useful for testing)" OFF)

# --- Dependencies ---
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-usockets CONFIG REQUIRED)
find_package(unofficial-uwebsockets CONFIG REQUIRED)
find_package(readerwriterqueue CONFIG REQUIRED)
find_package(Stb REQUIRED)

# --- Core Library Target ---
add_library(dmcp STATIC
  src/dmcp.cpp
  src/core/pool.cpp
  src/core/png.cpp
  src/core/server.cpp
)
add_library(dmcp::dmcp ALIAS dmcp)

# --- Compiler Settings ---
target_compile_features(dmcp PUBLIC cxx_std_17)
set_target_properties(dmcp PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

if(MSVC)
  target_compile_options(dmcp PRIVATE /W4 /permissive-)
else()
  target_compile_options(dmcp PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

# --- Include Directories ---
target_include_directories(dmcp PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_include_directories(dmcp PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${Stb_INCLUDE_DIR}
)

# --- Linking ---
target_link_libraries(dmcp PRIVATE
  nlohmann_json::nlohmann_json
  unofficial::usockets::usockets
  unofficial::uwebsockets::uwebsockets
  readerwriterqueue::readerwriterqueue
)

# --- Installation ---
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS dmcp
  EXPORT dmcpTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/dmcp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT dmcpTargets
  FILE dmcpTargets.cmake
  NAMESPACE dmcp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/dmcpConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmcpConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/dmcpConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/dmcpConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/dmcpConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
)

# --- Bridges (Optional) ---
# This allows the top-level project to decide if they want to add specific bridges
# or if they want to build everything provided by this repo.
if(DMCP_BUILD_BRIDGES)
  add_subdirectory(bridges)
endif()

# --- Examples ---
if(DMCP_BUILD_EXAMPLES AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_executable(dummy_server examples/dummy_server.cpp)
  target_link_libraries(dummy_server PRIVATE dmcp::dmcp)
endif()