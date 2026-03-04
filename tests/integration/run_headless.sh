#!/usr/bin/env bash
# Run headless Doom engine with DMCP for integration/e2e checks.
# Supported engine: crispy-doom

set -euo pipefail

# -----------------------------------------------------------------------------
# Paths and runtime configuration
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DMCP_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

WAD_DIR="$DMCP_ROOT/assets/wads"
WAD_FILE="$WAD_DIR/doom1.wad"
DOOM_ENGINE="${DOOM_ENGINE:-crispy}"
ENGINE_NAME="Crispy Doom"

BUILD_DIR="${DOOM_BUILD_DIR:-$DMCP_ROOT/crispy-doom/build}"
DOOM_BIN="${DOOM_BIN:-$BUILD_DIR/src/crispy-doom}"

DMCP_PORT="${DMCP_PORT:-6060}"
BASE_URL="http://localhost:$DMCP_PORT"
MCP_URL="$BASE_URL/mcp"
MCP_PROTOCOL_VERSION="${MCP_PROTOCOL_VERSION:-2025-11-25}"

MAX_START_ATTEMPTS="${DMCP_START_ATTEMPTS:-5}"
STARTUP_WAIT_SECONDS="${DMCP_STARTUP_WAIT_SECONDS:-30}"
HTTP_TIMEOUT="${DMCP_HTTP_TIMEOUT:-5}"

DOOM_PID=""

# -----------------------------------------------------------------------------
# Console output helpers
# -----------------------------------------------------------------------------

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass() { echo -e "${GREEN}✓ $1${NC}"; }
warn() { echo -e "${YELLOW}! $1${NC}"; }
fail() {
	echo -e "${RED}✗ $1${NC}"
	exit 1
}

test_title() {
	echo ""
	echo "$1"
}

# -----------------------------------------------------------------------------
# Process lifecycle
# -----------------------------------------------------------------------------

cleanup() {
	if [ -n "$DOOM_PID" ] && kill -0 "$DOOM_PID" 2>/dev/null; then
		kill "$DOOM_PID" 2>/dev/null || true
		wait "$DOOM_PID" 2>/dev/null || true
	fi
}

trap cleanup EXIT

# -----------------------------------------------------------------------------
# HTTP / JSON-RPC helpers
# -----------------------------------------------------------------------------

pretty_json() {
	if command -v jq >/dev/null 2>&1; then
		jq . 2>/dev/null || cat
	else
		cat
	fi
}

print_game_state_pretty() {
	if ! command -v jq >/dev/null 2>&1; then
		cat
		return 0
	fi

	local raw
	raw="$(cat)"

	local nested
	nested="$(printf '%s\n' "$raw" | jq -c '
		((.content // .result.content // [])[]? | select(.type == "text") | .text | fromjson? // empty)
		| select(type == "object")
	' 2>/dev/null | head -n1 || true)"

	if [ -n "$nested" ]; then
		printf '%s\n' "$nested" | jq .
		return 0
	fi

	printf '%s\n' "$raw" | jq . 2>/dev/null || printf '%s\n' "$raw"
}

game_state_has_player() {
	if ! command -v jq >/dev/null 2>&1; then
		return 1
	fi

	jq -e '
		[((.content // .result.content // [])[]? | select(.type == "text") | .text | fromjson?)]
		| any(type == "object" and has("player"))
	' >/dev/null 2>&1
}

health_get() {
	curl -sS -m "$HTTP_TIMEOUT" "$BASE_URL/health"
}

mcp_post() {
	local payload="$1"
	local session_id="${2:-}"
	local protocol_version="${3:-$MCP_PROTOCOL_VERSION}"

	local args=(
		-sS
		-m "$HTTP_TIMEOUT"
		-X POST "$MCP_URL"
		-H "Content-Type: application/json"
		-H "MCP-Protocol-Version: $protocol_version"
		-d "$payload"
	)

	if [ -n "$session_id" ]; then
		args+=(-H "MCP-Session-Id: $session_id")
	fi

	curl "${args[@]}"
}

extract_session_id() {
	local init_json="$1"
	if command -v jq >/dev/null 2>&1; then
		printf '%s\n' "$init_json" | jq -r '.result.sessionId // empty'
		return 0
	fi
	printf '%s\n' "$init_json" | grep -o '"sessionId":"[^"]*"' | cut -d'"' -f4
}

assert_contains_json() {
	local response="$1"
	local needle="$2"
	local ok_message="$3"
	local fail_message="$4"

	if printf '%s\n' "$response" | grep -q "$needle"; then
		pass "$ok_message"
		return 0
	fi

	echo "$fail_message:"
	printf '%s\n' "$response" | pretty_json
	fail "$fail_message"
}

assert_contains_text() {
	local response="$1"
	local needle="$2"
	local ok_message="$3"
	local fail_message="$4"

	if printf '%s\n' "$response" | grep -q "$needle"; then
		pass "$ok_message"
		return 0
	fi

	echo "$fail_message:"
	printf '%s\n' "$response"
	fail "$fail_message"
}

wait_for_server_ready() {
	local wait_secs=0
	while [ "$wait_secs" -lt "$STARTUP_WAIT_SECONDS" ]; do
		if health_get >/dev/null 2>&1; then
			return 0
		fi

		if ! kill -0 "$DOOM_PID" 2>/dev/null; then
			return 1
		fi

		sleep 1
		wait_secs=$((wait_secs + 1))
	done

	return 1
}

start_doom_with_retry() {
	for attempt in $(seq 1 "$MAX_START_ATTEMPTS"); do
		SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$DOOM_BIN" \
			-iwad "$WAD_FILE" \
			-nodraw \
			-nosound \
			-nomusic \
			-nosfx \
			-nograb \
			-warp 1 1 \
			-skill 3 \
			-dmcp_port "$DMCP_PORT" \
			</dev/null &
		DOOM_PID=$!

		echo "PID: $DOOM_PID (attempt $attempt/$MAX_START_ATTEMPTS)"
		echo "Waiting for DMCP server..."

		if wait_for_server_ready; then
			pass "DMCP server is running"
			return 0
		fi

		warn "Startup attempt $attempt failed"
		if kill -0 "$DOOM_PID" 2>/dev/null; then
			kill "$DOOM_PID" 2>/dev/null || true
			wait "$DOOM_PID" 2>/dev/null || true
		fi
		DOOM_PID=""

		if [ "$attempt" -eq "$MAX_START_ATTEMPTS" ]; then
			fail "DMCP server did not start within retry budget"
		fi

		sleep 1
	done
}

# -----------------------------------------------------------------------------
# Main flow
# -----------------------------------------------------------------------------

echo "=== DMCP Headless Test Runner ==="
echo ""

if [ "$DOOM_ENGINE" != "crispy" ]; then
	fail "Unsupported DOOM_ENGINE '$DOOM_ENGINE' (use 'crispy')"
fi

# Step 1: Download WAD if needed
if [ ! -f "$WAD_FILE" ]; then
	echo "Downloading shareware WAD..."
	"$SCRIPT_DIR/download_wad.sh"
fi

# Step 2: Check engine binary
if [ ! -f "$DOOM_BIN" ]; then
	warn "$ENGINE_NAME not built yet"
	echo ""
	echo "To build $ENGINE_NAME with DMCP:"
	echo "  1. make submodules"
	echo "  2. ./tests/integration/build_crispy_doom.sh"
	exit 1
fi

# Step 3: Start headless engine
echo "Starting $ENGINE_NAME (headless)..."
echo "  WAD: $WAD_FILE"
echo "  DMCP Port: $DMCP_PORT"
echo "  Log: output not redirected (stability)"
echo ""

start_doom_with_retry

# Step 4: Run protocol and route checks
echo ""
echo "Running MCP protocol tests..."

# Test 1: Health check
test_title "Test 1: Health check"
HEALTH="$(health_get)"
assert_contains_json "$HEALTH" '"status":"ok"' "Health check succeeded" "Health check failed"
printf '%s\n' "$HEALTH" | pretty_json

# Test 2: Unsupported protocol rejection
test_title "Test 2: MCP Initialize rejects unsupported protocol"
INIT_UNSUPPORTED="$(mcp_post \
	'{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-01-01","capabilities":{},"clientInfo":{"name":"dmcp-headless-test","version":"0.6.0"}}}' \
	"" \
	"2024-01-01")"
assert_contains_json "$INIT_UNSUPPORTED" 'Unsupported protocol version' "Unsupported protocol version is rejected" "Unsupported protocol version was not rejected"

# Test 3: Initialize
test_title "Test 3: MCP Initialize"
INIT="$(mcp_post \
	'{"jsonrpc":"2.0","id":2,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"dmcp-headless-test","version":"0.6.0"}}}')"
assert_contains_json "$INIT" '"result"' "Initialize succeeded" "Initialize failed"
printf '%s\n' "$INIT" | pretty_json

SESSION_ID="$(extract_session_id "$INIT")"
if [ -z "$SESSION_ID" ]; then
	fail "Failed to extract sessionId from initialize response"
fi
echo "Session ID: $SESSION_ID"

# Test 4: notifications/initialized
test_title "Test 4: notifications/initialized"
INIT_DONE="$(mcp_post '{"jsonrpc":"2.0","method":"notifications/initialized"}' "$SESSION_ID")"
pass "Initialized notification sent"
if [ -n "$INIT_DONE" ]; then
	printf '%s\n' "$INIT_DONE" | pretty_json
fi

# Test 5: tools/list
test_title "Test 5: Tools list"
TOOLS="$(mcp_post '{"jsonrpc":"2.0","id":3,"method":"tools/list"}' "$SESSION_ID")"
assert_contains_json "$TOOLS" 'get_player' "Tools list contains get_player" "Tools list failed"
assert_contains_json "$TOOLS" 'spawn_entity' "Tools list contains spawn_entity" "Tools list missing spawn_entity"
printf '%s\n' "$TOOLS" | pretty_json

# Test 6: tools/call get_player
test_title "Test 6: Get player state"
STATE="$(mcp_post '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"get_player"}}' "$SESSION_ID")"
if printf '%s\n' "$STATE" | game_state_has_player; then
	pass "Player state contains player data"
	printf '%s\n' "$STATE" | print_game_state_pretty
else
	echo "Player state response:"
	printf '%s\n' "$STATE" | print_game_state_pretty
	warn "Player state may be empty (game not started)"
fi

# Test 7: direct get_player alias
test_title "Test 7: Direct JSON-RPC get_player"
STATE_NATIVE="$(mcp_post '{"jsonrpc":"2.0","id":5,"method":"get_player"}' "$SESSION_ID")"
assert_contains_json "$STATE_NATIVE" '"result"' "Direct method get_player is available" "Direct get_player method failed"

# Test 8: direct execute_command alias
test_title "Test 8: Direct JSON-RPC execute_command"
COMMAND_NATIVE="$(mcp_post '{"jsonrpc":"2.0","id":6,"method":"execute_command","params":{"type":"pause_game","params":{"paused":false}}}' "$SESSION_ID")"
assert_contains_json "$COMMAND_NATIVE" '"queued"' "Direct method execute_command is available" "Direct execute_command method failed"

# Test 9: /game/state route
test_title "Test 9: GET /game/state"
GAME_STATE_ROUTE="$(curl -sS -m "$HTTP_TIMEOUT" "$BASE_URL/game/state" || true)"
assert_contains_json "$GAME_STATE_ROUTE" '"player"' "Game state route returns player data" "Game state route failed"

# Test 10: SSE endpoint accessibility
test_title "Test 10: SSE endpoint"
SSE_CHECK="$(curl -sS -m 2 -H "Accept: text/event-stream" "$MCP_URL" 2>/dev/null | head -1 || true)"
if [ -n "$SSE_CHECK" ]; then
	pass "SSE stream on /mcp is accessible"
else
	warn "SSE stream on /mcp had no immediate data"
fi

# Test 11: Unknown endpoint hygiene
test_title "Test 11: Unknown endpoint returns generic error"
UNKNOWN_RESPONSE="$(curl -sS -i -m "$HTTP_TIMEOUT" "$BASE_URL/does-not-exist" || true)"
if echo "$UNKNOWN_RESPONSE" | grep -q '"code":"not_found"'; then
	if echo "$UNKNOWN_RESPONSE" | grep -Eqi 'uWebSockets|cpp-httplib|sse-httplib'; then
		fail "Unknown endpoint response leaks transport implementation details"
	fi
	pass "Unknown endpoint response is generic"
else
	assert_contains_text "$UNKNOWN_RESPONSE" '"code":"not_found"' \
		"Unknown endpoint response is generic" \
		"Unknown endpoint did not return generic not_found error"
fi

# Test 12: Concurrent request handling
test_title "Test 12: Multiple concurrent requests"
REQUEST_PIDS=()
request_count=0
while [ "$request_count" -lt 5 ]; do
	curl -sS -m "$HTTP_TIMEOUT" "$BASE_URL/health" >/dev/null &
	REQUEST_PIDS+=("$!")
	request_count=$((request_count + 1))
done

for pid in "${REQUEST_PIDS[@]}"; do
	wait "$pid"
done

pass "5 concurrent requests completed"

# Final cleanup
echo ""
echo "Stopping $ENGINE_NAME..."
cleanup
DOOM_PID=""

echo ""
echo "=== All Tests Passed ==="
