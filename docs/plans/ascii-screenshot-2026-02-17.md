# ASCII Screenshot Implementation

**Status**: Complete
**Created**: 2026-02-17
**Updated**: 2026-02-18

## Summary
Implement ASCII screenshot capture in DMCP: convert raw pixel frames to ASCII art and return directly in MCP tool response.

## Context

- **Why**: Enable AI agents to "see" the game screen via text-based visual representation
- **Constraints**: ASCII-only for now (PNG deferred), 160-char default width, direct MCP response
- **Architecture**: Screenshot layer sits between raw pixel capture and MCP output

## Key Files

- `src/doom/internal/screenshot.hpp` — Screenshot state management
- `src/doom/screenshot.cpp` — **NEW** ASCII conversion logic
- `src/doom/mcp_handlers.cpp` — MCP tool handler for screenshots
- `include/dmcp/doom/api.h` — Public API declarations
- `CMakeLists.txt` — Build configuration

## Tasks (Sequential Order)

```
┌─────────────────────────────────────────────────────────────┐
│ 1. Create ASCII conversion core                             │
│    └─▶ 2. Extend screenshot state                          │
│         └─▶ 3. Add public API                              │
│              └─▶ 4. Wire MCP handler                        │
│                   └─▶ 5. Update build config                │
│                        └─▶ 6. Add unit tests                │
└─────────────────────────────────────────────────────────────┘
```

- [x] **Task 1**: Create ASCII conversion core
  - **Objective**: Implement pixel-to-ASCII conversion in new `src/doom/screenshot.cpp`
  - **Files**: `src/doom/screenshot.cpp` (new)
  - **Done when**: Function converts RGBA pixels to ASCII string at given width
  - **Commit hint**: `feat(screenshot): add pixel-to-ASCII conversion logic`

- [x] **Task 2**: Extend screenshot state
  - **Objective**: Add ASCII output storage and methods to `screenshot_state`
  - **Files**: `src/doom/internal/screenshot.hpp`
  - **Depends on**: Task 1 (interface defined)
  - **Done when**: State struct includes ASCII buffer and conversion methods
  - **Commit hint**: `refactor(screenshot): extend state for ASCII output`

- [x] **Task 3**: Add public API
  - **Objective**: Declare public functions for ASCII screenshot retrieval
  - **Files**: `include/dmcp/doom/api.h`
  - **Depends on**: Task 2
  - **Done when**: API signatures added for get_ascii and to_json methods
  - **Commit hint**: `feat(api): add screenshot ASCII public API`

- [x] **Task 4**: Wire MCP handler
  - **Objective**: Connect get_screenshot tool to return ASCII in response
  - **Files**: `src/doom/mcp_handlers.cpp`
  - **Depends on**: Task 3
  - **Done when**: Tool response contains ASCII art directly
  - **Commit hint**: `feat(handlers): return ASCII in screenshot tool response`

- [x] **Task 5**: Update build config
  - **Objective**: Include new screenshot.cpp in build
  - **Files**: `CMakeLists.txt`
  - **Depends on**: Task 1
  - **Done when**: Build compiles without errors
  - **Commit hint**: `build: add screenshot.cpp to build`

- [x] **Task 6**: Add unit tests
  - **Objective**: Test ASCII conversion logic
  - **Files**: `tests/unit/` (new test file)
  - **Depends on**: Task 1
  - **Done when**: Core conversion has test coverage
  - **Commit hint**: `test(screenshot): add ASCII conversion unit tests`

## Decisions Log

- 2026-02-17: Direct MCP response chosen over store+retrieve
- 2026-02-17: 160-char width selected as default
- 2026-02-17: 32-char gradient: `$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\|()1{}[]?-_+~<>i!lI;:,"^`'.`

## Notes

- Brightness formula: `(R*0.299 + G*0.587 + B*0.114)` standard luminance
- JSON output should include: dimensions, format version, ASCII data
- Aspect ratio correction: Use 0.5 factor since terminals have ~2:1 character cell ratio
