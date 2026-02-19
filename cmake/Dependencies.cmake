CPMAddPackage(
  NAME yyjson
  GITHUB_REPOSITORY ibireme/yyjson
  VERSION 0.10.0
  GIT_TAG 0.10.0
  OPTIONS "YYJSON_BUILD_TESTS OFF" "YYJSON_BUILD_MISC OFF"
)

if(DMCP_BUILD_SHARED AND TARGET yyjson)
  set_target_properties(yyjson PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

CPMFindPackage(
  NAME uSockets
  GITHUB_REPOSITORY uNetworking/uSockets
  VERSION 0.8.8
  GIT_SUBMODULES ""
)

if(uSockets_ADDED)
  file(GLOB_RECURSE USOCKETS_SOURCES CONFIGURE_DEPENDS "${uSockets_SOURCE_DIR}/src/*.c")
  add_library(uSockets STATIC ${USOCKETS_SOURCES})
  target_include_directories(uSockets PUBLIC "${uSockets_SOURCE_DIR}/src")
  target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL)

  if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(uSockets PUBLIC Threads::Threads)
  elseif(WIN32)
    target_link_libraries(uSockets PUBLIC ws2_32)
  endif()

  if(NOT MSVC)
    target_compile_options(uSockets PRIVATE -w)
  endif()

  if(DMCP_BUILD_SHARED)
    set_target_properties(uSockets PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

CPMFindPackage(
  NAME uWebSockets
  GITHUB_REPOSITORY uNetworking/uWebSockets
  VERSION 20.74.0
  GIT_SUBMODULES ""
)

if(uWebSockets_ADDED)
  find_package(ZLIB REQUIRED)
  add_library(uWebSockets INTERFACE)
  target_include_directories(uWebSockets INTERFACE "${uWebSockets_SOURCE_DIR}/src")
  target_link_libraries(uWebSockets INTERFACE uSockets ZLIB::ZLIB)

  if(NOT MSVC)
    target_compile_options(uWebSockets INTERFACE -Wno-deprecated-declarations)
  endif()
endif()

if(DMCP_BUILD_TESTS)
  CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.7.1
    GIT_TAG v3.7.1
    OPTIONS "CATCH_INSTALL_DOCS OFF" "CATCH_INSTALL_EXTRAS OFF"
  )
endif()
