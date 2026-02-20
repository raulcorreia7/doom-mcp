#!/bin/bash
# Run headless Doom engine with DMCP for e2e testing
# Supports: chocolate-doom, crispy-doom

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DMCP_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
WAD_DIR="$DMCP_ROOT/assets/wads"
WAD_FILE="$WAD_DIR/doom1.wad"
BUILD_DIR=""
DOOM_BIN=""
LOG_FILE="$SCRIPT_DIR/headless.log"
DMCP_PORT="${DMCP_PORT:-6060}"
MAX_START_ATTEMPTS="${DMCP_START_ATTEMPTS:-5}"
DOOM_ENGINE="${DOOM_ENGINE:-chocolate}"
ENGINE_NAME=""
DOOM_PID=""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass() { echo -e "${GREEN}✓ $1${NC}"; }
fail() {
	echo -e "${RED}✗ $1${NC}"
	exit 1
}
warn() { echo -e "${YELLOW}! $1${NC}"; }

cleanup() {
	if [ -n "$DOOM_PID" ] && kill -0 "$DOOM_PID" 2>/dev/null; then
		kill "$DOOM_PID" 2>/dev/null || true
		wait "$DOOM_PID" 2>/dev/null || true
	fi
}

trap cleanup EXIT

case "$DOOM_ENGINE" in
chocolate)
	ENGINE_NAME="Chocolate Doom"
	BUILD_DIR="${DOOM_BUILD_DIR:-$DMCP_ROOT/chocolate-doom/build}"
	DOOM_BIN="${DOOM_BIN:-$BUILD_DIR/src/chocolate-doom}"
	;;
crispy)
	ENGINE_NAME="Crispy Doom"
	BUILD_DIR="${DOOM_BUILD_DIR:-$DMCP_ROOT/crispy-doom/build}"
	DOOM_BIN="${DOOM_BIN:-$BUILD_DIR/src/crispy-doom}"
	;;
*)
	fail "Unsupported DOOM_ENGINE '$DOOM_ENGINE' (use 'chocolate' or 'crispy')"
	;;
esac

pretty_json() {
	if command -v python3 >/dev/null 2>&1; then
		python3 -m json.tool 2>/dev/null || cat
	else
		cat
	fi
}

print_game_state_pretty() {
	if ! command -v python3 >/dev/null 2>&1; then
		cat
		return 0
	fi

	python3 -c '
import json
import sys

raw = sys.stdin.read()
try:
    data = json.loads(raw)
except Exception:
    print(raw)
    raise SystemExit(0)

content = data.get("content")
if isinstance(content, list):
    for item in content:
        if isinstance(item, dict) and item.get("type") == "text":
            text = item.get("text", "")
            try:
                nested = json.loads(text)
            except Exception:
                continue
            print(json.dumps(nested, indent=2))
            raise SystemExit(0)

print(json.dumps(data, indent=2))
'
}

game_state_has_player() {
	if ! command -v python3 >/dev/null 2>&1; then
		return 1
	fi

	python3 -c '
import json
import sys

raw = sys.stdin.read()
try:
    data = json.loads(raw)
except Exception:
    raise SystemExit(1)

content = data.get("content")
if not isinstance(content, list):
    raise SystemExit(1)

for item in content:
    if not isinstance(item, dict) or item.get("type") != "text":
        continue
    text = item.get("text", "")
    try:
        nested = json.loads(text)
    except Exception:
        continue
    if isinstance(nested, dict) and "player" in nested:
        raise SystemExit(0)

raise SystemExit(1)
'
}

echo "=== DMCP Headless Test Runner ==="
echo ""

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
	if [ "$DOOM_ENGINE" = "crispy" ]; then
		echo "  1. make submodules"
		echo "  2. ./tests/integration/build_crispy_doom.sh"
	else
		echo "  1. make submodules"
		echo "  2. make chocolate-doom"
	fi
	exit 1
fi

# Step 3: Run headless
echo "Starting $ENGINE_NAME (headless)..."
echo "  WAD: $WAD_FILE"
echo "  DMCP Port: $DMCP_PORT"
echo "  Log: output not redirected (stability)"
echo ""

for attempt in $(seq 1 "$MAX_START_ATTEMPTS"); do
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy $DOOM_BIN \
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

	ready=false
	for i in $(seq 1 30); do
		if curl -s "http://localhost:$DMCP_PORT/health" >/dev/null 2>&1; then
			ready=true
			break
		fi

		if ! kill -0 "$DOOM_PID" 2>/dev/null; then
			break
		fi

		sleep 1
	done

	if [ "$ready" = true ]; then
		pass "DMCP server is running"
		break
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

# Step 5: Run tests
echo ""
echo "Running MCP protocol tests..."
echo ""

# Test 1: Health check
echo "Test 1: Health check"
HEALTH=$(curl -s "http://localhost:$DMCP_PORT/health")
if echo "$HEALTH" | grep -q '"status":"ok"'; then
	pass "Health check succeeded"
	printf '%s\n' "$HEALTH" | pretty_json
else
	echo "Health check failed response:"
	printf '%s\n' "$HEALTH" | pretty_json
	fail "Health check failed"
fi

# Test 2: MCP Initialize
echo ""
echo "Test 2: MCP Initialize"
INIT=$(curl -s -X POST "http://localhost:$DMCP_PORT/mcp" \
	-H "Content-Type: application/json" \
	-d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18"}}')
if echo "$INIT" | grep -q '"result"'; then
	pass "Initialize succeeded"
	printf '%s\n' "$INIT" | pretty_json
else
	echo "Initialize failed response:"
	printf '%s\n' "$INIT" | pretty_json
	fail "Initialize failed"
fi

# Test 3: Tools list
echo ""
echo "Test 3: Tools list"
TOOLS=$(curl -s -X POST "http://localhost:$DMCP_PORT/mcp" \
	-H "Content-Type: application/json" \
	-d '{"jsonrpc":"2.0","id":2,"method":"tools/list"}')
if echo "$TOOLS" | grep -q 'get_player'; then
	pass "Tools list contains get_player"
	printf '%s\n' "$TOOLS" | pretty_json
else
	echo "Tools list response:"
	printf '%s\n' "$TOOLS" | pretty_json
	fail "Tools list failed"
fi

if echo "$TOOLS" | grep -q 'spawn_entity'; then
	pass "Tools list contains spawn_entity"
else
	echo "Tools list response:"
	printf '%s\n' "$TOOLS" | pretty_json
	fail "Tools list missing spawn_entity"
fi

# Test 4: Get player state
echo ""
echo "Test 4: Get player state"
STATE=$(curl -s -X POST "http://localhost:$DMCP_PORT/mcp" \
	-H "Content-Type: application/json" \
	-d '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"get_player"}}')
if printf '%s\n' "$STATE" | game_state_has_player; then
	pass "Player state contains player data"
	printf '%s\n' "$STATE" | print_game_state_pretty
else
	echo "Player state response:"
	printf '%s\n' "$STATE" | print_game_state_pretty
	warn "Player state may be empty (game not started)"
fi

# Test 5: Direct method alias (get_player)
echo ""
echo "Test 5: Direct JSON-RPC get_player"
STATE_NATIVE=$(curl -s -X POST "http://localhost:$DMCP_PORT/mcp" \
	-H "Content-Type: application/json" \
	-d '{"jsonrpc":"2.0","id":4,"method":"get_player","params":{}}')
if echo "$STATE_NATIVE" | grep -q '"result"'; then
	pass "Direct method get_player is available"
else
	echo "Direct get_player response:"
	printf '%s\n' "$STATE_NATIVE" | pretty_json
	fail "Direct get_player method failed"
fi

# Test 6: Direct method alias (execute_command)
echo ""
echo "Test 6: Direct JSON-RPC execute_command"
COMMAND_NATIVE=$(curl -s -X POST "http://localhost:$DMCP_PORT/mcp" \
	-H "Content-Type: application/json" \
	-d '{"jsonrpc":"2.0","id":5,"method":"execute_command","params":{"type":"pause_game","params":{"paused":false}}}')
if echo "$COMMAND_NATIVE" | grep -q '"queued"'; then
	pass "Direct method execute_command is available"
else
	echo "Direct execute_command response:"
	printf '%s\n' "$COMMAND_NATIVE" | pretty_json
	fail "Direct execute_command method failed"
fi

# Test 7: Game state route
echo ""
echo "Test 7: GET /game/state"
GAME_STATE_ROUTE=$(curl -s -m 2 "http://localhost:$DMCP_PORT/game/state" || true)
if echo "$GAME_STATE_ROUTE" | grep -q '"player"'; then
	pass "Game state route returns player data"
else
	echo "Game state route response:"
	printf '%s\n' "$GAME_STATE_ROUTE" | pretty_json
	fail "Game state route failed"
fi

# Test 8: SSE streaming (quick check)
echo ""
echo "Test 8: SSE endpoint"
SSE_CHECK=$(curl -s -m 2 "http://localhost:$DMCP_PORT/mcp" 2>/dev/null | head -1 || true)
if [ -n "$SSE_CHECK" ]; then
	pass "SSE stream on /mcp is accessible"
else
	warn "SSE stream on /mcp had no immediate data"
fi

# Test 9: Unknown endpoint error hygiene
echo ""
echo "Test 9: Unknown endpoint returns generic error"
UNKNOWN_RESPONSE=$(curl -s -i -m 2 "http://localhost:$DMCP_PORT/does-not-exist" || true)
if echo "$UNKNOWN_RESPONSE" | grep -q '"code":"not_found"'; then
	if echo "$UNKNOWN_RESPONSE" | grep -qi 'uWebSockets'; then
		fail "Unknown endpoint response leaks transport implementation details"
	fi
	pass "Unknown endpoint response is generic"
else
	echo "Unknown endpoint response:"
	printf '%s\n' "$UNKNOWN_RESPONSE"
	fail "Unknown endpoint did not return generic not_found error"
fi

# Test 10: Multiple requests
echo ""
echo "Test 10: Multiple concurrent requests"
REQUEST_PIDS=()
for i in {1..5}; do
	curl -s -m 2 "http://localhost:$DMCP_PORT/health" >/dev/null &
	REQUEST_PIDS+=("$!")
done

for pid in "${REQUEST_PIDS[@]}"; do
	wait "$pid"
done

pass "5 concurrent requests completed"

# Cleanup
echo ""
echo "Stopping $ENGINE_NAME..."
cleanup
DOOM_PID=""

echo ""
echo "=== All Tests Passed ==="
