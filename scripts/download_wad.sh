#!/usr/bin/env bash
# download_wad.sh - Download DOOM shareware WAD (doom1.wad)
#
# Usage:
#   ./scripts/download_wad.sh [OUTPUT_DIR] [OPTIONS]
#
# Examples:
#   ./scripts/download_wad.sh                 # writes ./doom1.wad
#   ./scripts/download_wad.sh assets/wads     # writes assets/wads/doom1.wad
#   ./scripts/download_wad.sh assets/wads -f  # force re-download
#
# Options:
#   -f, --force    Re-download even if doom1.wad exists
#   -h, --help     Show this help
#
# Environment:
#   WAD_FILE       Full output file path override (takes precedence)

set -euo pipefail

OUTPUT_DIR="."
FORCE=0

usage() {
	cat <<USAGE
Download DOOM shareware WAD (doom1.wad).

Usage:
  $(basename "$0") [OUTPUT_DIR] [OPTIONS]

Arguments:
  OUTPUT_DIR       Directory where doom1.wad will be written (default: .)

Options:
  -f, --force      Re-download even if doom1.wad exists
  -h, --help       Show this help
USAGE
}

if [[ $# -gt 0 ]] && [[ "$1" != -* ]]; then
	OUTPUT_DIR="$1"
	shift
fi

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

mkdir -p "$OUTPUT_DIR"
WAD_FILE="${WAD_FILE:-$OUTPUT_DIR/doom1.wad}"
WAD_DIR="$(dirname "$WAD_FILE")"
mkdir -p "$WAD_DIR"

if [[ "$FORCE" -eq 1 ]]; then
	rm -f "$WAD_FILE"
fi

if [[ -f "$WAD_FILE" ]]; then
	size=$(stat -c%s "$WAD_FILE" 2>/dev/null || stat -f%z "$WAD_FILE" 2>/dev/null || echo "0")
	if [[ "$size" -ge 4000000 ]]; then
		echo "doom1.wad already exists ($size bytes): $WAD_FILE"
		exit 0
	fi
	rm -f "$WAD_FILE"
fi

if ! command -v curl >/dev/null 2>&1 && ! command -v wget >/dev/null 2>&1; then
	echo "error: requires curl or wget" >&2
	exit 1
fi

DIRECT_URLS=(
	"https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad"
	"https://raw.githubusercontent.com/Doom-Utils/shareware-collection/master/Doom%201.0/doom1.wad"
	"https://archive.org/download/DoomsharewareEpisode/doom1.wad"
)
ZIP_URL="https://www.quaddicted.com/files/idgames/idstuff/doom/doom19s.zip"
TMP_WAD="$WAD_DIR/.doom1.wad.tmp"
TMP_ZIP="$WAD_DIR/.doom19s.zip.tmp"

# shellcheck disable=SC2329  # Invoked via trap.
cleanup() {
	rm -f "$TMP_WAD" "$TMP_ZIP"
}
trap cleanup EXIT

download_to_file() {
	local url="$1"
	local out="$2"
	if command -v curl >/dev/null 2>&1; then
		curl -L --fail --retry 3 --progress-bar -o "$out" "$url" >/dev/null
	else
		wget -q --tries 3 -O "$out" "$url"
	fi
}

validate_wad_size() {
	local file="$1"
	local size
	size=$(stat -c%s "$file" 2>/dev/null || stat -f%z "$file" 2>/dev/null || echo "0")
	[[ "$size" -ge 4000000 ]]
}

echo "Downloading doom1.wad..."
for url in "${DIRECT_URLS[@]}"; do
	echo "Trying: $url"
	rm -f "$TMP_WAD"
	if download_to_file "$url" "$TMP_WAD" && validate_wad_size "$TMP_WAD"; then
		mv "$TMP_WAD" "$WAD_FILE"
		echo "Downloaded: $WAD_FILE"
		exit 0
	fi
done

if command -v unzip >/dev/null 2>&1; then
	echo "Trying zip fallback: $ZIP_URL"
	if download_to_file "$ZIP_URL" "$TMP_ZIP" &&
		unzip -p "$TMP_ZIP" doom1.wad >"$TMP_WAD" 2>/dev/null &&
		validate_wad_size "$TMP_WAD"; then
		mv "$TMP_WAD" "$WAD_FILE"
		echo "Downloaded (zip fallback): $WAD_FILE"
		exit 0
	fi
else
	echo "Skipping zip fallback (unzip not found)"
fi

echo "error: failed to download doom1.wad from all sources" >&2
echo "manual source: https://archive.org/details/DoomsharewareEpisode" >&2
exit 1
