#!/usr/bin/env bash
# build_crispy_doom.sh - Build Crispy Doom with DMCP enabled
#
# Usage:
#   build_crispy_doom.sh [OPTIONS]
#
# Options:
#   -h, --help        Show this help
#
# Environment:
#   DMCP_ROOT                Repo root (default: auto-detected)
#   DMCP_BUILD_DIR           DMCP CMake build dir (default: <root>/build/default)
#   CRISPY_BUILD_DIR         Crispy Doom build dir (default: <root>/crispy-doom/build)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMCP_ROOT="${DMCP_ROOT:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
DMCP_BUILD_DIR="${DMCP_BUILD_DIR:-$DMCP_ROOT/build/default}"
CRISPY_BUILD_DIR="${CRISPY_BUILD_DIR:-$DMCP_ROOT/crispy-doom/build}"

usage() {
	cat <<EOF
Build Crispy Doom with DMCP enabled.

Usage:
  $(basename "$0") [OPTIONS]

Options:
  -h, --help        Show this help
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
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

if ! grep -q "DMCP_Init" "$DMCP_ROOT/crispy-doom/src/doom/d_main.c" ||
	! grep -q "DMCP_Tick" "$DMCP_ROOT/crispy-doom/src/doom/g_game.c" ||
	! grep -q "DMCP_ENABLE" "$DMCP_ROOT/crispy-doom/CMakeLists.txt"; then
	echo "error: crispy-doom submodule is missing the checked-in DMCP integration" >&2
	echo "hint: run 'git submodule update --init --recursive' and ensure the pinned DMCP fork is checked out" >&2
	exit 1
fi

echo "==> Configuring Crispy Doom (pre-build for headers)"
cmake -S "$DMCP_ROOT/crispy-doom" -B "$CRISPY_BUILD_DIR" \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DDMCP_ENABLE=OFF

echo "==> Configuring DMCP core build (shared)"
cmake -B "$DMCP_BUILD_DIR" \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DDMCP_BUILD_SHARED=ON \
	-DDMCP_BUILD_SINGLE_DLL=ON \
	-DDMCP_BUILD_TESTS=OFF \
	-DDMCP_BUILD_EXAMPLES=OFF \
	-DDMCP_BUILD_ADAPTER_CRISPY=OFF \
	-DDMCP_BUILD_ADAPTER_ZDOOM=OFF \
	-DDMCP_BUILD_ADAPTER_FAKE=OFF

echo "==> Building DMCP core"
cmake --build "$DMCP_BUILD_DIR" --parallel

echo "==> Configuring Crispy Doom with DMCP"
cmake -S "$DMCP_ROOT/crispy-doom" -B "$CRISPY_BUILD_DIR" \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DDMCP_ROOT="$DMCP_ROOT" \
	-DDMCP_BUILD_DIR="$DMCP_BUILD_DIR" \
	-DDMCP_LIBRARY_DIR="$DMCP_BUILD_DIR" \
	-DDMCP_ENABLE=ON

echo "==> Building Crispy Doom"
cmake --build "$CRISPY_BUILD_DIR" --parallel

DOOM_BIN="$CRISPY_BUILD_DIR/src/crispy-doom"
if [[ ! -x "$DOOM_BIN" ]]; then
	echo "error: build finished but binary not found at $DOOM_BIN" >&2
	exit 1
fi

echo "==> Crispy Doom ready: $DOOM_BIN"

if [[ -x "$DMCP_ROOT/scripts/refresh_compdb.sh" ]]; then
	echo "==> Refreshing compile_commands.json for LSP"
	"$DMCP_ROOT/scripts/refresh_compdb.sh" "$DMCP_BUILD_DIR" "$CRISPY_BUILD_DIR"
fi
