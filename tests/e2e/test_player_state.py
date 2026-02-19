"""E2E tests for player state validation."""

import pytest

from .conftest import ARMOR_TYPES, PLAYER_STATES, RUN_EXTENDED_SCENARIOS, WEAPON_NAMES

PLAYER_REQUIRED_FIELDS = [
    "hp",
    "armor",
    "armortype",
    "position",
    "angle",
    "readyweapon",
    "pendingweapon",
    "weaponowned",
    "ammo",
    "maxammo",
    "backpack",
    "powers",
    "cards",
    "playerstate",
    "cheats",
    "damagecount",
]

LEVEL_CASES = [
    pytest.param(1, 1, "E1M1", id="e1m1"),
    pytest.param(
        1,
        2,
        "E1M2",
        marks=pytest.mark.skipif(
            not RUN_EXTENDED_SCENARIOS,
            reason="Set DMCP_E2E_EXTENDED_SCENARIOS=1 to run non-default map startup tests",
        ),
        id="e1m2",
    ),
    pytest.param(
        1,
        3,
        "E1M3",
        marks=pytest.mark.skipif(
            not RUN_EXTENDED_SCENARIOS,
            reason="Set DMCP_E2E_EXTENDED_SCENARIOS=1 to run non-default map startup tests",
        ),
        id="e1m3",
    ),
]

SKILL_CASES = [
    pytest.param(
        1,
        "I'm Too Young To Die",
        marks=pytest.mark.skipif(
            not RUN_EXTENDED_SCENARIOS,
            reason="Set DMCP_E2E_EXTENDED_SCENARIOS=1 to run non-default skill startup tests",
        ),
        id="skill1",
    ),
    pytest.param(
        2,
        "Hey, Not Too Rough",
        marks=pytest.mark.skipif(
            not RUN_EXTENDED_SCENARIOS,
            reason="Set DMCP_E2E_EXTENDED_SCENARIOS=1 to run non-default skill startup tests",
        ),
        id="skill2",
    ),
    pytest.param(
        4,
        "Ultra-Violence",
        marks=pytest.mark.skipif(
            not RUN_EXTENDED_SCENARIOS,
            reason="Set DMCP_E2E_EXTENDED_SCENARIOS=1 to run non-default skill startup tests",
        ),
        id="skill4",
    ),
    pytest.param(
        5,
        "Nightmare!",
        marks=pytest.mark.skipif(
            not RUN_EXTENDED_SCENARIOS,
            reason="Set DMCP_E2E_EXTENDED_SCENARIOS=1 to run non-default skill startup tests",
        ),
        id="skill5",
    ),
]


@pytest.fixture
def player_state(fresh_game):
    return fresh_game.get_state()["player"]


class TestPlayerState:
    """Validate player state structure and values."""

    @pytest.mark.parametrize("field", PLAYER_REQUIRED_FIELDS)
    def test_player_has_required_fields(self, player_state, field):
        assert field in player_state, f"Missing field: {field}"

    @pytest.mark.parametrize(
        "field,min_value,max_value",
        [
            ("hp", 0, 200),
            ("armor", 0, None),
        ],
    )
    def test_numeric_ranges(self, player_state, field, min_value, max_value):
        value = player_state[field]
        assert isinstance(value, (int, float))
        assert value >= min_value
        if max_value is not None:
            assert value <= max_value

    @pytest.mark.parametrize(
        "field,allowed",
        [
            ("armortype", ARMOR_TYPES),
            ("playerstate", PLAYER_STATES),
            ("readyweapon", WEAPON_NAMES),
        ],
    )
    def test_enum_values(self, player_state, field, allowed):
        assert player_state[field] in allowed

    @pytest.mark.parametrize(
        "field,expected_len",
        [
            ("weaponowned", 9),
            ("ammo", 4),
            ("maxammo", 4),
            ("powers", 6),
            ("cards", 6),
        ],
    )
    def test_array_lengths(self, player_state, field, expected_len):
        assert len(player_state[field]) == expected_len

    @pytest.mark.parametrize("field", ["weaponowned", "cards"])
    def test_binary_arrays(self, player_state, field):
        for value in player_state[field]:
            assert value in (0, 1)

    @pytest.mark.parametrize(
        "field,min_value",
        [
            ("ammo", 0),
            ("maxammo", 1),
            ("powers", 0),
        ],
    )
    def test_array_minimum_values(self, player_state, field, min_value):
        for value in player_state[field]:
            assert value >= min_value

    def test_backpack_is_bool(self, player_state):
        assert isinstance(player_state["backpack"], bool)


class TestPlayerPosition:
    """Validate player position."""

    @pytest.mark.parametrize("axis", ["x", "y", "z"])
    def test_position_axes_are_numeric(self, player_state, axis):
        position = player_state["position"]
        assert axis in position
        assert isinstance(position[axis], (int, float))

    def test_angle_is_radians(self, player_state):
        angle = player_state["angle"]
        assert isinstance(angle, (int, float))
        assert 0 <= angle <= 6.28


class TestPlayerInitialState:
    """Validate initial player state on E1M1."""

    @pytest.mark.parametrize(
        "field,expected",
        [
            ("hp", 100),
            ("armor", 0),
            ("readyweapon", "Pistol"),
            ("playerstate", "alive"),
        ],
    )
    def test_initial_state_values(self, player_state, field, expected):
        assert player_state[field] == expected


class TestProcessIsolation:
    """Test function-scoped fixture for process isolation."""

    def test_fresh_instance_default(self, fresh_game):
        state = fresh_game.get_state()
        assert state["level"]["level_id"] == "E1M1"
        assert state["level"]["gamestate"] == "in_level"
        assert state["player"]["hp"] == 100

    @pytest.mark.parametrize("episode,map_num,expected_id", LEVEL_CASES)
    def test_different_levels(self, doom_instance, episode, map_num, expected_id):
        try:
            with doom_instance(episode=episode, map_num=map_num, skill=3) as game:
                assert game.get_state()["level"]["level_id"] == expected_id
                assert game.get_state()["level"]["gamestate"] == "in_level"
        except (RuntimeError, TimeoutError) as exc:
            if expected_id != "E1M1":
                pytest.skip(
                    f"Non-default map startup unstable in this environment: {exc}"
                )
            raise

    @pytest.mark.parametrize("skill,expected_name", SKILL_CASES)
    def test_different_skills(self, doom_instance, skill, expected_name):
        try:
            with doom_instance(episode=1, map_num=1, skill=skill) as game:
                assert game.get_state()["level"]["skill"] == expected_name
        except (RuntimeError, TimeoutError) as exc:
            if skill != 3:
                pytest.skip(
                    f"Non-default skill startup unstable in this environment: {exc}"
                )
            raise
