"""E2E tests for enemy state validation."""

import pytest

from .mcp_rpc import call_rpc

ENEMY_REQUIRED_FIELDS = [
    "id",
    "hp",
    "max_hp",
    "state",
    "position",
    "angle",
    "target_id",
    "type",
]


def _call_rpc(port: int, method: str, params: dict, request_id: int) -> dict:
    return call_rpc(port, method, params, request_id=request_id)


def _get_enemy_section(port: int, status: str, request_id: int) -> dict:
    data = _call_rpc(
        port,
        "get_state",
        {"section": "enemies", "status": status, "offset": 0, "limit": 4096},
        request_id=request_id,
    )
    assert "error" not in data
    result = data.get("result", {})
    assert isinstance(result, dict)
    return result


class TestEnemyState:
    """Validate enemy state structure."""

    def test_enemy_count_non_negative(self, fresh_game):
        count = fresh_game.get_state()["enemy_count"]
        assert isinstance(count, int)
        assert count >= 0

    def test_enemies_is_list(self, fresh_game):
        state = fresh_game.get_state()
        enemies = state["enemies"]
        assert isinstance(enemies, list)
        assert len(enemies) == state["enemy_count"]

    @pytest.mark.parametrize("field", ENEMY_REQUIRED_FIELDS)
    def test_enemy_required_fields(self, fresh_game, field):
        for enemy in fresh_game.get_state()["enemies"]:
            assert field in enemy, f"Missing field '{field}' in {enemy}"

    @pytest.mark.parametrize("axis", ["x", "y", "z"])
    def test_enemy_position_has_xyz(self, fresh_game, axis):
        for enemy in fresh_game.get_state()["enemies"]:
            assert axis in enemy["position"]

    def test_enemy_hp_non_negative(self, fresh_game):
        for enemy in fresh_game.get_state()["enemies"]:
            assert enemy["hp"] >= 0

    def test_enemy_types_not_unknown(self, fresh_game):
        for enemy in fresh_game.get_state()["enemies"]:
            enemy_type = enemy.get("type", "Unknown")
            assert enemy_type != "Unknown", f"Enemy has Unknown type: {enemy}"

    def test_enemies_on_e1m1(self, fresh_game):
        assert fresh_game.get_state()["enemy_count"] > 0, "E1M1 should have enemies"

    def test_enemy_ids_unique(self, fresh_game):
        enemies = fresh_game.get_state()["enemies"]
        ids = [enemy["id"] for enemy in enemies]
        assert len(ids) == len(set(ids)), "Enemy IDs should be unique"

    def test_enemy_status_filter_alive_dead_all(self, fresh_game):
        port = fresh_game.config.port

        alive = _get_enemy_section(port, "alive", request_id=801)
        dead = _get_enemy_section(port, "dead", request_id=802)
        all_enemies = _get_enemy_section(port, "all", request_id=803)

        alive_count = int(alive.get("enemy_count", 0))
        dead_count = int(dead.get("enemy_count", 0))
        all_count = int(all_enemies.get("enemy_count", 0))

        assert all_count == alive_count + dead_count

        for enemy in alive.get("enemies", []):
            assert enemy.get("state") == "alive"
            assert enemy.get("hp", 0) > 0

        for enemy in dead.get("enemies", []):
            assert enemy.get("state") == "dead"
            assert enemy.get("hp", 0) <= 0


class TestEnemyTypes:
    """Test enemy type identification on E1M1."""

    KNOWN_ENEMY_TYPES = [
        "Zombieman",
        "Shotgun Guy",
        "Archvile",
        "Revenant",
        "Mancubus",
        "Chaingunner",
        "DoomImp",
        "Demon",
        "Spectre",
        "Cacodemon",
        "Baron of Hell",
        "Hell Knight",
        "Lost Soul",
        "Spider Mastermind",
        "Arachnotron",
        "Cyberdemon",
        "Pain Elemental",
    ]

    def test_e1m1_has_basic_enemy_types(self, fresh_game):
        types_found = {enemy["type"] for enemy in fresh_game.get_state()["enemies"]}
        assert "Zombieman" in types_found or "DoomImp" in types_found

    def test_all_enemy_types_are_known(self, fresh_game):
        for enemy in fresh_game.get_state()["enemies"]:
            assert enemy["type"] in self.KNOWN_ENEMY_TYPES
