"""E2E tests for non-enemy world entities state."""

ENTITY_REQUIRED_FIELDS = ["id", "hp", "max_hp", "position", "angle", "type"]

KNOWN_ENEMY_TYPES = {
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
}

KNOWN_PROJECTILE_TYPES = {
    "Rocket",
    "PlasmaBall",
    "Fireball",
    "Tracer",
    "BFGBall",
}


class TestEntitiesState:
    def test_entity_count_non_negative(self, fresh_game):
        count = fresh_game.get_state()["entity_count"]
        assert isinstance(count, int)
        assert count >= 0

    def test_entities_is_list_and_matches_count(self, fresh_game):
        state = fresh_game.get_state()
        entities = state["entities"]
        assert isinstance(entities, list)
        assert len(entities) == state["entity_count"]

    def test_entities_have_required_fields(self, fresh_game):
        for entity in fresh_game.get_state()["entities"]:
            for field in ENTITY_REQUIRED_FIELDS:
                assert field in entity, f"Missing field '{field}' in {entity}"

    def test_entities_exclude_enemies_and_projectiles(self, fresh_game):
        for entity in fresh_game.get_state()["entities"]:
            entity_type = entity.get("type", "")
            assert entity_type not in KNOWN_ENEMY_TYPES
            assert entity_type not in KNOWN_PROJECTILE_TYPES
