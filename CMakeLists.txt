cmake_minimum_required(VERSION 3.15)
project(dmcp LANGUAGES CXX VERSION 0.1.0)

# --- Dependencies ---
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-usockets CONFIG REQUIRED)
find_package(unofficial-uwebsockets CONFIG REQUIRED)
find_package(readerwriterqueue CONFIG REQUIRED)
find_package(Stb REQUIRED)

# --- Main Library Target ---
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

# Warnings
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

# Export Targets
install(EXPORT dmcpTargets
  FILE dmcpTargets.cmake
  NAMESPACE dmcp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmcp
)

# Config Version
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/dmcpConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

# Config File
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

# --- Subdirectories ---
add_subdirectory(adapters)

# --- Examples ---
# Only build examples if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_executable(dummy_server examples/dummy_server.cpp)
  target_link_libraries(dummy_server PRIVATE dmcp::dmcp)
endif()
