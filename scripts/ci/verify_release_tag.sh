#!/usr/bin/env bash
#
# Purpose: Verify a DMCP release tag matches the project version.
# Dependencies: bash

set -Eeuo pipefail
IFS=$'\n\t'

readonly PROGRAM_NAME="${0##*/}"
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

tag=""

usage() {
  cat <<EOF
Usage:
  $PROGRAM_NAME --tag dmcp-vX.Y.Z[-PRERELEASE]

Verify a DMCP release tag matches the CMake project version.

Options:
      --tag TAG  Git tag to validate.
  -h, --help     Show this help and exit.

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
      --tag)
        [[ "${2:-}" ]] || die_usage "$1 requires TAG"
        tag="$2"
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

project_version() {
  sed -nE 's/^project\(dmcp .*VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' \
    "$REPO_ROOT/CMakeLists.txt" | head -n 1
}

main() {
  parse_args "$@"
  [[ -n "$tag" ]] || die_usage "--tag is required"
  [[ "$tag" == dmcp-v* ]] || die_usage "--tag must start with dmcp-v"

  local version
  version="$(project_version)"
  [[ -n "$version" ]] || die "could not read project version from CMakeLists.txt"

  local tag_version="${tag#dmcp-v}"
  if [[ "$tag_version" != "$version" && "$tag_version" != "$version"-* ]]; then
    die "tag $tag does not match project version $version"
  fi
}

main "$@"
