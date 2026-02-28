#!/usr/bin/env bash
# download_wad.sh - Download Doom shareware WAD
#
# Usage: ./download_wad.sh [OUTPUT_DIR]
#
# Downloads doom1.wad (shareware) to OUTPUT_DIR (default: current dir).
# Requires curl or wget.

set -euo pipefail

OUT_DIR="${1:-.}"
OUT_FILE="${OUT_DIR}/doom1.wad"

mkdir -p "${OUT_DIR}"

# Check if already exists and is valid (>=4MB)
if [[ -f "${OUT_FILE}" ]]; then
	size=$(stat -f%z "$OUT_FILE" 2>/dev/null || stat -c%s "$OUT_FILE" 2>/dev/null || echo "0")
	if [[ "$size" -ge 4000000 ]]; then
		echo "doom1.wad already exists ($size bytes)"
		exit 0
	fi
	rm -f "${OUT_FILE}"
fi

# URLs to try
urls=(
	"https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad"
	"https://raw.githubusercontent.com/Doom-Utils/shareware-collection/master/Doom%201.0/doom1.wad"
	"https://archive.org/download/DoomsharewareEpisode/doom1.wad"
)

# Download function
download() {
	local url="$1"
	if command -v curl &>/dev/null; then
		curl -fsSL --retry 3 -o "${OUT_FILE}" "${url}" 2>/dev/null
	elif command -v wget &>/dev/null; then
		wget -q --tries 3 -O "${OUT_FILE}" "${url}" 2>/dev/null
	fi
}

echo "Downloading doom1.wad..."
for url in "${urls[@]}"; do
	rm -f "${OUT_FILE}" 2>/dev/null
	if download "$url"; then
		size=$(stat -f%z "$OUT_FILE" 2>/dev/null || stat -c%s "$OUT_FILE" 2>/dev/null || echo "0")
		if [[ "$size" -ge 4000000 ]]; then
			echo "Downloaded: ${OUT_FILE}"
			exit 0
		fi
	fi
done

echo "Failed to download. Get doom1.wad from:"
echo "  https://archive.org/details/DoomsharewareEpisode"
exit 1
