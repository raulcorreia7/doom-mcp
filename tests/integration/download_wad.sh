#!/bin/bash
# Download Doom Shareware WAD (doom1.wad) for testing
# This is the free shareware episode (Knee-Deep in the Dead)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WAD_DIR="$SCRIPT_DIR/wads"
WAD_FILE="$WAD_DIR/doom1.wad"

# Create wads directory
mkdir -p "$WAD_DIR"

# Check if already downloaded
if [ -f "$WAD_FILE" ]; then
	echo "doom1.wad already exists at $WAD_FILE"
	echo "Size: $(du -h "$WAD_FILE" | cut -f1)"
	exit 0
fi

echo "Downloading DOOM Shareware (doom1.wad)..."

# Try multiple mirrors
MIRRORS=(
	"https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad"
	"https://www.quaddicted.com/files/idgames/idstuff/doom/doom1.wad"
	"https://archive.org/download/Doomshare/doom1.wad"
	"https://ftp.gwdg.de/pub/idgames/idstuff/doom/doom1.wad"
)

DOWNLOADED=0
for MIRROR in "${MIRRORS[@]}"; do
	echo "Trying: $MIRROR"
	if curl -L --fail --progress-bar -o "$WAD_FILE" "$MIRROR" 2>/dev/null; then
		DOWNLOADED=1
		break
	fi
	echo "Failed, trying next mirror..."
done

if [ $DOWNLOADED -eq 0 ]; then
	echo "ERROR: Failed to download doom1.wad from all mirrors"
	echo "Please download manually and place at: $WAD_FILE"
	exit 1
fi

# Verify file size (shareware WAD is ~4.2MB)
SIZE=$(stat -c%s "$WAD_FILE" 2>/dev/null || stat -f%z "$WAD_FILE" 2>/dev/null)
if [ "$SIZE" -lt 4000000 ]; then
	echo "WARNING: Downloaded file seems too small ($SIZE bytes)"
	echo "Expected ~4.2MB for doom1.wad"
fi

echo ""
echo "✓ Downloaded doom1.wad successfully"
echo "  Location: $WAD_FILE"
echo "  Size: $(du -h "$WAD_FILE" | cut -f1)"
echo ""
echo "This is the shareware version containing Episode 1: Knee-Deep in the Dead"
