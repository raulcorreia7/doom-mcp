from __future__ import annotations

import json
from contextlib import AsyncExitStack
from typing import Any, Dict, Optional

from mcp import ClientSession, types
from mcp.client.streamable_http import streamable_http_client

from .errors import DMCPError

Json = Dict[str, Any]


class DMCPClient:
    """Thin owner for the MCP session lifecycle.

    Doom-specific policy lives in the other modules. Keeping transport details
    here makes it easier to add stdio or authenticated remote transports later.
    """

    def __init__(self, url: str) -> None:
        self.url = url
        self.session: Optional[ClientSession] = None
        self._exit_stack = AsyncExitStack()

    async def __aenter__(self) -> "DMCPClient":
        read_stream, write_stream, _session_id = await self._exit_stack.enter_async_context(
            streamable_http_client(self.url)
        )
        self.session = await self._exit_stack.enter_async_context(
            ClientSession(read_stream, write_stream)
        )
        await self.session.initialize()
        return self

    async def __aexit__(self, exc_type: Any, exc: Any, tb: Any) -> None:
        await self._exit_stack.aclose()

    async def list_tools(self) -> Any:
        return model_to_json(await self._require_session().list_tools())

    async def call_tool(self, name: str, arguments: Optional[Json] = None) -> Any:
        result = await self._require_session().call_tool(name, arguments or {})
        if result.isError:
            raise DMCPError(tool_text(result) or f"tool {name!r} returned an error")
        if result.structuredContent is not None:
            return result.structuredContent
        text = tool_text(result)
        if text is None:
            return model_to_json(result)
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            return text

    def _require_session(self) -> ClientSession:
        if self.session is None:
            raise DMCPError("MCP session is not connected")
        return self.session


def model_to_json(value: Any) -> Any:
    if hasattr(value, "model_dump"):
        return value.model_dump(mode="json", by_alias=True, exclude_none=True)
    return value


def tool_text(result: types.CallToolResult) -> Optional[str]:
    for item in result.content:
        if isinstance(item, types.TextContent):
            return item.text
    return None
