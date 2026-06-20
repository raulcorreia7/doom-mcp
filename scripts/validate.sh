#!/usr/bin/env bash
# validate.sh - Fast no-game validation matrix for core-first + opt-in adapters.
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
  command -v cmake >/dev/null 2>&1 || fail "cmake is required"
  cmake -S "$REPO_ROOT" -B "$dir" "$@"
  cmake --build "$dir" --parallel
}

run_unit_tests() {
  local dir="$1"
  ctest --test-dir "$dir" -L unit -j1 --timeout 60 --output-on-failure
}

run_no_game_integration_tests() {
  local dir="$1"
  ctest --test-dir "$dir" -L integration -LE requires_game -j1 --timeout 60 --output-on-failure
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
SHARED_DIR="$BUILD_ROOT/shared-runtime"
EXAMPLES_DIR="$BUILD_ROOT/examples-on"
FAKE_DIR="$BUILD_ROOT/fake-adapter-on"

step "Validating default core-only build"
configure_build "$CORE_DIR"
assert_not_exists "$CORE_DIR/dummy_server"
assert_no_default_adapter_artifacts "$CORE_DIR"

step "Validating unit test build"
configure_build "$TEST_DIR" -DDMCP_BUILD_TESTS=ON
run_unit_tests "$TEST_DIR"

step "Validating unified shared runtime build"
configure_build "$SHARED_DIR" \
  -DDMCP_BUILD_SHARED=ON \
  -DDMCP_BUILD_SINGLE_DLL=ON \
  -DDMCP_BUILD_TESTS=OFF
if [[ "$OSTYPE" == "darwin"* ]]; then
  assert_exists "$SHARED_DIR/libdmcp.dylib"
elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "win32"* ]]; then
  if [[ -f "$SHARED_DIR/dmcp.dll" ]]; then
    assert_exists "$SHARED_DIR/dmcp.dll"
  else
    assert_exists "$SHARED_DIR/Release/dmcp.dll"
  fi
else
  assert_exists "$SHARED_DIR/libdmcp.so"
fi
assert_no_default_adapter_artifacts "$SHARED_DIR"

step "Validating examples as explicit opt-in"
configure_build "$EXAMPLES_DIR" -DDMCP_BUILD_EXAMPLES=ON
assert_exists "$EXAMPLES_DIR/dummy_server"

step "Validating fake adapter as explicit opt-in without launching a game"
configure_build "$FAKE_DIR" \
  -DDMCP_BUILD_TESTS=ON \
  -DDMCP_BUILD_INTEGRATION_TESTS=ON \
  -DDMCP_BUILD_ADAPTER_FAKE=ON \
  -DDMCP_BUILD_ADAPTER_ZDOOM=OFF \
  -DDMCP_BUILD_ADAPTER_CRISPY=OFF
run_unit_tests "$FAKE_DIR"
run_no_game_integration_tests "$FAKE_DIR"

printf '\nNo-game validation matrix completed successfully.\n'
