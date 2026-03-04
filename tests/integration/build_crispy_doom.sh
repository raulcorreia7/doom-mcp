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
#   DMCP_KEEP_CRISPY_PATCH   Keep patch applied after build: 1 keep, 0 auto-revert (default)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMCP_ROOT="${DMCP_ROOT:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
DMCP_BUILD_DIR="${DMCP_BUILD_DIR:-$DMCP_ROOT/build/default}"
CRISPY_BUILD_DIR="${CRISPY_BUILD_DIR:-$DMCP_ROOT/crispy-doom/build}"
PATCH_FILE="$DMCP_ROOT/adapters/crispy-doom/patches/dmcp_integration.patch"
KEEP_PATCH="${DMCP_KEEP_CRISPY_PATCH:-0}"
PATCH_APPLIED_NOW=0

cleanup() {
	if [[ "$KEEP_PATCH" != "1" && "$PATCH_APPLIED_NOW" == "1" ]]; then
		git -C "$DMCP_ROOT/crispy-doom" apply --reverse "$PATCH_FILE" >/dev/null 2>&1 || true
		echo "==> Restored Crispy source tree to clean state"
	fi
}

trap cleanup EXIT

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

if [[ ! -f "$PATCH_FILE" ]]; then
	echo "error: patch file not found at $PATCH_FILE" >&2
	exit 1
fi

if git -C "$DMCP_ROOT/crispy-doom" apply --check "$PATCH_FILE" >/dev/null 2>&1; then
	PATCH_APPLIED_NOW=1
fi

echo "==> Applying Crispy Doom DMCP patch"
"$SCRIPT_DIR/apply_crispy_dmcp_patch.sh"

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
	-DDMCP_BUILD_ADAPTERS=ON \
	-DDMCP_BUILD_ADAPTER_CRISPY=ON

echo "==> Building DMCP core"
cmake --build "$DMCP_BUILD_DIR" --parallel

echo "==> Configuring Crispy Doom with DMCP"
cmake -S "$DMCP_ROOT/crispy-doom" -B "$CRISPY_BUILD_DIR" \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DDMCP_ROOT="$DMCP_ROOT" \
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
