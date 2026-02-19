#!/bin/bash

# Start server
./build/dummy_server >server.log 2>&1 &
SERVER_PID=$!
echo "Server started with PID $SERVER_PID"
sleep 2

# Function to check if server is up
check_server() {
	curl -s http://localhost:6060/health >/dev/null
	return $?
}

if ! check_server; then
	echo "Server failed to start"
	cat server.log
	kill $SERVER_PID
	exit 1
fi

echo "--- 1. Checking MCP Capabilities ---"
curl -s -X POST http://localhost:6060/mcp | jq .

echo -e "\n--- 2. Consuming Snapshots (SSE) ---"
# Connect to SSE stream in background, capture output
curl -N -s http://localhost:6060/mcp >sse_stream.txt &
SSE_PID=$!

# Wait for ~3 seconds (at 35Hz, this should be ~100 snapshots)
sleep 3
kill $SSE_PID

# Count "event: state" lines
SNAPSHOT_COUNT=$(grep -c "event: state" sse_stream.txt)
echo "Captured $SNAPSHOT_COUNT snapshots"

if [ "$SNAPSHOT_COUNT" -lt 30 ]; then
	echo "FAILURE: Expected at least 30 snapshots, got $SNAPSHOT_COUNT"
	cat server.log
	kill $SERVER_PID
	exit 1
fi

echo -e "\n--- 3. Game Screenshot Route ---"
curl -s http://localhost:6060/game/screenshot | jq .

echo -e "\n--- Server Logs ---"
cat server.log
echo "..."

kill $SERVER_PID
echo -e "\nTest Completed Successfully"
