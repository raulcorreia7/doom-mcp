# DMCP Agent Helper

Optional Python helper for Codex, OpenCode, Claude, and other agentic CLIs. It
uses `uv` and the official MCP Python SDK; it is not part of the default C/C++
test framework.

Prefer this helper over raw MCP calls for normal CLI-agent gameplay workflows.
It returns compact, structured output and hides verbose schemas so models spend
fewer tokens on plumbing.

Run from this directory:

```bash
uv run dmcp-agent --help
uv run dmcp-agent --pretty brief
uv run dmcp-agent --pretty read player
uv run dmcp-agent --pretty read enemies --status alive --limit 8
uv run dmcp-agent --pretty content
uv run dmcp-agent --pretty content enemies
uv run dmcp-agent --pretty content maps
uv run dmcp-agent spawn DoomImp --x 160 --y 96
uv run dmcp-agent spawn-many --spawns-json '[{"entity_class":"DoomImp","x":160,"y":96,"angle":0}]'
uv run dmcp-agent give-many --items-json '[{"item_class":"Shotgun","amount":1},{"item_class":"Shells","amount":20}]'
uv run dmcp-agent set-position --x 160 --y 96 --angle 90
uv run dmcp-agent level E1M2 --skill-level 3
uv run dmcp-agent weapon 3
uv run dmcp-agent input-plan forward --ticks 8
uv run dmcp-agent result --sequences-json '[1,2]'
uv run dmcp-agent screenshot
```

For one-off execution without creating a local environment:

```bash
uvx --from . dmcp-agent brief
```

The helper keeps output compact but readable:

- `brief` returns selected state sections, content availability, and agent rules.
- `read` maps to granular read tools: `get_player`, `get_map`, `get_game_info`, `get_enemies`, `get_entities`, `get_items`, and `get_inventory`.
- `content` calls `get_available_content` by default, or a granular `get_available_*` tool when given `enemies`, `entities`, `items`, `weapons`, `ammo`, `keys`, `maps`, or `giveable`.
- `screenshot` calls `get_screenshot`.
- Mutating commands use the current direct tool names: `spawn_entity`, `give_item`, `change_level`, `set_player_health`, `set_player_position`, `pause_game`, `damage_entity`, `kill_entity`, `execute_console`, and `player_input`.
- JSON batch helpers accept canonical MCP fields only. `spawn-many` expects `spawn_entity` arguments with `entity_class`, `x`, `y`, optional `angle`, and optional `tid`. `give-many` expects `give_item` arguments with `item_class` and optional `amount`.
- `batch` passes canonical `execute_batch.calls` entries shaped as `{"name":"give_item","arguments":{"item_class":"Shotgun","amount":1}}`.
- `change_level` and `player_input` are direct tools in the current contract; use `level`, `input`, `weapon`, or `input-plan` instead of `batch` for those actions.
- Weapon input uses slots: `1` Fist/Chainsaw, `2` Pistol, `3` Shotgun/SuperShotgun, `4` Chaingun, `5` RocketLauncher, `6` PlasmaRifle, `7` BFG9000.
- `result` calls `get_command_result` with a canonical `sequences` array.
- `tools --compact` lists callable tools without dumping full schemas.
