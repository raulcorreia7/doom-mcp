# Test Matrix

This matrix classifies test complexity and intent so contributors can add
coverage at the right level.

## Simple

- Fast API contracts and defaults
- No orchestration, no sequencing dependencies
- Examples:
  - `tests/unit/test_mcp_server.cpp`
  - `tests/unit/test_mcp_protocol.cpp`
  - `tests/unit/test_layers.cpp`
  - `tests/unit/test_fake_adapter.cpp` (`[simple]` cases)

## Medium

- Multi-step state transitions inside one subsystem
- Queue + processing flows and command/result lifecycle
- Examples:
  - `tests/unit/test_doom_context.cpp`
  - `tests/unit/test_fake_adapter.cpp` (`[medium]` cases)

## Hard

- End-to-end smoke flows with multiple components cooperating
- Protocol + state + command execution + serialization verification
- Examples:
  - `tests/unit/test_fake_adapter.cpp` (`[hard][smoke]` case)
  - `tests/e2e/*.py`

## Coverage Rules

- New behavior must ship with at least one test in its primary layer.
- User-visible behavior changes should include one medium or hard test.
- Bug fixes should include a regression assertion that fails without the fix.
