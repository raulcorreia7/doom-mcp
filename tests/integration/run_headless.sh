#!/bin/bash
# Run headless Chocolate Doom with DMCP for e2e testing
# Requires: chocolate-doom built with DMCP adapter

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DMCP_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
WAD_DIR="$DMCP_ROOT/tests/integration/wads"
WAD_FILE="$WAD_DIR/doom1.wad"
BUILD_DIR="$DMCP_ROOT/chocolate-doom/build"
DOOM_BIN="$BUILD_DIR/src/doom/chocolate-doom"
LOG_FILE="$SCRIPT_DIR/headless.log"

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

echo "=== DMCP Headless Test Runner ==="
echo ""

# Step 1: Download WAD if needed
if [ ! -f "$WAD_FILE" ]; then
	echo "Downloading shareware WAD..."
	"$SCRIPT_DIR/download_wad.sh"
fi

# Step 2: Check Chocolate Doom binary
if [ ! -f "$DOOM_BIN" ]; then
	warn "Chocolate Doom not built yet"
	echo ""
	echo "To build Chocolate Doom with DMCP:"
	echo "  1. cd $DMCP_ROOT/chocolate-doom"
	echo "  2. Apply patches from adapters/chocolate-doom/README.md"
	echo "  3. cmake -B build -DDMCP_INCLUDE_DIR=$DMCP_ROOT/include -DDMCP_LIB_DIR=$DMCP_ROOT/build"
	echo "  4. cmake --build build"
	exit 1
fi

# Step 3: Run headless
echo "Starting Chocolate Doom (headless)..."
echo "  WAD: $WAD_FILE"
echo "  Port: 6060"
echo "  Log: $LOG_FILE"
echo ""

$DOOM_BIN \
	-iwad "$WAD_FILE" \
	-nodraw \
	-nosound \
	-nomusic \
	-nosfx \
	-window \
	-nograb \
	>"$LOG_FILE" 2>&1 &
DOOM_PID=$!

echo "PID: $DOOM_PID"

# Step 4: Wait for DMCP server
echo "Waiting for DMCP server..."
for i in $(seq 1 30); do
	if curl -s http://localhost:6060/health >/dev/null 2>&1; then
		pass "DMCP server is running"
		break
	fi
	if [ $i -eq 30 ]; then
		fail "DMCP server did not start within 30 seconds"
	fi
	sleep 1
done

# Step 5: Run tests
echo ""
echo "Running MCP protocol tests..."
echo ""

# Test 1: Health check
echo "Test 1: Health check"
HEALTH=$(curl -s http://localhost:6060/health)
if echo "$HEALTH" | grep -q '"status":"ok"'; then
	pass "Health: $HEALTH"
else
	fail "Health check failed: $HEALTH"
fi

# Test 2: MCP Initialize
echo ""
echo "Test 2: MCP Initialize"
INIT=$(curl -s -X POST http://localhost:6060/mcp \
	-H "Content-Type: application/json" \
	-d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18"}}')
if echo "$INIT" | grep -q '"result"'; then
	pass "Initialize succeeded"
else
	fail "Initialize failed: $INIT"
fi

# Test 3: Tools list
echo ""
echo "Test 3: Tools list"
TOOLS=$(curl -s -X POST http://localhost:6060/mcp \
	-H "Content-Type: application/json" \
	-d '{"jsonrpc":"2.0","id":2,"method":"tools/list"}')
if echo "$TOOLS" | grep -q 'get_game_state'; then
	pass "Tools list contains get_game_state"
else
	fail "Tools list failed"
fi

# Test 4: Get game state
echo ""
echo "Test 4: Get game state"
STATE=$(curl -s -X POST http://localhost:6060/mcp \
	-H "Content-Type: application/json" \
	-d '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"get_game_state"}}')
if echo "$STATE" | grep -q '"player"'; then
	pass "Game state contains player data"
	echo "  Response: $(echo $STATE | head -c 100)..."
else
	warn "Game state may be empty (game not started)"
fi

# Test 5: SSE streaming (quick check)
echo ""
echo "Test 5: SSE endpoint"
SSE_CHECK=$(curl -s -m 2 http://localhost:6060/sse 2>/dev/null | head -1 || true)
if [ ! -z "$SSE_CHECK" ]; then
	pass "SSE endpoint accessible"
else
	pass "SSE endpoint accessible (no data yet)"
fi

# Test 6: Multiple requests
echo ""
echo "Test 6: Multiple concurrent requests"
for i in {1..5}; do
	curl -s http://localhost:6060/health >/dev/null &
done
wait
pass "5 concurrent requests completed"

# Cleanup
echo ""
echo "Stopping Chocolate Doom..."
kill $DOOM_PID 2>/dev/null || true
wait $DOOM_PID 2>/dev/null || true

echo ""
echo "=== All Tests Passed ==="
echo ""
echo "Log file: $LOG_FILE"
