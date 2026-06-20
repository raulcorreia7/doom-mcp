from __future__ import annotations

from typing import Any, Dict, Iterable

from .errors import DMCPError

Json = Dict[str, Any]


def compact(value: Any, max_items: int = 12) -> Any:
    """Drop schema/description noise and trim arrays for prompt-sized output."""
    if isinstance(value, dict):
        keep = {}
        for key, item in value.items():
            if key in {"schema", "inputSchema", "description"}:
                continue
            keep[key] = compact(item, max_items)
        return keep
    if isinstance(value, list):
        return [compact(item, max_items) for item in value[:max_items]]
    return value


def resolve_available(content: Json, kinds: Iterable[str], name: str) -> str:
    scanned: list[str] = []

    for kind in kinds:
        entries = content.get(kind)
        if not isinstance(entries, list):
            continue

        for entry in entries:
            entry_text = str(entry)
            scanned.append(entry_text)
            if entry_text == name:
                return entry_text

    preview = ", ".join(scanned[:20])
    raise DMCPError(f"{name!r} is not available as a canonical content name; available: {preview}")


def require_available(content: Json, kind: str, name: str) -> str:
    return resolve_available(content, [kind], name)


def require_giveable(content: Json, name: str) -> str:
    return resolve_available(content, ["giveable"], name)


def content_digest(content: Json, limit: int) -> Json:
    return {
        "game_mode": content.get("game_mode"),
        "game_mode_source": content.get("game_mode_source"),
        "current_level": content.get("current_level"),
        "enemies": content.get("enemies", [])[:limit],
        "entities": content.get("entities", [])[:limit],
        "weapons": content.get("weapons", [])[:limit],
        "ammo": content.get("ammo", [])[:limit],
        "keys": content.get("keys", [])[:limit],
        "items": content.get("items", [])[:limit],
        "giveable": content.get("giveable", [])[:limit],
        "maps": content.get("maps", [])[:limit],
    }
