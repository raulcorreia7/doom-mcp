#!/usr/bin/env bash
# download_wad.sh - Download the DOOM shareware WAD used by integration/e2e tests
#
# Usage:
#   download_wad.sh [OPTIONS]
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
WAD_DIR="$(dirname "$WAD_FILE")"

FORCE=0

usage() {
	cat <<EOF
Download the DOOM shareware WAD (doom1.wad) used by DMCP tests.

Usage:
  $(basename "$0") [OPTIONS]

Options:
  -f, --force      Re-download even if the file already exists
  -h, --help       Show this help
EOF
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

if ! command -v curl >/dev/null 2>&1; then
	echo "error: curl is required to download doom1.wad" >&2
	exit 1
fi

mkdir -p "$WAD_DIR"

if [[ -f "$WAD_FILE" && "$FORCE" -eq 0 ]]; then
	echo "doom1.wad already exists: $WAD_FILE"
	echo "size: $(du -h "$WAD_FILE" | cut -f1)"
	exit 0
fi

if [[ "$FORCE" -eq 1 ]]; then
	rm -f "$WAD_FILE"
fi

DIRECT_URLS=(
	"https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad"
)

ZIP_URL="https://www.quaddicted.com/files/idgames/idstuff/doom/doom19s.zip"
TMP_WAD="$WAD_DIR/.doom1.wad.tmp"
TMP_ZIP="$WAD_DIR/.doom19s.zip.tmp"

cleanup() {
	rm -f "$TMP_WAD" "$TMP_ZIP"
}
trap cleanup EXIT

download_direct() {
	local url
	for url in "${DIRECT_URLS[@]}"; do
		echo "Trying: $url"
		if curl -L --fail --progress-bar -o "$TMP_WAD" "$url"; then
			mv "$TMP_WAD" "$WAD_FILE"
			return 0
		fi
		echo "Failed: $url"
	done
	return 1
}

download_from_zip() {
	if ! command -v unzip >/dev/null 2>&1; then
		echo "Skipping zip fallback (unzip not found)"
		return 1
	fi

	echo "Trying: $ZIP_URL"
	if ! curl -L --fail --progress-bar -o "$TMP_ZIP" "$ZIP_URL"; then
		return 1
	fi

	if ! unzip -p "$TMP_ZIP" doom1.wad >"$TMP_WAD" 2>/dev/null; then
		return 1
	fi

	mv "$TMP_WAD" "$WAD_FILE"
	return 0
}

echo "Downloading DOOM shareware WAD..."
if ! download_direct && ! download_from_zip; then
	echo "error: failed to download doom1.wad from all sources" >&2
	echo "place file manually at: $WAD_FILE" >&2
	exit 1
fi

SIZE=$(stat -c%s "$WAD_FILE" 2>/dev/null || stat -f%z "$WAD_FILE")
if [[ "$SIZE" -lt 4000000 ]]; then
	echo "warning: downloaded file looks small ($SIZE bytes)" >&2
fi

echo "Downloaded: $WAD_FILE"
echo "Size: $(du -h "$WAD_FILE" | cut -f1)"
