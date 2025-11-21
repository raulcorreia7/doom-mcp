#!/bin/bash

# Start server
./build/dummy_server > server.log 2>&1 &
SERVER_PID=$!
echo "Server started with PID $SERVER_PID"
sleep 2

# Function to check if server is up
check_server() {
    curl -s http://localhost:6060/mcp > /dev/null
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
curl -N -s http://localhost:6060/sse > sse_stream.txt &
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

echo -e "\n--- 3. Requesting Screenshot 1 ---"
curl -s -X POST http://localhost:6060/tools/call \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"params":{"name":"capture_screenshot"}}' | jq .

sleep 1

echo -e "\n--- Fetching Screenshot 1 ---"
curl -s http://localhost:6060/screenshot/latest.png -o screenshot1.png
SIZE1=$(stat -c%s screenshot1.png)
echo "Screenshot 1 size: $SIZE1 bytes"

if [ ! -s screenshot1.png ]; then
    echo "FAILURE: Screenshot 1 empty"
    kill $SERVER_PID
    exit 1
fi

echo -e "\n--- 4. Requesting Screenshot 2 ---"
curl -s -X POST http://localhost:6060/tools/call \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":2,"params":{"name":"capture_screenshot"}}' | jq .

sleep 1

echo -e "\n--- Fetching Screenshot 2 ---"
curl -s http://localhost:6060/screenshot/latest.png -o screenshot2.png
SIZE2=$(stat -c%s screenshot2.png)
echo "Screenshot 2 size: $SIZE2 bytes"

if [ ! -s screenshot2.png ]; then
    echo "FAILURE: Screenshot 2 empty"
    kill $SERVER_PID
    exit 1
fi

# Optional: Check if they are different (dummy server generates dynamic pattern)
if cmp -s screenshot1.png screenshot2.png; then
    echo "WARNING: Screenshots are identical (expected dynamic content)"
else
    echo "SUCCESS: Screenshots differ (dynamic content verified)"
fi

echo -e "\n--- Server Logs ---"
cat server.log
echo "..."

kill $SERVER_PID
echo -e "\nTest Completed Successfully"
