#!/usr/bin/env bash
#
# Purpose: Verify the optional Python agent helper without launching a game.
# Dependencies: bash, python, uv

set -Eeuo pipefail
IFS=$'\n\t'

readonly PROGRAM_NAME="${0##*/}"
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

agent_dir="$REPO_ROOT/examples/agents/python"
python_bin="${PYTHON:-}"

usage() {
  cat <<EOF
Usage:
  $PROGRAM_NAME [--agent-dir DIR]

Verify the optional Python dmcp-agent helper without connecting to a server.

Options:
      --agent-dir DIR  Python agent project directory.
  -h, --help           Show this help and exit.

Exit status:
  0  Success.
  1  Runtime failure.
  2  Usage or validation error.
EOF
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

die_usage() {
  printf 'error: %s\n\n' "$*" >&2
  printf "Try '%s --help' for usage.\n" "$PROGRAM_NAME" >&2
  exit 2
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || die "missing required command: $1"
}

parse_args() {
  while [[ "$#" -gt 0 ]]; do
    case "$1" in
      --agent-dir)
        [[ "${2:-}" ]] || die_usage "$1 requires DIR"
        agent_dir="$2"
        shift 2
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      -*)
        die_usage "unknown option: $1"
        ;;
      *)
        die_usage "unexpected argument: $1"
        ;;
    esac
  done
}

select_python() {
  if [[ -n "$python_bin" ]]; then
    require_command "$python_bin"
    return
  fi

  if command -v python3 >/dev/null 2>&1; then
    python_bin="python3"
  elif command -v python >/dev/null 2>&1; then
    python_bin="python"
  else
    die "missing required command: python"
  fi
}

main() {
  parse_args "$@"
  [[ -d "$agent_dir" ]] || die "agent directory not found: $agent_dir"
  select_python
  require_command uv

  grep -F 'dmcp-agent = "dmcp_agent.cli:main"' "$agent_dir/pyproject.toml" >/dev/null ||
    die "pyproject.toml is missing the dmcp-agent entry point"

  "$python_bin" -m compileall -q "$agent_dir/dmcp_agent"
  uv run --project "$agent_dir" --frozen dmcp-agent --help >/dev/null
}

main "$@"
