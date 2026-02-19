"""E2E tests for game mode and skill validation."""

import pytest

from .conftest import RUN_EXTENDED_SCENARIOS

SKILL_FIXTURES = [
    pytest.param(
        "fresh_game_skill_1",
        "I'm Too Young To Die",
        marks=pytest.mark.skipif(
            not RUN_EXTENDED_SCENARIOS,
            reason="Set DMCP_E2E_EXTENDED_SCENARIOS=1 to run non-default skill tests",
        ),
        id="skill1",
    ),
    pytest.param("fresh_game", "Hurt Me Plenty", id="skill3"),
    pytest.param(
        "fresh_game_skill_5",
        "Nightmare!",
        marks=pytest.mark.skipif(
            not RUN_EXTENDED_SCENARIOS,
            reason="Set DMCP_E2E_EXTENDED_SCENARIOS=1 to run non-default skill tests",
        ),
        id="skill5",
    ),
]


class TestGameMode:
    """Validate game mode state."""

    def test_mode_is_single_player(self, fresh_game):
        game = fresh_game.get_state()["game"]
        assert game["mode"] == "single_player"

    def test_respawnmonsters_is_bool(self, fresh_game):
        assert isinstance(fresh_game.get_state()["game"]["respawnmonsters"], bool)

    def test_consoleplayer_is_zero(self, fresh_game):
        assert fresh_game.get_state()["game"]["consoleplayer"] == 0


class TestSkillLevels:
    """Validate exposed skill labels."""

    @pytest.mark.parametrize("fixture_name,expected_skill", SKILL_FIXTURES)
    def test_skill_levels(self, request, fixture_name, expected_skill):
        game = request.getfixturevalue(fixture_name)
        assert game.get_state()["level"]["skill"] == expected_skill
