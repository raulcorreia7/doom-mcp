"""E2E tests for SSE integration."""

import json
import time
from typing import Iterator

import pytest
import requests


class SSEClient:
    """Client for testing SSE connections."""

    def __init__(self, url: str = "http://localhost:6060/mcp"):
        self.url = url
        self.response = None

    def connect(self, timeout: int = 5) -> Iterator[dict]:
        """Connect to SSE stream and yield events."""
        self.response = requests.get(
            self.url,
            stream=True,
            headers={"Accept": "text/event-stream"},
            timeout=timeout,
        )
        self.response.raise_for_status()

        buffer = ""
        for chunk in self.response.iter_content(chunk_size=1024, decode_unicode=True):
            buffer += chunk.decode("utf-8") if isinstance(chunk, bytes) else chunk

            # Parse SSE events
            while "\n\n" in buffer:
                event_text, buffer = buffer.split("\n\n", 1)
                event = self._parse_event(event_text)
                if event:
                    yield event

    def _parse_event(self, text: str) -> dict | None:
        """Parse an SSE event from text."""
        event = {}
        data_lines = []
        for line in text.strip().split("\n"):
            if line.startswith("event: "):
                event["event"] = line[7:]
            elif line.startswith("data: "):
                data_lines.append(line[6:])
            elif line.startswith("data:"):
                data_lines.append(line[5:])

        if data_lines:
            event["data"] = "\n".join(data_lines)

        return event if "event" in event else None

    def close(self):
        """Close the SSE connection."""
        if self.response:
            self.response.close()


class TestSSEConnection:
    """Test SSE endpoint basic connectivity."""

    def test_sse_endpoint_available(self, fresh_game):
        """Test that SSE endpoint returns 200 OK."""
        resp = requests.get(
            f"http://localhost:{fresh_game.config.port}/mcp", stream=True, timeout=5
        )
        assert resp.status_code == 200
        assert "text/event-stream" in resp.headers.get("Content-Type", "")
        resp.close()

    def test_sse_connected_event(self, fresh_game):
        """Test that initial connected event is received."""
        client = SSEClient(f"http://localhost:{fresh_game.config.port}/mcp")
        events = []

        try:
            start = time.time()
            for event in client.connect():
                events.append(event)
                if time.time() - start > 2:
                    break
        finally:
            client.close()

        # Should have at least the connected event
        assert len(events) >= 1
        assert events[0]["event"] == "connected"
        data = json.loads(events[0]["data"])
        assert "client_id" in data


class TestSSEStateEvents:
    """Test state event streaming."""

    def test_sse_state_events_format(self, fresh_game):
        """Test that state events have correct format."""
        client = SSEClient(f"http://localhost:{fresh_game.config.port}/mcp")
        state_events = []

        try:
            start = time.time()
            for event in client.connect():
                if event["event"] == "state":
                    state_events.append(event)
                    if len(state_events) >= 2 or time.time() - start > 5:
                        break
        finally:
            client.close()

        # Should receive multiple state events
        assert len(state_events) >= 1

        # Verify state event format
        for event in state_events:
            data = json.loads(event["data"])
            assert "player" in data
            assert "level" in data
            assert "game" in data
            assert "enemies" in data

    def test_sse_state_updates(self, fresh_game):
        """Test that state events update over time."""
        client = SSEClient(f"http://localhost:{fresh_game.config.port}/mcp")
        leveltimes = []

        try:
            start = time.time()
            for event in client.connect():
                if event["event"] == "state":
                    data = json.loads(event["data"])
                    leveltimes.append(data["level"]["leveltime"])
                    if len(leveltimes) >= 3 or time.time() - start > 5:
                        break
        finally:
            client.close()

        # Should have multiple state updates
        assert len(leveltimes) >= 2
        # Level time should advance
        assert leveltimes[-1] > leveltimes[0]


class TestSSEMultipleClients:
    """Test multiple concurrent SSE clients."""

    def test_multiple_sse_clients(self, fresh_game):
        """Test that multiple clients can connect simultaneously."""
        base_url = f"http://localhost:{fresh_game.config.port}"
        clients = [SSEClient(f"{base_url}/mcp") for _ in range(3)]
        events_received = [[] for _ in range(3)]

        try:
            # Connect all clients
            for i, client in enumerate(clients):
                for event in client.connect():
                    events_received[i].append(event)
                    if len(events_received[i]) >= 2:
                        break
        finally:
            for client in clients:
                client.close()

        # All clients should have received events
        for i, events in enumerate(events_received):
            assert len(events) >= 1, f"Client {i} received no events"
            assert events[0]["event"] == "connected"


class TestSSEHealthIntegration:
    """Test SSE integration with health endpoint."""

    def test_sse_updates_client_count(self, fresh_game):
        """Test that SSE connections update client count in health."""
        port = fresh_game.config.port
        health_url = f"http://localhost:{port}/health"
        sse_url = f"http://localhost:{port}/mcp"

        # Check initial client count
        resp = requests.get(health_url)
        initial_clients = resp.json()["clients"]

        # Connect SSE client
        client = SSEClient(sse_url)
        try:
            event_stream = client.connect()
            first_event = next(event_stream)
            assert first_event["event"] == "connected"

            # Give server time to update
            time.sleep(0.5)

            # Check updated client count
            resp = requests.get(health_url)
            updated_clients = resp.json()["clients"]

            # Should have one more client
            assert updated_clients == initial_clients + 1
        finally:
            client.close()
            time.sleep(0.5)

            # Verify client count returns to normal
            resp = requests.get(health_url)
            final_clients = resp.json()["clients"]
            assert final_clients == initial_clients


class TestSSEErrorHandling:
    """Test SSE error handling."""

    def test_sse_graceful_disconnect(self, fresh_game):
        """Test that SSE client disconnect is handled gracefully."""
        client = SSEClient(f"http://localhost:{fresh_game.config.port}/mcp")

        try:
            # Connect and receive some events
            events = []
            for event in client.connect():
                events.append(event)
                if len(events) >= 2:
                    break

            assert len(events) >= 1
        finally:
            # Close should not raise
            client.close()

    def test_sse_client_reconnect(self, fresh_game):
        """Test that client can reconnect after disconnect."""
        port = fresh_game.config.port

        # First connection
        client1 = SSEClient(f"http://localhost:{port}/mcp")
        try:
            events1 = []
            for event in client1.connect():
                events1.append(event)
                if len(events1) >= 1:
                    break
        finally:
            client1.close()

        # Second connection
        client2 = SSEClient(f"http://localhost:{port}/mcp")
        try:
            events2 = []
            for event in client2.connect():
                events2.append(event)
                if len(events2) >= 1:
                    break
        finally:
            client2.close()

        # Both should have received connected events
        assert events1[0]["event"] == "connected"
        assert events2[0]["event"] == "connected"
