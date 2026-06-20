from __future__ import annotations

from typing import Any, Dict, List, Optional

Json = Dict[str, Any]


def tool_call(name: str, arguments: Json) -> Json:
    return {"name": name, "arguments": arguments}


def spawn_args(
    entity_class: str,
    x: float,
    y: float,
    angle: float = 0.0,
    tid: Optional[int] = None,
) -> Json:
    args: Json = {"entity_class": entity_class, "x": x, "y": y, "angle": angle}
    if tid is not None:
        args["tid"] = tid
    return args


def spawn_call(
    entity_class: str,
    x: float,
    y: float,
    angle: float = 0.0,
    tid: Optional[int] = None,
) -> Json:
    return tool_call("spawn_entity", spawn_args(entity_class, x, y, angle, tid))


def give_args(item_class: str, amount: int = 1) -> Json:
    return {"item_class": item_class, "amount": amount}


def give_call(item_class: str, amount: int = 1) -> Json:
    return tool_call("give_item", give_args(item_class, amount))


def give_many(items: List[Json]) -> List[Json]:
    return [tool_call("give_item", item) for item in items]


def change_level_args(map_name: str, skill_level: int = 3, reset_inventory: bool = False) -> Json:
    return {"map_name": map_name, "skill_level": skill_level, "reset_inventory": reset_inventory}


def set_player_health_args(health: float) -> Json:
    return {"health": health}


def set_player_position_args(x: float, y: float, angle: float = 0.0) -> Json:
    return {"x": x, "y": y, "angle": angle}


def execute_console_args(command: str) -> Json:
    return {"command": command}


def pause_game_args(paused: bool) -> Json:
    return {"paused": paused}


def damage_entity_args(target_tid: int, damage: float, damage_type: str = "Normal") -> Json:
    return {"target_tid": target_tid, "damage": damage, "damage_type": damage_type}


def kill_entity_args(target_tid: int) -> Json:
    return {"target_tid": target_tid}


def player_input_args(action: str, value: Any = None) -> Json:
    params: Json = {"action": action}
    if value is not None:
        params["value"] = value
    return params
