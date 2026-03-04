"""Shared MCP JSON-RPC helpers for e2e tests."""

from __future__ import annotations

from typing import Any

import requests

PROTOCOL_VERSION = "2025-11-25"
DEFAULT_TIMEOUT = 5.0
_SESSION_CACHE: dict[int, tuple[requests.Session, str]] = {}


def _initialize_session(
    port: int, timeout: float = DEFAULT_TIMEOUT
) -> tuple[requests.Session, str]:
    session = requests.Session()

    init_payload = {
        "jsonrpc": "2.0",
        "id": 0,
        "method": "initialize",
        "params": {
            "protocolVersion": PROTOCOL_VERSION,
            "capabilities": {},
            "clientInfo": {"name": "dmcp-e2e-rpc", "version": "0.6.0"},
        },
    }
    init_resp = session.post(
        f"http://localhost:{port}/mcp",
        json=init_payload,
        headers={
            "Content-Type": "application/json",
            "MCP-Protocol-Version": PROTOCOL_VERSION,
        },
        timeout=timeout,
    )
    init_resp.raise_for_status()
    init_data = init_resp.json()
    if "error" in init_data:
        raise RuntimeError(f"MCP initialize failed: {init_data['error']}")

    session_id = init_data.get("result", {}).get("sessionId", "")
    if not session_id:
        raise RuntimeError("MCP initialize returned no sessionId")

    session.post(
        f"http://localhost:{port}/mcp",
        json={"jsonrpc": "2.0", "method": "notifications/initialized"},
        headers={
            "Content-Type": "application/json",
            "MCP-Protocol-Version": PROTOCOL_VERSION,
            "MCP-Session-Id": session_id,
        },
        timeout=timeout,
    ).raise_for_status()

    return session, session_id


def _session_for_port(
    port: int, timeout: float = DEFAULT_TIMEOUT
) -> tuple[requests.Session, str]:
    cached = _SESSION_CACHE.get(port)
    if cached is None:
        cached = _initialize_session(port, timeout=timeout)
        _SESSION_CACHE[port] = cached
    return cached


def call_rpc(
    port: int,
    method: str,
    params: dict[str, Any],
    request_id: int = 1,
    timeout: float = DEFAULT_TIMEOUT,
) -> dict[str, Any]:
    payload = {"jsonrpc": "2.0", "id": request_id, "method": method, "params": params}

    session, session_id = _session_for_port(port, timeout=timeout)
    response = session.post(
        f"http://localhost:{port}/mcp",
        json=payload,
        headers={
            "Content-Type": "application/json",
            "MCP-Protocol-Version": PROTOCOL_VERSION,
            "MCP-Session-Id": session_id,
        },
        timeout=timeout,
    )
    response.raise_for_status()
    data = response.json()

    # Process may have restarted on the same port; refresh session once.
    if data.get("error", {}).get("code") == -32002:
        _SESSION_CACHE.pop(port, None)
        session, session_id = _session_for_port(port, timeout=timeout)
        retry = session.post(
            f"http://localhost:{port}/mcp",
            json=payload,
            headers={
                "Content-Type": "application/json",
                "MCP-Protocol-Version": PROTOCOL_VERSION,
                "MCP-Session-Id": session_id,
            },
            timeout=timeout,
        )
        retry.raise_for_status()
        data = retry.json()

    return data


def close_port_session(port: int) -> None:
    cached = _SESSION_CACHE.pop(port, None)
    if cached is None:
        return
    session, _ = cached
    session.close()


def close_all_sessions() -> None:
    ports = list(_SESSION_CACHE.keys())
    for port in ports:
        close_port_session(port)
