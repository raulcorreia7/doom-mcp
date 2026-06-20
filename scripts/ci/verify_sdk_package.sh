#!/usr/bin/env bash
#
# Purpose: Verify a packaged DMCP SDK archive without installing it system-wide.
# Dependencies: bash, tar for .tar.gz archives, 7z for .zip archives

set -Eeuo pipefail
IFS=$'\n\t'
shopt -s nullglob

readonly PROGRAM_NAME="${0##*/}"

asset_name=""
artifact_name=""
platform=""
linkage=""
smoke_dir="build/sdk-package-smoke"

usage() {
  cat <<EOF
Usage:
  $PROGRAM_NAME --asset-name FILE --artifact-name NAME --platform linux|macos|windows --linkage static|shared [OPTIONS]

Verify a packaged DMCP SDK archive without installing it system-wide.

Options:
      --asset-name FILE     Archive file to inspect.
      --artifact-name NAME  Directory name expected inside the archive.
      --platform NAME       Target platform: linux, macos, or windows.
      --linkage NAME        SDK linkage: static or shared.
      --smoke-dir DIR       Temporary extraction directory (default: build/sdk-package-smoke).
  -h, --help                Show this help and exit.

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
      --asset-name)
        [[ "${2:-}" ]] || die_usage "$1 requires FILE"
        asset_name="$2"
        shift 2
        ;;
      --artifact-name)
        [[ "${2:-}" ]] || die_usage "$1 requires NAME"
        artifact_name="$2"
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
      --smoke-dir)
        [[ "${2:-}" ]] || die_usage "$1 requires DIR"
        smoke_dir="$2"
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
  [[ -n "$asset_name" ]] || die_usage "--asset-name is required"
  [[ -f "$asset_name" ]] || die "asset does not exist: $asset_name"
  [[ -n "$artifact_name" ]] || die_usage "--artifact-name is required"
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
}

assert_file() {
  [[ -f "$1" ]] || die "missing package file: $1"
}

sdk_version_from_info() {
  local info_file="$1"
  local release_tag
  release_tag="$(sed -n 's/^release_tag=//p' "$info_file" | head -n 1)"

  if [[ "$release_tag" == dmcp-v* ]]; then
    printf '%s\n' "${release_tag#dmcp-v}"
  fi
}

write_consumer_project() {
  local consumer_dir="$1"
  local sdk_version="$2"
  local find_package_line="find_package(dmcp REQUIRED CONFIG)"

  if [[ -n "$sdk_version" ]]; then
    find_package_line="find_package(dmcp ${sdk_version} REQUIRED CONFIG)"
  fi

  mkdir -p "$consumer_dir"
  cat >"$consumer_dir/CMakeLists.txt" <<CMAKE
cmake_minimum_required(VERSION 3.25)
project(dmcp_package_consumer LANGUAGES C)

${find_package_line}

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE dmcp::runtime)
CMAKE

  cat >"$consumer_dir/main.c" <<'C'
#include "dmcp/doom/api.h"

int main(void) {
  dmcp_config_t config = dmcp_config_default();
  dmcp_context_t* ctx = 0;

  config.start_transport = false;
  ctx = dmcp_context_create(&config);
  if (ctx == 0) {
    return 1;
  }

  dmcp_context_destroy(ctx);
  return 0;
}
C
}

run_consumer_project() {
  local root="$1"
  local consumer_dir="$2"
  local out_dir="$consumer_dir/out"
  local -a candidates=(
    "$out_dir/consumer"
    "$out_dir/Release/consumer.exe"
    "$out_dir/Debug/consumer.exe"
  )
  local executable=""

  cmake -S "$consumer_dir" -B "$out_dir" -Ddmcp_DIR="$root/cmake"
  cmake --build "$out_dir" --parallel

  export PATH="$root/bin:$root/lib:$PATH"
  export LD_LIBRARY_PATH="$root/lib:${LD_LIBRARY_PATH:-}"
  export DYLD_LIBRARY_PATH="$root/lib:${DYLD_LIBRARY_PATH:-}"

  for candidate in "${candidates[@]}"; do
    if [[ -x "$candidate" ]]; then
      executable="$candidate"
      break
    fi
  done

  [[ -n "$executable" ]] || die "package consumer executable was not produced"
  "$executable"
}

main() {
  parse_args "$@"
  validate_args
  require_command cmake

  rm -rf "$smoke_dir"
  mkdir -p "$smoke_dir"

  if [[ "$asset_name" == *.zip ]]; then
    7z x "$asset_name" "-o$smoke_dir" >/dev/null
  else
    tar -xzf "$asset_name" -C "$smoke_dir"
  fi

  local root="$smoke_dir/$artifact_name"
  [[ -d "$root" ]] || die "archive root directory not found: $root"
  root="$(cd "$root" && pwd)"

  assert_file "$root/include/dmcp/doom/api.h"
  assert_file "$root/include/mcp/generic/server.h"
  assert_file "$root/include/mcp/core/core.h"
  assert_file "$root/include/mcp/core/version_config.h"
  assert_file "$root/cmake/dmcp-config.cmake"
  assert_file "$root/cmake/dmcp-config-version.cmake"
  assert_file "$root/SDK_INFO.txt"
  assert_file "$root/README.md"
  assert_file "$root/examples/agents/python/README.md"
  assert_file "$root/examples/agents/python/pyproject.toml"
  assert_file "$root/examples/agents/python/dmcp_agent/cli.py"

  if [[ "$platform" == "windows" ]]; then
    assert_file "$root/lib/dmcp.lib"
    if [[ "$linkage" == "shared" ]]; then
      assert_file "$root/bin/dmcp.dll"
    fi
  elif [[ "$platform" == "macos" && "$linkage" == "shared" ]]; then
    assert_file "$root/lib/libdmcp.dylib"
  elif [[ "$linkage" == "shared" ]]; then
    assert_file "$root/lib/libdmcp.so"
  else
    assert_file "$root/lib/libdmcp.a"
  fi

  if find "$root" -type f -iname '*.wad' -print -quit | grep -q .; then
    die "SDK package contains game data files"
  fi

  if [[ -d "$root/assets/wads" ]]; then
    die "SDK package contains game asset directory"
  fi

  if find "$root/examples/agents/python" -type d \( -name '.venv' -o -name '__pycache__' \) \
      -print -quit | grep -q .; then
    die "SDK package contains Python virtualenvs or bytecode caches"
  fi

  local sdk_version
  sdk_version="$(sdk_version_from_info "$root/SDK_INFO.txt")"
  write_consumer_project "$smoke_dir/consumer" "$sdk_version"
  run_consumer_project "$root" "$smoke_dir/consumer"
}

main "$@"
