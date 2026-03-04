#!/usr/bin/env bash
# download_wad.sh - Integration wrapper for canonical WAD downloader
#
# Usage:
#   tests/integration/download_wad.sh [OPTIONS]
#
# Options:
#   -f, --force      Re-download even when doom1.wad already exists
#   -h, --help       Show this help
#
# Environment:
#   DMCP_ROOT        Repo root (default: auto-detected)
#   WAD_FILE         Target WAD path (default: <root>/assets/wads/doom1.wad)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMCP_ROOT="${DMCP_ROOT:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
WAD_FILE="${WAD_FILE:-$DMCP_ROOT/assets/wads/doom1.wad}"

FORCE=0

usage() {
	cat <<EOF_USAGE
Download the DOOM shareware WAD (doom1.wad) used by DMCP tests.

Usage:
  $(basename "$0") [OPTIONS]

Options:
  -f, --force      Re-download even if the file already exists
  -h, --help       Show this help
EOF_USAGE
}

while [[ $# -gt 0 ]]; do
	case "$1" in
	-f | --force)
		FORCE=1
		shift
		;;
	-h | --help)
		usage
		exit 0
		;;
	*)
		echo "error: unknown option: $1" >&2
		usage >&2
		exit 2
		;;
	esac
done

OUTPUT_DIR="$(dirname "$WAD_FILE")"
DOWNLOADER="$DMCP_ROOT/scripts/download_wad.sh"

if [[ ! -x "$DOWNLOADER" ]]; then
	echo "error: downloader not executable: $DOWNLOADER" >&2
	exit 1
fi

if [[ "$FORCE" -eq 1 ]]; then
	WAD_FILE="$WAD_FILE" "$DOWNLOADER" "$OUTPUT_DIR" --force
else
	WAD_FILE="$WAD_FILE" "$DOWNLOADER" "$OUTPUT_DIR"
fi

echo "Ready: $WAD_FILE"
