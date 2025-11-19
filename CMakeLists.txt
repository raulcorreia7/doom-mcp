cmake_minimum_required(VERSION 3.15)
project(dmcp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION
    "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
  file(DOWNLOAD
       https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
       ${CPM_DOWNLOAD_LOCATION})
endif()
include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  VERSION 3.12.0
  OPTIONS "JSON_BuildTests OFF")

CPMAddPackage(
  NAME tl-expected
  GITHUB_REPOSITORY TartanLlama/expected
  VERSION 1.3.1
  OPTIONS "EXPECTED_BUILD_TESTS OFF")

CPMAddPackage(
  NAME uSockets
  GITHUB_REPOSITORY uNetworking/uSockets
  VERSION 0.8.8
  OPTIONS "USOCKETS_BUILD_EXAMPLE OFF")
set(uSockets_SOURCE_DIR ${CPM_PACKAGE_uSockets_SOURCE_DIR})

CPMAddPackage(
  NAME uWebSockets
  GITHUB_REPOSITORY uNetworking/uWebSockets
  VERSION 20.74.0
  DOWNLOAD_ONLY YES)
set(uWebSockets_SOURCE_DIR ${CPM_PACKAGE_uWebSockets_SOURCE_DIR})

CPMAddPackage(
  NAME readerwriterqueue
  GITHUB_REPOSITORY cameron314/readerwriterqueue
  VERSION 1.0.6)
CPMAddPackage(
  NAME stb
  GITHUB_REPOSITORY nothings/stb
  GIT_TAG master
  DOWNLOAD_ONLY YES)

add_library(
  dmcp STATIC
  src/dmcp.cpp
  src/core/pool.cpp
  src/core/png.cpp
  src/core/server.cpp
  src/logic/binder.cpp)

target_include_directories(
  dmcp
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
  PRIVATE ${uWebSockets_SOURCE_DIR}/src
  PRIVATE ${uSockets_SOURCE_DIR}/src
  PRIVATE ${readerwriterqueue_SOURCE_DIR}
  PRIVATE ${stb_SOURCE_DIR})

target_link_libraries(
  dmcp
  PRIVATE nlohmann_json::nlohmann_json
          tl::expected
          uWebSockets)
