#!/usr/bin/env bash
# validate.sh - Fast project validation matrix for core-first + opt-in adapters.
#
# Usage:
#   ./scripts/validate.sh [build-root]
#
# Default build root:
#   ./build/validate

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_ROOT="${1:-${DMCP_VALIDATE_BUILD_ROOT:-$REPO_ROOT/build/validate}}"

step() {
  printf '\n==> %s\n' "$1"
}

fail() {
  printf 'error: %s\n' "$1" >&2
  exit 1
}

configure_build() {
  local dir="$1"
  shift
  cmake -S "$REPO_ROOT" -B "$dir" "$@"
  cmake --build "$dir" --parallel
}

assert_not_exists() {
  local path="$1"
  if [[ -e "$path" ]]; then
    fail "unexpected artifact exists: $path"
  fi
}

assert_exists() {
  local path="$1"
  if [[ ! -e "$path" ]]; then
    fail "expected artifact missing: $path"
  fi
}

assert_no_default_adapter_artifacts() {
  local dir="$1"

  local found=""
  found="$(find "$dir" -type f \
    \( -name "libdmcp_fake*" -o -name "libdmcp_zdoom*" -o -name "libdmcp_crispy*" \
       -o -name "dmcp_fake.*" -o -name "dmcp_zdoom.*" -o -name "dmcp_crispy.*" \) \
    -print -quit)"

  if [[ -n "$found" ]]; then
    fail "default core build produced adapter artifact: $found"
  fi
}

step "Preparing build root: $BUILD_ROOT"
rm -rf "$BUILD_ROOT"
mkdir -p "$BUILD_ROOT"

CORE_DIR="$BUILD_ROOT/core-default"
TEST_DIR="$BUILD_ROOT/unit-tests"
EXAMPLES_DIR="$BUILD_ROOT/examples-on"
FAKE_DIR="$BUILD_ROOT/fake-adapter-on"

step "Validating default core-only build"
configure_build "$CORE_DIR"
assert_not_exists "$CORE_DIR/dummy_server"
assert_no_default_adapter_artifacts "$CORE_DIR"

step "Validating unit test build"
configure_build "$TEST_DIR" -DDMCP_BUILD_TESTS=ON
ctest --test-dir "$TEST_DIR" -L unit -j1 --output-on-failure

step "Validating examples as explicit opt-in"
configure_build "$EXAMPLES_DIR" -DDMCP_BUILD_EXAMPLES=ON
assert_exists "$EXAMPLES_DIR/dummy_server"

step "Validating fake adapter as explicit opt-in"
configure_build "$FAKE_DIR" \
  -DDMCP_BUILD_ADAPTERS=ON \
  -DDMCP_BUILD_ADAPTER_FAKE=ON \
  -DDMCP_BUILD_ADAPTER_ZDOOM=OFF \
  -DDMCP_BUILD_ADAPTER_CRISPY=OFF

printf '\nValidation matrix completed successfully.\n'
