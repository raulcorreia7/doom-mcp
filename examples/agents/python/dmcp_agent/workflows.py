from __future__ import annotations

import argparse
import json
from typing import Any, Dict, List, Optional

from . import contract
from .client import DMCPClient
from .commands import (
    change_level_args,
    damage_entity_args,
    execute_console_args,
    give_args,
    give_many,
    kill_entity_args,
    pause_game_args,
    player_input_args,
    set_player_health_args,
    set_player_position_args,
    spawn_args,
    tool_call,
)
from .content import compact, content_digest, require_available, require_giveable
from .errors import DMCPError

Json = Dict[str, Any]


async def content(client: DMCPClient, args: argparse.Namespace) -> Any:
    params: Json = {}
    if args.mode:
        params["game_mode"] = args.mode
    return await client.call_tool(contract.CONTENT_TOOLS[args.target], params)


async def read(client: DMCPClient, args: argparse.Namespace) -> Any:
    return await client.call_tool(contract.READ_TOOLS[args.target], read_arguments(args))


async def brief(client: DMCPClient, args: argparse.Namespace) -> Any:
    requests = [
        {"section": "game"},
        {"section": "player"},
        {"section": "map"},
        {"section": "inventory", "limit": args.limit},
        {"section": "enemies", "status": "alive", "limit": args.limit},
        {"section": "entities", "kind": "all", "status": "all", "limit": args.limit},
        {"section": "items", "kind": "item", "limit": args.limit},
    ]
    state = await client.call_tool("get_state_batch", {"requests": requests})
    available = await client.call_tool("get_available_content", {})
    return {
        "dmcp": "compact-brief",
        "state": compact(state, args.limit),
        "available_content": content_digest(available, args.limit),
        "agent_rules": [
            "Use exact canonical names from available_content.",
            "Use granular read tools for focused context: get_player, get_map, get_game_info, "
            "get_enemies, get_entities, get_items, and get_screenshot.",
            "Use direct MCP tools for single commands and execute_batch.calls for batchable commands.",
            "execute_batch calls use {name, arguments}; change_level and player_input are direct tools.",
            contract.WEAPON_SLOT_HELP,
            "Use get_command_result with sequences returned by queued mutating tools.",
        ],
    }


async def spawn(client: DMCPClient, args: argparse.Namespace) -> Any:
    available = await client.call_tool("get_available_entities", {})
    entity_class = require_available(available, "entities", args.entity_class)
    return await client.call_tool(
        "spawn_entity", spawn_args(entity_class, args.x, args.y, args.angle, args.tid)
    )


async def spawn_batch(client: DMCPClient, args: argparse.Namespace) -> Any:
    available = await client.call_tool("get_available_entities", {})
    calls: List[Json] = []
    for item in parse_spawn_specs(args.spawns_json):
        entity_class = require_available(available, "entities", str(item["entity_class"]))
        item["entity_class"] = entity_class
        calls.append(tool_call("spawn_entity", item))

    return batch_response(await client.call_tool("execute_batch", {"calls": calls}), len(calls))


async def give_item(client: DMCPClient, args: argparse.Namespace) -> Any:
    available = await client.call_tool("get_available_giveable", {})
    item_class = require_giveable(available, args.item_class)
    return await client.call_tool("give_item", give_args(item_class, args.amount))


async def batch_give(client: DMCPClient, args: argparse.Namespace) -> Any:
    available = await client.call_tool("get_available_giveable", {})
    specs = parse_give_specs(args.items_json)
    for item in specs:
        item["item_class"] = require_giveable(available, str(item["item_class"]))

    result = await client.call_tool("execute_batch", {"calls": give_many(specs)})
    return batch_response(result, len(specs))


async def level(client: DMCPClient, args: argparse.Namespace) -> Any:
    available = await client.call_tool("get_available_maps", {})
    map_name = require_available(available, "maps", args.map_name)
    return await client.call_tool(
        "change_level", change_level_args(map_name, args.skill_level, args.reset_inventory)
    )


async def set_health(client: DMCPClient, args: argparse.Namespace) -> Any:
    return await client.call_tool("set_player_health", set_player_health_args(args.health))


async def set_position(client: DMCPClient, args: argparse.Namespace) -> Any:
    return await client.call_tool(
        "set_player_position", set_player_position_args(args.x, args.y, args.angle)
    )


async def pause(client: DMCPClient, args: argparse.Namespace) -> Any:
    return await client.call_tool("pause_game", pause_game_args(parse_bool(args.paused)))


async def damage(client: DMCPClient, args: argparse.Namespace) -> Any:
    return await client.call_tool(
        "damage_entity", damage_entity_args(args.target_tid, args.damage, args.damage_type)
    )


async def kill(client: DMCPClient, args: argparse.Namespace) -> Any:
    return await client.call_tool("kill_entity", kill_entity_args(args.target_tid))


async def console(client: DMCPClient, args: argparse.Namespace) -> Any:
    return await client.call_tool("execute_console", execute_console_args(args.command_text))


async def input_tick(client: DMCPClient, args: argparse.Namespace) -> Any:
    validate_input_value(args.action, args.value)
    return await client.call_tool("player_input", player_input_args(args.action, args.value))


async def weapon(client: DMCPClient, args: argparse.Namespace) -> Any:
    return await client.call_tool("player_input", player_input_args("weapon", args.slot))


async def input_plan(client: DMCPClient, args: argparse.Namespace) -> Any:
    validate_input_value(args.action, args.value)
    results: List[Any] = []
    for _ in range(args.ticks):
        results.append(
            await client.call_tool("player_input", player_input_args(args.action, args.value))
        )
    return {"sent": len(results), "results": results}


async def batch(client: DMCPClient, args: argparse.Namespace) -> Any:
    calls = parse_batch_calls(args.calls_json)
    return batch_response(await client.call_tool("execute_batch", {"calls": calls}), len(calls))


async def command_result(client: DMCPClient, args: argparse.Namespace) -> Any:
    return await client.call_tool(
        "get_command_result", {"sequences": parse_sequence_list(args.sequences_json)}
    )


async def screenshot(client: DMCPClient, _args: argparse.Namespace) -> Any:
    return await client.call_tool("get_screenshot", {})


async def tools(client: DMCPClient, args: argparse.Namespace) -> Any:
    listed = await client.list_tools()
    if args.compact:
        return compact(listed, args.limit)
    return listed


def read_arguments(args: argparse.Namespace) -> Json:
    target = args.target
    params: Json = {}

    if target not in contract.PAGINATED_READS and (
        args.offset is not None or args.limit is not None
    ):
        raise DMCPError(f"read {target} does not accept offset or limit")
    if target in contract.PAGINATED_READS:
        if args.offset is not None:
            require_minimum_int(args.offset, 0, "offset")
            params["offset"] = args.offset
        if args.limit is not None:
            require_minimum_int(args.limit, 1, "limit")
            params["limit"] = args.limit

    if args.status is not None:
        if target not in contract.STATUS_READS:
            raise DMCPError(f"read {target} does not accept status")
        params["status"] = args.status

    if args.kind is not None:
        if target not in contract.KIND_READS:
            raise DMCPError(f"read {target} does not accept kind")
        if target == "items" and args.kind not in contract.ITEM_KIND_VALUES:
            raise DMCPError(
                "read items kind must be one of: " + ", ".join(contract.ITEM_KIND_VALUES)
            )
        params["kind"] = args.kind

    return params


def batch_response(result: Any, requested: int) -> Json:
    return {
        "status": result.get("status", "queued") if isinstance(result, dict) else "queued",
        "requested": requested,
        "result": result,
        "next": "poll get_command_result with returned sequences",
    }


def parse_spawn_specs(raw: str) -> List[Json]:
    data = parse_json_array(raw, "spawns-json")
    specs: List[Json] = []

    for idx, entry in enumerate(data):
        obj = require_object(entry, f"spawns-json[{idx}]")
        require_only_fields(obj, {"entity_class", "x", "y", "angle", "tid"}, f"spawns-json[{idx}]")
        entity_class = require_string(obj, "entity_class", f"spawns-json[{idx}]")
        spec = spawn_args(
            entity_class,
            require_number(obj, "x", f"spawns-json[{idx}]"),
            require_number(obj, "y", f"spawns-json[{idx}]"),
            optional_number(obj, "angle", 0.0, f"spawns-json[{idx}]"),
            optional_int(obj, "tid", None, f"spawns-json[{idx}]"),
        )
        specs.append(spec)

    return specs


def parse_give_specs(raw: str) -> List[Json]:
    data = parse_json_array(raw, "items-json")
    specs: List[Json] = []

    for idx, entry in enumerate(data):
        obj = require_object(entry, f"items-json[{idx}]")
        require_only_fields(obj, {"item_class", "amount"}, f"items-json[{idx}]")
        specs.append(
            give_args(
                require_string(obj, "item_class", f"items-json[{idx}]"),
                optional_int(obj, "amount", 1, f"items-json[{idx}]"),
            )
        )

    return specs


def parse_batch_calls(raw: str) -> List[Json]:
    data = parse_json_array(raw, "calls-json")
    calls: List[Json] = []

    for idx, entry in enumerate(data):
        obj = require_object(entry, f"calls-json[{idx}]")
        require_only_fields(obj, {"name", "arguments"}, f"calls-json[{idx}]")
        name = require_string(obj, "name", f"calls-json[{idx}]")
        if name == "change_level":
            raise ValueError("execute_batch rejects change_level; use the level command")
        if name == "player_input":
            raise ValueError("execute_batch does not accept player_input; use input or input-plan")
        if name not in contract.BATCHABLE_COMMAND_TOOLS:
            allowed = ", ".join(contract.BATCHABLE_COMMAND_TOOLS)
            raise ValueError(f"calls-json[{idx}] name must be one of: {allowed}")
        arguments = require_object(obj.get("arguments"), f"calls-json[{idx}].arguments")
        calls.append(tool_call(name, dict(arguments)))

    return calls


def parse_sequence_list(raw: str) -> List[int]:
    data = parse_json_array(raw, "sequences-json")
    sequences: List[int] = []
    for idx, item in enumerate(data):
        if not isinstance(item, int) or isinstance(item, bool) or item <= 0:
            raise ValueError(f"sequences-json[{idx}] must be a positive integer")
        sequences.append(item)
    return sequences


def parse_json_array(raw: str, label: str) -> List[Any]:
    data = json.loads(raw)
    if not isinstance(data, list):
        raise ValueError(f"{label} must be a JSON array")
    if not data:
        raise ValueError(f"{label} must not be empty")
    return data


def require_object(value: Any, label: str) -> Json:
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be an object")
    return value


def require_only_fields(value: Json, allowed: set[str], label: str) -> None:
    unsupported = sorted(set(value) - allowed)
    if unsupported:
        raise ValueError(f"{label} has unsupported field(s): {', '.join(unsupported)}")


def require_string(value: Json, field: str, label: str) -> str:
    item = value.get(field)
    if not isinstance(item, str) or not item:
        raise ValueError(f"{label}.{field} must be a non-empty string")
    return item


def require_number(value: Json, field: str, label: str) -> float:
    item = value.get(field)
    if not is_number(item):
        raise ValueError(f"{label}.{field} must be a number")
    return float(item)


def optional_number(value: Json, field: str, default: float, label: str) -> float:
    if field not in value:
        return default
    return require_number(value, field, label)


def optional_int(value: Json, field: str, default: Optional[int], label: str) -> Optional[int]:
    if field not in value:
        return default
    item = value.get(field)
    if not isinstance(item, int) or isinstance(item, bool):
        raise ValueError(f"{label}.{field} must be an integer")
    return item


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def parse_bool(raw: str) -> bool:
    if raw == "true":
        return True
    if raw == "false":
        return False
    raise ValueError("boolean value must be true or false")


def require_minimum_int(value: int, minimum: int, label: str) -> None:
    if value < minimum:
        raise DMCPError(f"{label} must be >= {minimum}")


def validate_input_value(action: str, value: Any) -> None:
    if action in {"aim", "weapon"} and value is None:
        raise DMCPError(f"input action {action!r} requires --value")
    if action == "weapon" and value is not None:
        slot = int(value)
        if float(slot) != float(value) or slot < 1 or slot > 7:
            raise DMCPError("weapon input value must be an integer slot from 1 to 7")
