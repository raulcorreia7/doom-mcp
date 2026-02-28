#!/usr/bin/env bash
# apply_crispy_dmcp_patch.sh - Apply DMCP integration patch to Crispy Doom
#
# Usage:
#   apply_crispy_dmcp_patch.sh
#
# Environment:
#   DMCP_ROOT  Repo root (default: auto-detected)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMCP_ROOT="${DMCP_ROOT:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
CRISPY_DIR="$DMCP_ROOT/crispy-doom"
PATCH_FILE="$DMCP_ROOT/adapters/crispy-doom/patches/dmcp_integration.patch"

if [[ ! -e "$CRISPY_DIR/.git" ]]; then
	echo "error: crispy-doom submodule not initialized at $CRISPY_DIR" >&2
	echo "hint: run 'git submodule update --init --recursive'" >&2
	exit 1
fi

# Check if already patched (DMCP_Init present in source)
if grep -q "DMCP_Init" "$CRISPY_DIR/src/doom/d_main.c" 2>/dev/null; then
	echo "DMCP integration already present in source"
	exit 0
fi

if [[ ! -f "$PATCH_FILE" ]]; then
	echo "error: patch file not found: $PATCH_FILE" >&2
	exit 1
fi

if git -C "$CRISPY_DIR" apply --check "$PATCH_FILE" >/dev/null 2>&1; then
	git -C "$CRISPY_DIR" apply "$PATCH_FILE"
	echo "Applied Crispy Doom DMCP patch"
	exit 0
fi

if git -C "$CRISPY_DIR" apply --reverse --check "$PATCH_FILE" >/dev/null 2>&1; then
	echo "Crispy Doom DMCP patch already applied"
	exit 0
fi

echo "error: Crispy Doom tree does not match expected patch base" >&2
echo "hint: reset submodule to a clean checkout and retry" >&2
exit 1
