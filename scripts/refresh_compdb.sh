#!/usr/bin/env bash
# refresh_compdb.sh - Generate root compile_commands.json for LSP/clangd.
#
# Usage:
#   ./scripts/refresh_compdb.sh [DMCP_BUILD_DIR] [CRISPY_BUILD_DIR]
#
# Behavior:
# - Reads <DMCP_BUILD_DIR>/compile_commands.json (required)
# - Reads <CRISPY_BUILD_DIR>/compile_commands.json (optional)
# - Writes merged output to <repo>/compile_commands.json

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

DMCP_BUILD_DIR="${1:-${DMCP_BUILD_DIR:-$REPO_ROOT/build/default}}"
CRISPY_BUILD_DIR="${2:-${CRISPY_BUILD_DIR:-$REPO_ROOT/crispy-doom/build}}"

DMCP_DB="$DMCP_BUILD_DIR/compile_commands.json"
CRISPY_DB="$CRISPY_BUILD_DIR/compile_commands.json"
OUT_DB="$REPO_ROOT/compile_commands.json"

if [[ ! -f "$DMCP_DB" ]]; then
	echo "error: missing compile database: $DMCP_DB" >&2
	exit 1
fi

if command -v jq >/dev/null 2>&1; then
	if [[ -f "$CRISPY_DB" ]]; then
		jq -s '
      def merged_entries:
        (map(select(type == "array")) | add // []);
      def absfile:
        if (.file | startswith("/")) then .file
        else ((.directory // ".") + "/" + .file)
        end;
      merged_entries
      | map(select(type == "object"))
      | unique_by(absfile)
    ' "$DMCP_DB" "$CRISPY_DB" >"$OUT_DB"
	else
		cp "$DMCP_DB" "$OUT_DB"
	fi
else
	echo "warn: jq not found; linking DMCP compile database only" >&2
	ln -sf "$DMCP_DB" "$OUT_DB"
fi

echo "Updated $OUT_DB"
