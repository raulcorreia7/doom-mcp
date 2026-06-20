# MCP Tool Surface Audit

Updated: 2026-06-20

This document classifies DMCP tools by operational risk and relevance for remote game-agent control.

## Current Tool Groups

### Read-only state and metadata tools

- `get_player`
- `get_enemies`
- `get_entities`
- `get_items`
- `get_map`
- `get_inventory`
- `get_game_info`
- `get_state`
- `get_state_batch`
- `get_screenshot`
- `get_command_result`
- `get_available_content`
- `get_available_enemies`
- `get_available_entities`
- `get_available_items`
- `get_available_weapons`
- `get_available_ammo`
- `get_available_keys`
- `get_available_maps`
- `get_available_giveable`
- `get_command_examples`

### Mutating game-state tools

- `execute_batch`
- `spawn_entity`
- `change_level`
- `give_item`
- `set_player_health`
- `set_player_position`
- `execute_console`
- `pause_game`
- `damage_entity`
- `kill_entity`
- `player_input`

## Relevance Assessment (Remote Agent + Live Game)

- Keep: all read-only tools listed above.
- Keep: direct command tools and `execute_batch.calls` as the mutating control API.
- Keep: canonical structured arguments only; aliases and shorthand fields are rejected.
- Keep with caution: `execute_console` (highest-risk capability due broad command surface).

## Hardening Recommendations

- Use tool annotations (`readOnlyHint`, `destructiveHint`, `idempotentHint`) so clients can enforce safer defaults.
- Default agent auto-approval to read-only tools only.
- Require explicit approval for:
  - `execute_console`
  - `change_level`
  - `spawn_entity`
  - `execute_batch`
- Keep MCP lifecycle strict (`MCP-Session-Id` required after initialize).

## Follow-up (Optional)

- Add runtime allow/deny list config for tool exposure per deployment profile (safe vs full-control).
- Split privileged tools (for example `execute_console`) behind a separate capability gate.
