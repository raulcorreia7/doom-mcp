#!/usr/bin/env bash
#
# Purpose: Build and test one DMCP SDK CI matrix entry without launching a game.
# Dependencies: bash, cmake, ctest

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

Build and test one DMCP SDK CI matrix entry without launching a game.

Options:
      --build-dir DIR   CMake build directory.
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

configure_sdk() {
  cmake -S "$REPO_ROOT" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DDMCP_BUILD_TESTS=ON \
    -DDMCP_BUILD_EXAMPLES=OFF \
    -DDMCP_BUILD_INTEGRATION_TESTS=ON \
    -DDMCP_BUILD_ADAPTER_FAKE=ON \
    -DDMCP_BUILD_ADAPTER_ZDOOM=OFF \
    -DDMCP_BUILD_ADAPTER_CRISPY=OFF \
    -DDMCP_BUILD_SHARED="$shared" \
    -DDMCP_BUILD_SINGLE_DLL=ON
}

run_tests() {
  ctest --test-dir "$build_dir" -L unit --timeout 60 --output-on-failure
  ctest --test-dir "$build_dir" -L integration -LE requires_game --timeout 60 --output-on-failure
}

main() {
  parse_args "$@"
  validate_args
  require_command cmake
  require_command ctest

  configure_sdk
  cmake --build "$build_dir" --config "$build_type" --parallel
  run_tests
}

main "$@"
