#!/usr/bin/env bash
#
# Purpose: Fail CI if a no-game workflow downloaded or produced IWAD files.
# Dependencies: bash

set -Eeuo pipefail
IFS=$'\n\t'

readonly PROGRAM_NAME="${0##*/}"
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

root="$REPO_ROOT"

usage() {
  cat <<EOF
Usage:
  $PROGRAM_NAME [--root DIR]

Fail if a no-game workflow downloaded or produced IWAD files.

Options:
      --root DIR  Repository root to inspect (default: $REPO_ROOT).
  -h, --help      Show this help and exit.

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
      --root)
        [[ "${2:-}" ]] || die_usage "$1 requires DIR"
        root="$2"
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

main() {
  parse_args "$@"
  [[ -d "$root" ]] || die "root is not a directory: $root"

  if find "$root" -path "$root/.git" -prune -o -iname 'doom1.wad' -print -quit | grep -q .; then
    die "CI produced or downloaded doom1.wad"
  fi

  [[ ! -d "$root/assets/wads" ]] || die "CI produced assets/wads"
}

main "$@"
