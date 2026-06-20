#!/usr/bin/env bash
#
# Purpose: Build DMCP adapter link checks without launching a game.
# Dependencies: bash, cmake, ctest

set -Eeuo pipefail
IFS=$'\n\t'

readonly PROGRAM_NAME="${0##*/}"
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

build_dir="$REPO_ROOT/build/adapters"
headers_dir="$REPO_ROOT/build/crispy-headers"
build_type="Release"

usage() {
  cat <<EOF
Usage:
  $PROGRAM_NAME [--build-dir DIR] [--headers-dir DIR] [--build-type TYPE]

Build DMCP adapter link checks without launching a game.

Options:
      --build-dir DIR    Adapter CMake build directory.
      --headers-dir DIR  Crispy Doom generated-header build directory.
      --build-type TYPE  CMake build type (default: Release).
  -h, --help             Show this help and exit.

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
      --headers-dir)
        [[ "${2:-}" ]] || die_usage "$1 requires DIR"
        headers_dir="$2"
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

assert_file() {
  [[ -f "$1" ]] || die "expected file is missing: $1"
}

assert_no_wads() {
  if find "$REPO_ROOT" -path "$REPO_ROOT/.git" -prune -o -iname 'doom1.wad' -print -quit | grep -q .; then
    die "adapter checks produced or downloaded doom1.wad"
  fi
}

generate_crispy_headers() {
  cmake -S "$REPO_ROOT/crispy-doom" -B "$headers_dir" \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DENABLE_SDL2_NET=OFF \
    -DENABLE_SDL2_MIXER=OFF
  assert_file "$headers_dir/config.h"
}

build_adapters() {
  cmake -S "$REPO_ROOT" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DDMCP_BUILD_TESTS=ON \
    -DDMCP_BUILD_EXAMPLES=OFF \
    -DDMCP_BUILD_INTEGRATION_TESTS=OFF \
    -DDMCP_BUILD_SHARED=ON \
    -DDMCP_BUILD_SINGLE_DLL=ON \
    -DDMCP_BUILD_ADAPTER_FAKE=ON \
    -DDMCP_BUILD_ADAPTER_CRISPY=ON \
    -DDMCP_BUILD_ADAPTER_ZDOOM=OFF \
    -DDMCP_CRISPY_SRC_DIR="$REPO_ROOT/crispy-doom/src" \
    -DDMCP_CRISPY_DOOM_DIR="$REPO_ROOT/crispy-doom/src/doom" \
    -DDMCP_CRISPY_GEN_INCLUDE_DIR="$headers_dir"

  cmake --build "$build_dir" --config "$build_type" --parallel
  ctest --test-dir "$build_dir" -L unit --timeout 60 --output-on-failure

  assert_file "$build_dir/libdmcp.so"
  assert_file "$build_dir/adapters/fake/libdmcp_fake.so"
  assert_file "$build_dir/adapters/crispy-doom/libdmcp_crispy.so"
  assert_no_wads
}

validate_zdoom_sdk_guard() {
  local guard_dir="$REPO_ROOT/build/zdoom-no-sdk"
  local log_file="$REPO_ROOT/build/zdoom-no-sdk.log"

  if cmake -S "$REPO_ROOT" -B "$guard_dir" \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DDMCP_BUILD_TESTS=OFF \
    -DDMCP_BUILD_EXAMPLES=OFF \
    -DDMCP_BUILD_INTEGRATION_TESTS=OFF \
    -DDMCP_BUILD_ADAPTER_ZDOOM=ON \
    -DDMCP_BUILD_ADAPTER_FAKE=OFF \
    -DDMCP_BUILD_ADAPTER_CRISPY=OFF >"$log_file" 2>&1; then
    die "ZDoom adapter configured without required SDK headers"
  fi

  grep -q "requires DMCP_ZDOOM_INCLUDE_DIRS" "$log_file" ||
    die "ZDoom SDK guard did not report the expected message"
}

main() {
  parse_args "$@"
  require_command cmake
  require_command ctest

  generate_crispy_headers
  build_adapters
  validate_zdoom_sdk_guard
}

main "$@"
