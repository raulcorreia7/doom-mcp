"""E2E tests for enemy state validation."""

import pytest

ENEMY_REQUIRED_FIELDS = ["id", "hp", "max_hp", "position", "angle", "target_id", "type"]


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


class TestEnemyTypes:
    """Test enemy type identification on E1M1."""

    KNOWN_ENEMY_TYPES = [
        "Zombieman",
        "Shotgun Guy",
        "Archvile",
        "Revenant",
        "Mancubus",
        "Chaingunner",
        "Imp",
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
        assert "Zombieman" in types_found or "Imp" in types_found

    def test_all_enemy_types_are_known(self, fresh_game):
        for enemy in fresh_game.get_state()["enemies"]:
            assert enemy["type"] in self.KNOWN_ENEMY_TYPES
