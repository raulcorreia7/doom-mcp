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

main() {
  parse_args "$@"
  validate_args

  rm -rf "$smoke_dir"
  mkdir -p "$smoke_dir"

  if [[ "$asset_name" == *.zip ]]; then
    7z x "$asset_name" "-o$smoke_dir" >/dev/null
  else
    tar -xzf "$asset_name" -C "$smoke_dir"
  fi

  local root="$smoke_dir/$artifact_name"
  [[ -d "$root" ]] || die "archive root directory not found: $root"

  assert_file "$root/include/dmcp/doom/api.h"
  assert_file "$root/include/mcp/generic/server.h"
  assert_file "$root/include/mcp/core/core.h"
  assert_file "$root/cmake/dmcp-config.cmake"
  assert_file "$root/SDK_INFO.txt"
  assert_file "$root/README.md"

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
}

main "$@"
