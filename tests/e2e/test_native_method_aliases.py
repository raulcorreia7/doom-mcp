"""E2E tests for direct JSON-RPC method aliases."""

import json
import time

import requests


def call_rpc(port: int, method: str, params: dict, request_id: int = 1) -> dict:
    response = requests.post(
        f"http://localhost:{port}/mcp",
        json={"jsonrpc": "2.0", "id": request_id, "method": method, "params": params},
        headers={"Content-Type": "application/json"},
        timeout=5,
    )
    response.raise_for_status()
    return response.json()


def call_tool_json(
    port: int, name: str, arguments: dict | None = None, request_id: int = 1
) -> dict:
    payload: dict = {"name": name}
    if arguments is not None:
        payload["arguments"] = arguments

    data = call_rpc(port, "tools/call", payload, request_id=request_id)
    assert "error" not in data

    content = data.get("result", {}).get("content", [])
    assert content and content[0].get("type") == "text"
    return json.loads(content[0].get("text", "{}"))


def get_player_state(port: int, request_id: int = 1) -> dict:
    data = call_rpc(port, "get_player", {}, request_id=request_id)
    assert "error" not in data
    return data.get("result", {}).get("player", {})


def get_map_state(port: int, request_id: int = 1) -> dict:
    data = call_rpc(port, "get_map", {}, request_id=request_id)
    assert "error" not in data
    return data.get("result", {}).get("map", {})


def get_command_result(port: int, sequence: int, request_id: int = 1) -> dict:
    data = call_rpc(
        port,
        "get_command_result",
        {"sequence": sequence},
        request_id=request_id,
    )
    assert "error" not in data
    return data.get("result", {})


def wait_for(predicate, timeout: float = 2.0, interval: float = 0.05) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if predicate():
            return True
        time.sleep(interval)
    return False


def test_direct_get_player_method(fresh_game):
    data = call_rpc(fresh_game.config.port, "get_player", {}, request_id=101)

    assert "error" not in data
    result = data.get("result", {})
    assert "player" in result
    assert isinstance(result.get("player", {}).get("hp"), int)


def test_tools_list_exposes_command_tools(fresh_game):
    data = call_rpc(fresh_game.config.port, "tools/list", {}, request_id=102)

    assert "error" not in data
    tools = data.get("result", {}).get("tools", [])
    names = {tool.get("name") for tool in tools}

    assert "get_player" in names
    assert "get_map" in names
    assert "get_entities" in names
    assert "get_state_batch" in names
    assert "execute_command" in names
    assert "execute_batch" in names
    assert "get_command_result" in names
    assert "get_command_examples" in names
    assert "spawn_entity" in names
    assert "teleport_player" in names


def test_get_command_examples_separates_read_and_write_batches(fresh_game):
    port = fresh_game.config.port

    examples = call_tool_json(port, "get_command_examples", request_id=150)
    assert "execute_batch" in examples
    assert "get_state_batch" in examples

    execute_batch_params = (
        examples.get("execute_batch", {}).get("example", {}).get("params", {})
    )
    assert "commands" in execute_batch_params
    assert "ordering" not in execute_batch_params

    get_state_batch_params = (
        examples.get("get_state_batch", {}).get("example", {}).get("params", {})
    )
    assert "requests" in get_state_batch_params
    assert isinstance(get_state_batch_params.get("requests"), list)


def test_execute_batch_queues_and_completes_commands(fresh_game):
    port = fresh_game.config.port

    result = call_tool_json(
        port,
        "execute_batch",
        {
            "commands": [
                {"type": "pause_game", "pause": True},
                {"type": "set_player_position", "x": 0, "y": 64, "angle": 0},
            ]
        },
        request_id=160,
    )

    assert result.get("status") == "queued"
    assert result.get("queued") == 2
    assert result.get("rejected") == 0
    assert result.get("execution_order") == [0, 1]

    sequences = result.get("sequences", [])
    assert len(sequences) == 2

    first_done = wait_for(
        lambda: (
            get_command_result(port, int(sequences[0]), request_id=161).get("completed")
            is True
        )
    )
    second_done = wait_for(
        lambda: (
            get_command_result(port, int(sequences[1]), request_id=162).get("completed")
            is True
        )
    )
    assert first_done and second_done

    first_result = get_command_result(port, int(sequences[0]), request_id=163)
    second_result = get_command_result(port, int(sequences[1]), request_id=164)
    assert first_result.get("status") == "success"
    assert second_result.get("status") == "success"


def test_execute_batch_rejects_change_level_entries(fresh_game):
    port = fresh_game.config.port

    result = call_tool_json(
        port,
        "execute_batch",
        {
            "commands": [
                {"type": "change_level", "level": "E1M1"},
                {"type": "pause_game", "pause": True},
            ]
        },
        request_id=170,
    )

    assert result.get("status") == "queued"
    assert result.get("queued") == 1
    assert result.get("rejected") == 1

    rejected = result.get("rejected_commands", [])
    assert rejected
    assert "change_level is not supported in execute_batch" in rejected[0].get(
        "error", ""
    )

    sequences = result.get("sequences", [])
    assert len(sequences) == 1

    done = wait_for(
        lambda: (
            get_command_result(port, int(sequences[0]), request_id=171).get("completed")
            is True
        )
    )
    assert done

    command_result = get_command_result(port, int(sequences[0]), request_id=172)
    assert command_result.get("status") == "success"


def test_direct_execute_command_method_accepts_agent_payloads(fresh_game):
    port = fresh_game.config.port

    payloads = [
        {"type": "set_player_health", "value": 100},
        {"type": "pause_game", "pause": False},
        {"type": "teleport_player", "x": 0, "y": 70, "z": 0},
        {"type": "give_item", "item": "Pistol", "quantity": 1},
        {"type": "change_level", "level": "E1M1"},
        {"type": "spawn_entity", "entity": "Zombieman", "x": 10, "y": 64, "z": -5},
    ]

    for index, command in enumerate(payloads, start=1):
        data = call_rpc(port, "execute_command", command, request_id=200 + index)
        assert "error" not in data

        result = data.get("result", {})
        assert result.get("status") == "queued"
        assert isinstance(result.get("command_type"), int)


def test_set_player_health_applies_points_value(doom_instance):
    with doom_instance() as game:
        port = game.config.port

        pause_data = call_rpc(
            port,
            "execute_command",
            {"type": "pause_game", "pause": True},
            request_id=300,
        )
        assert "error" not in pause_data
        assert wait_for(
            lambda: bool(get_map_state(port, request_id=3000).get("paused"))
        )

        data = call_rpc(
            port,
            "execute_command",
            {"type": "set_player_health", "health": 50},
            request_id=301,
        )
        assert "error" not in data

        assert wait_for(
            lambda: (
                40 <= int(get_player_state(port, request_id=302).get("hp", -1)) <= 60
            )
        )

        unpause_data = call_rpc(
            port,
            "execute_command",
            {"type": "pause_game", "pause": False},
            request_id=3001,
        )
        assert "error" not in unpause_data


def test_set_player_health_accepts_integer_value(doom_instance):
    with doom_instance() as game:
        port = game.config.port

        pause_data = call_rpc(
            port,
            "execute_command",
            {"type": "pause_game", "pause": True},
            request_id=3020,
        )
        assert "error" not in pause_data
        assert wait_for(
            lambda: bool(get_map_state(port, request_id=3021).get("paused"))
        )

        data = call_rpc(
            port,
            "execute_command",
            {"type": "set_player_health", "health": 50},
            request_id=303,
        )
        assert "error" not in data

        assert wait_for(
            lambda: (
                40 <= int(get_player_state(port, request_id=304).get("hp", -1)) <= 60
            )
        )

        unpause_data = call_rpc(
            port,
            "execute_command",
            {"type": "pause_game", "pause": False},
            request_id=3022,
        )
        assert "error" not in unpause_data


def test_pause_game_alias_controls_pause_state(doom_instance):
    with doom_instance() as game:
        port = game.config.port

        pause_data = call_rpc(
            port,
            "execute_command",
            {"type": "pause_game", "pause": True},
            request_id=305,
        )
        assert "error" not in pause_data
        assert wait_for(
            lambda: bool(get_map_state(port, request_id=306).get("paused")) is True
        )

        unpause_data = call_rpc(
            port,
            "execute_command",
            {"type": "pause_game", "pause": False},
            request_id=307,
        )
        assert "error" not in unpause_data
        assert wait_for(
            lambda: bool(get_map_state(port, request_id=308).get("paused")) is False
        )


def test_get_command_result_reports_success(doom_instance):
    with doom_instance() as game:
        port = game.config.port

        queued = call_rpc(
            port,
            "execute_command",
            {"type": "set_player_health", "health": 80},
            request_id=400,
        )
        assert "error" not in queued

        sequence = int(queued.get("result", {}).get("sequence", 0))
        assert sequence > 0

        result = {}

        def _complete() -> bool:
            nonlocal result
            result = get_command_result(port, sequence, request_id=401)
            return bool(result.get("completed"))

        assert wait_for(_complete)
        assert result.get("status") == "success"
        assert result.get("success") is True
        assert result.get("sequence") == sequence


def test_get_command_result_reports_failure(doom_instance):
    with doom_instance() as game:
        port = game.config.port

        queued = call_rpc(
            port,
            "execute_command",
            {"type": "kill_entity", "target_tid": 99999},
            request_id=410,
        )
        assert "error" not in queued

        sequence = int(queued.get("result", {}).get("sequence", 0))
        assert sequence > 0

        result = {}

        def _complete() -> bool:
            nonlocal result
            result = get_command_result(port, sequence, request_id=411)
            return bool(result.get("completed"))

        assert wait_for(_complete)
        assert result.get("status") == "failed"
        assert result.get("success") is False
        assert result.get("sequence") == sequence
