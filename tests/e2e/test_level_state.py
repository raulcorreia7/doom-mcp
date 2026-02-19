"""E2E tests for level/game state validation."""

from .conftest import GAME_MODES, GameState, SKILL_NAMES

LEVEL_REQUIRED_FIELDS = [
    "tic",
    "leveltime",
    "level_id",
    "level_name",
    "kill_count",
    "item_count",
    "secret_count",
    "totalkills",
    "totalitems",
    "totalsecrets",
    "skill",
    "gamestate",
    "paused",
]

GAME_REQUIRED_FIELDS = ["mode", "respawnmonsters", "consoleplayer"]


class TestLevelState:
    """Validate level state structure and values."""

    def test_level_has_required_fields(self, fresh_game):
        level = fresh_game.get_state()["level"]
        for field in LEVEL_REQUIRED_FIELDS:
            assert field in level, f"Missing: {field}"

    def test_level_id_format(self, fresh_game):
        level_id = fresh_game.get_state()["level"]["level_id"]
        assert level_id.startswith("E") or level_id.startswith("MAP"), (
            f"Invalid: {level_id}"
        )

    def test_leveltime_valid(self, fresh_game):
        assert fresh_game.get_state()["level"]["leveltime"] >= 0

    def test_skill_valid(self, fresh_game):
        assert fresh_game.get_state()["level"]["skill"] in SKILL_NAMES

    def test_gamestate_valid(self, fresh_game):
        gamestate = fresh_game.get_state()["level"]["gamestate"]
        assert GameState.from_string(gamestate) is not None

    def test_paused_state(self, fresh_game):
        paused = fresh_game.get_state()["level"]["paused"]
        assert isinstance(paused, bool)
        assert paused is False


class TestGameMode:
    """Validate game mode state."""

    def test_game_has_required_fields(self, fresh_game):
        game = fresh_game.get_state()["game"]
        for field in GAME_REQUIRED_FIELDS:
            assert field in game, f"Missing: {field}"

    def test_mode_valid(self, fresh_game):
        assert fresh_game.get_state()["game"]["mode"] in GAME_MODES

    def test_single_player_defaults(self, fresh_game):
        game = fresh_game.get_state()["game"]
        assert game["mode"] == "single_player"
        assert game["consoleplayer"] == 0


class TestDoomInstanceHelpers:
    """Test DoomInstance helper methods."""

    def test_is_healthy(self, fresh_game):
        assert fresh_game.is_healthy() is True

    def test_get_state_returns_dict(self, fresh_game):
        state = fresh_game.get_state()
        assert isinstance(state, dict)
        assert "player" in state
        assert "level" in state
        assert "game" in state
        assert "enemies" in state
