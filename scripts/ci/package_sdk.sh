#!/usr/bin/env bash
#
# Purpose: Assemble one DMCP SDK archive from an already-built tree.
# Dependencies: bash, tar on Linux/macOS, 7z on Windows

set -Eeuo pipefail
IFS=$'\n\t'
shopt -s nullglob

readonly PROGRAM_NAME="${0##*/}"
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

build_dir=""
platform=""
linkage=""
asset_name=""
release_tag=""
package_root="$REPO_ROOT/build/sdk-package"
artifact_name=""
build_type="Release"

usage() {
  cat <<EOF
Usage:
  $PROGRAM_NAME --build-dir DIR --platform linux|macos|windows --linkage static|shared --asset-name FILE [OPTIONS]

Assemble one DMCP SDK archive from an already-built tree.

Options:
      --build-dir DIR      CMake build directory containing the DMCP library.
      --platform NAME      Target platform: linux, macos, or windows.
      --linkage NAME       SDK linkage: static or shared.
      --asset-name FILE    Archive file to write.
      --release-tag TAG    Release tag recorded in SDK_INFO.txt.
      --package-root DIR   Temporary package root (default: build/sdk-package).
      --build-type TYPE    Multi-config build type (default: Release).
  -h, --help               Show this help and exit.

Exit status:
  0  Success.
  1  Runtime failure.
  2  Usage or validation error.
EOF
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

die_usage() {
  printf 'error: %s\n\n' "$*" >&2
  printf "Try '%s --help' for usage.\n" "$PROGRAM_NAME" >&2
  exit 2
}

abs_path() {
  case "$1" in
    /*|[A-Za-z]:*) printf '%s\n' "$1" ;;
    *) printf '%s/%s\n' "$REPO_ROOT" "$1" ;;
  esac
}

parse_args() {
  while [[ "$#" -gt 0 ]]; do
    case "$1" in
      --build-dir)
        [[ "${2:-}" ]] || die_usage "$1 requires DIR"
        build_dir="$2"
        shift 2
        ;;
      --platform)
        [[ "${2:-}" ]] || die_usage "$1 requires NAME"
        platform="$2"
        shift 2
        ;;
      --linkage)
        [[ "${2:-}" ]] || die_usage "$1 requires NAME"
        linkage="$2"
        shift 2
        ;;
      --asset-name)
        [[ "${2:-}" ]] || die_usage "$1 requires FILE"
        asset_name="$2"
        shift 2
        ;;
      --release-tag)
        [[ "${2:-}" ]] || die_usage "$1 requires TAG"
        release_tag="$2"
        shift 2
        ;;
      --package-root)
        [[ "${2:-}" ]] || die_usage "$1 requires DIR"
        package_root="$2"
        shift 2
        ;;
      --build-type)
        [[ "${2:-}" ]] || die_usage "$1 requires TYPE"
        build_type="$2"
        shift 2
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      -*)
        die_usage "unknown option: $1"
        ;;
      *)
        die_usage "unexpected argument: $1"
        ;;
    esac
  done
}

derive_artifact_name() {
  local base="${asset_name##*/}"
  base="${base%.tar.gz}"
  base="${base%.zip}"
  printf '%s\n' "$base"
}

validate_args() {
  [[ -n "$build_dir" ]] || die_usage "--build-dir is required"
  [[ -d "$build_dir" ]] || die "build directory does not exist: $build_dir"
  case "$platform" in
    linux|macos|windows) ;;
    "") die_usage "--platform is required" ;;
    *) die_usage "--platform must be linux, macos, or windows" ;;
  esac
  case "$linkage" in
    static|shared) ;;
    "") die_usage "--linkage is required" ;;
    *) die_usage "--linkage must be static or shared" ;;
  esac
  [[ -n "$asset_name" ]] || die_usage "--asset-name is required"

  build_dir="$(abs_path "$build_dir")"
  package_root="$(abs_path "$package_root")"
  asset_name="$(abs_path "$asset_name")"
  artifact_name="$(derive_artifact_name)"
}

copy_file() {
  local src="$1"
  local dst="$2"
  [[ -f "$src" ]] || die "required file not found: $src"
  mkdir -p "$(dirname "$dst")"
  cp "$src" "$dst"
}

copy_tree() {
  local src="$1"
  local dst="$2"
  [[ -d "$src" ]] || die "required directory not found: $src"
  mkdir -p "$(dirname "$dst")"
  cp -R "$src" "$dst"
}

copy_tree_contents() {
  local src="$1"
  local dst="$2"
  [[ -d "$src" ]] || die "required directory not found: $src"
  mkdir -p "$dst"
  cp -R "$src"/. "$dst"/
}

copy_public_headers() {
  local root="$1"
  copy_tree "$REPO_ROOT/include" "$root/include"
  copy_tree_contents "$build_dir/generated/include" "$root/include"
}

first_existing() {
  for candidate in "$@"; do
    if [[ -f "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

copy_library_files() {
  local root="$1"
  mkdir -p "$root/lib" "$root/bin"

  if [[ "$platform" == "windows" ]]; then
    local import_lib
    import_lib="$(first_existing "$build_dir/$build_type/dmcp.lib" "$build_dir/dmcp.lib")" ||
      die "dmcp.lib not found in $build_dir"
    copy_file "$import_lib" "$root/lib/dmcp.lib"

    if [[ "$linkage" == "shared" ]]; then
      local dll
      dll="$(first_existing "$build_dir/$build_type/dmcp.dll" "$build_dir/dmcp.dll")" ||
        die "dmcp.dll not found in $build_dir"
      copy_file "$dll" "$root/bin/dmcp.dll"
    fi
    return
  fi

  if [[ "$linkage" == "static" ]]; then
    local static_lib
    static_lib="$(first_existing "$build_dir/libdmcp.a")" ||
      die "libdmcp.a not found in $build_dir"
    copy_file "$static_lib" "$root/lib/libdmcp.a"
    return
  fi

  if [[ "$platform" == "macos" ]]; then
    local dylib
    dylib="$(first_existing "$build_dir/libdmcp.dylib")" ||
      die "libdmcp.dylib not found in $build_dir"
    copy_file "$dylib" "$root/lib/libdmcp.dylib"
  else
    local so
    so="$(first_existing "$build_dir/libdmcp.so")" || die "libdmcp.so not found in $build_dir"
    copy_file "$so" "$root/lib/libdmcp.so"
  fi
}

write_cmake_config() {
  local root="$1"
  local runtime_kind="UNKNOWN"
  local imported_location=""
  local imported_implib=""
  local static_defs=""
  local package_version=""
  local package_major=""

  if [[ "$linkage" == "static" ]]; then
    runtime_kind="STATIC"
    static_defs='DMCP_STATIC;MCP_STATIC;MCP_CORE_STATIC'
    if [[ "$platform" == "windows" ]]; then
      imported_location='${DMCP_SDK_ROOT}/lib/dmcp.lib'
    else
      imported_location='${DMCP_SDK_ROOT}/lib/libdmcp.a'
    fi
  elif [[ "$platform" == "windows" ]]; then
    runtime_kind="SHARED"
    imported_location='${DMCP_SDK_ROOT}/bin/dmcp.dll'
    imported_implib='${DMCP_SDK_ROOT}/lib/dmcp.lib'
  elif [[ "$platform" == "macos" ]]; then
    runtime_kind="SHARED"
    imported_location='${DMCP_SDK_ROOT}/lib/libdmcp.dylib'
  else
    runtime_kind="SHARED"
    imported_location='${DMCP_SDK_ROOT}/lib/libdmcp.so'
  fi

  if [[ "$release_tag" == dmcp-v* ]]; then
    package_version="${release_tag#dmcp-v}"
  else
    package_version="$(sed -nE 's/^project\(dmcp .* VERSION ([0-9]+(\.[0-9]+){1,2}).*/\1/p' "$REPO_ROOT/CMakeLists.txt" | head -n 1)"
  fi
  [[ -n "$package_version" ]] || die "unable to derive package version"
  package_major="${package_version%%.*}"

  mkdir -p "$root/cmake"
  cat >"$root/cmake/dmcp-config.cmake" <<EOF
get_filename_component(DMCP_SDK_ROOT "\${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

EOF

  if [[ "$linkage" == "static" ]]; then
    cat >>"$root/cmake/dmcp-config.cmake" <<'EOF'
include(CMakeFindDependencyMacro)

if(NOT CMAKE_CXX_COMPILER_LOADED)
  enable_language(CXX)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_dependency(Threads)

EOF
  fi

  cat >>"$root/cmake/dmcp-config.cmake" <<EOF
add_library(dmcp::runtime ${runtime_kind} IMPORTED)
set_target_properties(dmcp::runtime PROPERTIES
  IMPORTED_LOCATION "${imported_location}"
  INTERFACE_INCLUDE_DIRECTORIES "\${DMCP_SDK_ROOT}/include")
EOF

  if [[ -n "$imported_implib" ]]; then
    cat >>"$root/cmake/dmcp-config.cmake" <<EOF
set_target_properties(dmcp::runtime PROPERTIES
  IMPORTED_IMPLIB "${imported_implib}")
EOF
  fi

  if [[ -n "$static_defs" ]]; then
    cat >>"$root/cmake/dmcp-config.cmake" <<EOF
set_target_properties(dmcp::runtime PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
  INTERFACE_COMPILE_DEFINITIONS "${static_defs}")
target_link_libraries(dmcp::runtime INTERFACE Threads::Threads)
if(WIN32)
  target_link_libraries(dmcp::runtime INTERFACE ws2_32)
endif()
EOF
  fi

  cat >>"$root/cmake/dmcp-config.cmake" <<'EOF'

foreach(_dmcp_alias IN ITEMS dmcp::doom dmcp::core dmcp::generic mcp::core mcp::generic mcp::game)
  if(NOT TARGET ${_dmcp_alias})
    add_library(${_dmcp_alias} INTERFACE IMPORTED)
    target_link_libraries(${_dmcp_alias} INTERFACE dmcp::runtime)
  endif()
endforeach()
unset(_dmcp_alias)
EOF

  cat >"$root/cmake/dmcp-config-version.cmake" <<EOF
set(PACKAGE_VERSION "${package_version}")

if(PACKAGE_FIND_VERSION_RANGE)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
elseif(NOT PACKAGE_FIND_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
else()
  if(PACKAGE_FIND_VERSION_MAJOR STREQUAL "${package_major}" AND
     PACKAGE_FIND_VERSION VERSION_LESS_EQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
  else()
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
  endif()

  if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
EOF
}

write_sdk_info() {
  local root="$1"
  local commit="unknown"
  if git -C "$REPO_ROOT" rev-parse HEAD >/dev/null 2>&1; then
    commit="$(git -C "$REPO_ROOT" rev-parse HEAD)"
  fi

  {
    printf 'name=dmcp-sdk\n'
    printf 'release_tag=%s\n' "${release_tag:-unknown}"
    printf 'commit=%s\n' "$commit"
    printf 'platform=%s\n' "$platform"
    printf 'linkage=%s\n' "$linkage"
  } >"$root/SDK_INFO.txt"
}

copy_docs() {
  local root="$1"
  copy_file "$REPO_ROOT/README.md" "$root/README.md"
  copy_tree "$REPO_ROOT/docs" "$root/docs"
}

copy_agent_examples() {
  local root="$1"
  copy_tree "$REPO_ROOT/examples/agents/python" "$root/examples/agents/python"
  rm -rf "$root/examples/agents/python/.venv" \
         "$root/examples/agents/python/__pycache__" \
         "$root/examples/agents/python/dmcp_agent/__pycache__"
}

create_archive() {
  if [[ "$platform" == "windows" ]]; then
    (cd "$package_root" && 7z a "$asset_name" "$artifact_name")
    return
  fi
  tar -czf "$asset_name" -C "$package_root" "$artifact_name"
}

main() {
  parse_args "$@"
  validate_args

  local root="$package_root/$artifact_name"
  rm -rf "$root" "$asset_name"
  mkdir -p "$root"

  copy_public_headers "$root"
  copy_library_files "$root"
  write_cmake_config "$root"
  write_sdk_info "$root"
  copy_docs "$root"
  copy_agent_examples "$root"
  create_archive

  "$SCRIPT_DIR/verify_sdk_package.sh" \
    --asset-name "$asset_name" \
    --artifact-name "$artifact_name" \
    --platform "$platform" \
    --linkage "$linkage"
}

main "$@"
