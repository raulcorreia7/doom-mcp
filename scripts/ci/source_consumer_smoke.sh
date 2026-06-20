#!/usr/bin/env bash
#
# Purpose: Prove DMCP can be consumed from source via add_subdirectory.
# Dependencies: bash, cmake

set -Eeuo pipefail
IFS=$'\n\t'

readonly PROGRAM_NAME="${0##*/}"
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

build_dir=""
build_type="Release"
shared=""

usage() {
  cat <<EOF
Usage:
  $PROGRAM_NAME --build-dir DIR --shared ON|OFF [--build-type TYPE]

Build and run a tiny local source consumer that links dmcp::runtime.

Options:
      --build-dir DIR   Directory for generated smoke project and build output.
      --shared ON|OFF   Value for DMCP_BUILD_SHARED.
      --build-type TYPE CMake build type (default: Release).
  -h, --help            Show this help and exit.

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

require_command() {
  command -v "$1" >/dev/null 2>&1 || die "missing required command: $1"
}

parse_args() {
  while [[ "$#" -gt 0 ]]; do
    case "$1" in
      --build-dir)
        [[ "${2:-}" ]] || die_usage "$1 requires DIR"
        build_dir="$2"
        shift 2
        ;;
      --shared)
        [[ "${2:-}" ]] || die_usage "$1 requires ON or OFF"
        shared="$2"
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

validate_args() {
  [[ -n "$build_dir" ]] || die_usage "--build-dir is required"
  [[ "$shared" == "ON" || "$shared" == "OFF" ]] ||
    die_usage "--shared must be ON or OFF"
}

write_smoke_project() {
  mkdir -p "$build_dir"

  cat >"$build_dir/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.25)
project(dmcp_consumer_smoke LANGUAGES C)

add_subdirectory("${DMCP_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/dmcp" EXCLUDE_FROM_ALL)

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE dmcp::runtime)
CMAKE

  cat >"$build_dir/main.c" <<'C'
#include "mcp/core/core.h"

int main(void) {
  mcp_core_api_info_t api = {0};
  api.struct_size = sizeof(api);
  mcp_core_api_get(&api);
  return api.api_version == MCP_CORE_API_VERSION ? 0 : 1;
}
C
}

run_consumer() {
  local out_dir="$build_dir/out"
  local -a candidates=(
    "$out_dir/consumer"
    "$out_dir/Release/consumer.exe"
    "$out_dir/Debug/consumer.exe"
  )
  local executable=""

  export PATH="$out_dir/dmcp:$out_dir/dmcp/Release:$out_dir/dmcp/Debug:$PATH"
  export LD_LIBRARY_PATH="$out_dir/dmcp:${LD_LIBRARY_PATH:-}"
  export DYLD_LIBRARY_PATH="$out_dir/dmcp:${DYLD_LIBRARY_PATH:-}"

  for candidate in "${candidates[@]}"; do
    if [[ -x "$candidate" ]]; then
      executable="$candidate"
      break
    fi
  done

  [[ -n "$executable" ]] || die "consumer executable was not produced"
  "$executable"
}

main() {
  parse_args "$@"
  validate_args
  require_command cmake

  write_smoke_project
  cmake -S "$build_dir" -B "$build_dir/out" \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DDMCP_SOURCE_DIR="$REPO_ROOT" \
    -DDMCP_BUILD_SHARED="$shared" \
    -DDMCP_BUILD_SINGLE_DLL=ON
  cmake --build "$build_dir/out" --config "$build_type" --parallel
  run_consumer
}

main "$@"
