from __future__ import annotations

READ_TOOLS = {
    "player": "get_player",
    "map": "get_map",
    "game-info": "get_game_info",
    "enemies": "get_enemies",
    "entities": "get_entities",
    "items": "get_items",
    "inventory": "get_inventory",
}

CONTENT_TOOLS = {
    "all": "get_available_content",
    "enemies": "get_available_enemies",
    "entities": "get_available_entities",
    "items": "get_available_items",
    "weapons": "get_available_weapons",
    "ammo": "get_available_ammo",
    "keys": "get_available_keys",
    "maps": "get_available_maps",
    "giveable": "get_available_giveable",
}

CONTENT_MODES = ("shareware", "registered", "commercial", "retail")

STATUS_VALUES = ("alive", "dead", "all")
ENTITY_KIND_VALUES = (
    "all",
    "enemy",
    "item",
    "weapon",
    "ammo",
    "key",
    "health",
    "armor",
    "powerup",
    "world",
)
ITEM_KIND_VALUES = ("item", "weapon", "ammo", "key", "health", "armor", "powerup")

PAGINATED_READS = frozenset({"enemies", "entities", "items", "inventory"})
STATUS_READS = frozenset({"enemies", "entities"})
KIND_READS = frozenset({"entities", "items"})

INPUT_ACTIONS = (
    "forward",
    "backward",
    "strafe_left",
    "strafe_right",
    "turn_left",
    "turn_right",
    "aim",
    "attack",
    "use",
    "weapon",
)

WEAPON_SLOT_HELP = (
    "For action=weapon: 1=Fist/Chainsaw, 2=Pistol, 3=Shotgun/SuperShotgun, "
    "4=Chaingun, 5=RocketLauncher, 6=PlasmaRifle, 7=BFG9000"
)

COMMAND_TOOLS = (
    "spawn_entity",
    "change_level",
    "give_item",
    "set_player_health",
    "set_player_position",
    "execute_console",
    "pause_game",
    "damage_entity",
    "kill_entity",
)

BATCHABLE_COMMAND_TOOLS = tuple(tool for tool in COMMAND_TOOLS if tool != "change_level")
