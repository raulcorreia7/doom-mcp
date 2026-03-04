# MCP Tool Surface Audit

Updated: 2026-03-04

This document classifies DMCP tools by operational risk and relevance for remote game-agent control.

## Current Tool Groups

### Read-only state and metadata tools

- `get_player`
- `get_enemies`
- `get_entities`
- `get_map` / `get_level`
- `get_inventory`
- `get_game_info` / `get_game`
- `get_state`
- `get_state_batch`
- `get_screenshot`
- `get_command_result`
- `get_available_content`
- `get_command_examples`

### Mutating game-state tools

- `execute_command`
- `execute_batch`
- Command aliases:
  - `spawn_entity`
  - `change_level`
  - `give_item`
  - `set_player_health`
  - `teleport_player`
  - `set_player_position`
  - `execute_console`
  - `pause_game`
  - `damage_entity`
  - `kill_entity`
- `player_input`

## Relevance Assessment (Remote Agent + Live Game)

- Keep: all read-only tools listed above.
- Keep: `execute_command` and `execute_batch` as the main mutating control API.
- Keep: command aliases for agent ergonomics (shorter prompts, fewer schema mistakes).
- Keep with caution: `execute_console` (highest-risk capability due broad command surface).

## Hardening Recommendations

- Use tool annotations (`readOnlyHint`, `destructiveHint`, `idempotentHint`) so clients can enforce safer defaults.
- Default agent auto-approval to read-only tools only.
- Require explicit approval for:
  - `execute_console`
  - `change_level`
  - `spawn_entity`
  - `execute_batch`
- Keep MCP lifecycle strict by default (`MCP-Session-Id` required after initialize).
- Enable compatibility fallback only when needed: `DMCP_ALLOW_IMPLICIT_SESSION=1`.

## Follow-up (Optional)

- Add runtime allow/deny list config for tool exposure per deployment profile (safe vs full-control).
- Split privileged tools (for example `execute_console`) behind a separate capability gate.
