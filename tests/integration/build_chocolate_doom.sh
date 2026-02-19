#!/usr/bin/env bash
# build_chocolate_doom.sh - Build Chocolate Doom with DMCP enabled
#
# Usage:
#   build_chocolate_doom.sh [OPTIONS]
#
# Options:
#   -j, --jobs N      Parallel build jobs (default: auto)
#   -h, --help        Show this help
#
# Environment:
#   DMCP_ROOT                Repo root (default: auto-detected)
#   DMCP_BUILD_DIR           DMCP CMake build dir (default: <root>/build)
#   CHOCOLATE_BUILD_DIR      Chocolate Doom build dir (default: <root>/chocolate-doom/build)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMCP_ROOT="${DMCP_ROOT:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
DMCP_BUILD_DIR="${DMCP_BUILD_DIR:-$DMCP_ROOT/build}"
CHOCOLATE_BUILD_DIR="${CHOCOLATE_BUILD_DIR:-$DMCP_ROOT/chocolate-doom/build}"
JOBS="${JOBS:-$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"

usage() {
	cat <<EOF
Build Chocolate Doom with DMCP enabled.

Usage:
  $(basename "$0") [OPTIONS]

Options:
  -j, --jobs N      Parallel build jobs (default: $JOBS)
  -h, --help        Show this help
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
	-j | --jobs)
		if [[ $# -lt 2 ]]; then
			echo "error: --jobs requires a value" >&2
			exit 2
		fi
		JOBS="$2"
		shift 2
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

if [[ ! -d "$DMCP_ROOT/chocolate-doom" ]]; then
	echo "error: chocolate-doom directory not found at $DMCP_ROOT/chocolate-doom" >&2
	exit 1
fi

echo "==> Configuring DMCP core build"
cmake -B "$DMCP_BUILD_DIR" \
	-DDMCP_BUILD_TESTS=ON \
	-DDMCP_BUILD_EXAMPLES=ON

echo "==> Building DMCP core"
cmake --build "$DMCP_BUILD_DIR" -j"$JOBS"

echo "==> Configuring Chocolate Doom with DMCP"
cmake -S "$DMCP_ROOT/chocolate-doom" -B "$CHOCOLATE_BUILD_DIR" \
	-DDMCP_ENABLE=ON \
	-DDMCP_INCLUDE_DIR="$DMCP_ROOT/include" \
	-DDMCP_LIB_DIR="$DMCP_BUILD_DIR"

echo "==> Building Chocolate Doom"
cmake --build "$CHOCOLATE_BUILD_DIR" -j"$JOBS"

DOOM_BIN="$CHOCOLATE_BUILD_DIR/src/chocolate-doom"
if [[ ! -x "$DOOM_BIN" ]]; then
	echo "error: build finished but binary not found at $DOOM_BIN" >&2
	exit 1
fi

echo "==> Chocolate Doom ready: $DOOM_BIN"
