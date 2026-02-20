#!/usr/bin/env bash
# build_crispy_doom.sh - Build Crispy Doom with DMCP enabled
#
# Usage:
#   build_crispy_doom.sh [OPTIONS]
#
# Options:
#   -j, --jobs N      Parallel build jobs (default: auto)
#   -h, --help        Show this help
#
# Environment:
#   DMCP_ROOT                Repo root (default: auto-detected)
#   DMCP_BUILD_DIR           DMCP CMake build dir (default: <root>/build)
#   CRISPY_BUILD_DIR         Crispy Doom build dir (default: <root>/crispy-doom/build)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMCP_ROOT="${DMCP_ROOT:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
DMCP_BUILD_DIR="${DMCP_BUILD_DIR:-$DMCP_ROOT/build}"
CRISPY_BUILD_DIR="${CRISPY_BUILD_DIR:-$DMCP_ROOT/crispy-doom/build}"
JOBS="${JOBS:-$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"

usage() {
	cat <<EOF
Build Crispy Doom with DMCP enabled.

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

if [[ ! -d "$DMCP_ROOT/crispy-doom" ]]; then
	echo "error: crispy-doom directory not found at $DMCP_ROOT/crispy-doom" >&2
	exit 1
fi

echo "==> Applying Crispy Doom DMCP patch"
"$SCRIPT_DIR/apply_crispy_dmcp_patch.sh"

echo "==> Configuring DMCP core build"
cmake -B "$DMCP_BUILD_DIR" \
	-DDMCP_BUILD_TESTS=ON \
	-DDMCP_BUILD_EXAMPLES=ON

echo "==> Building DMCP core"
cmake --build "$DMCP_BUILD_DIR" -j"$JOBS"

echo "==> Configuring Crispy Doom with DMCP"
cmake -S "$DMCP_ROOT/crispy-doom" -B "$CRISPY_BUILD_DIR" \
	-DDMCP_ENABLE=ON \
	-DDMCP_INCLUDE_DIR="$DMCP_ROOT/include" \
	-DDMCP_LIB_DIR="$DMCP_BUILD_DIR"

echo "==> Building Crispy Doom"
cmake --build "$CRISPY_BUILD_DIR" -j"$JOBS"

DOOM_BIN="$CRISPY_BUILD_DIR/src/crispy-doom"
if [[ ! -x "$DOOM_BIN" ]]; then
	echo "error: build finished but binary not found at $DOOM_BIN" >&2
	exit 1
fi

echo "==> Crispy Doom ready: $DOOM_BIN"
