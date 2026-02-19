# E2E Full State Validation

**Status**: Complete
**Created**: 2026-02-17
**Updated**: 2026-02-18

## Summary
Refactor adapter for maintainability, extend DMCP to expose full game state with human-friendly output, implement e2e tests.

## Context
- **Why**: Current adapter is a monolithic 260-line file with 60-line switch statements. AI agents need full game state visibility. All output should be human-readable strings, not numeric codes.
- **Constraints**: Backward compatibility, Chocolate Doom GPL, headless testing
- **Architecture**: See `docs/ARCHITECTURE.md`
- **Related**: N/A

## Key Files
- `include/dmcp/doom/types.h` — data model definitions
- `adapters/chocolate-doom/` — adapter implementation
- `chocolate-doom/src/doom/` — patch points

## Tasks

### Phase 0: Adapter Refactor (Prerequisite)

- [x] Task 0.1: Create adapter module structure
  - Objective: Split adapter.c into modular components
  - Files: 
    - `adapters/chocolate-doom/adapter.c` (lifecycle only)
    - `adapters/chocolate-doom/state_player.c`
    - `adapters/chocolate-doom/state_level.c`
    - `adapters/chocolate-doom/state_enemies.c`
    - `adapters/chocolate-doom/commands.c`
  - Done when: Single-responsibility modules, each <150 lines
  - Commit hint: refactor(adapter): split into modular components

- [x] Task 0.2: Convert enemy type lookup to data-driven table
  - Objective: Replace 60-line switch with static lookup table
  - Files: `adapters/chocolate-doom/enemy_types.h`
  - Done when: Enemy names from table, O(n) lookup with early exit
  - Commit hint: refactor(adapter): use lookup table for enemy types

- [x] Task 0.3: Add mapping utilities for Chocolate Doom types
  - Objective: Centralize fixed_t→float, enum→string conversions
  - Files: `adapters/chocolate-doom/internal/mapping.h`
  - Done when: All conversions in one place, reusable
  - Commit hint: refactor(adapter): add type mapping utilities

- [x] Task 0.4: Update CMakeLists for modular adapter
  - Objective: Build all adapter modules as single library
  - Files: `adapters/chocolate-doom/CMakeLists.txt`
  - Done when: `make` builds refactored adapter correctly
  - Commit hint: build(adapter): add modular sources to cmake

### Phase 1: Data Model (Parallel)

- [x] Task 1.1: Extend Player Data Model
  - Objective: Add weapons, ammo[4], powerups[6], keys[6], armortype, playerstate, cheats, damagecount, angle
  - Files: `include/dmcp/doom/types.h`, `include/mcp/generic/constants.h`
  - Done when: dmcp_player_t matches Chocolate Doom player_t with string fields

- [x] Task 1.2: Extend Level/Game Data Model
  - Objective: Add skill (string), totalkills/items/secrets, leveltime, gamestate (string); add dmcp_game_t
  - Files: `include/dmcp/doom/types.h`
  - Done when: Difficulty and game mode as human-readable strings

- [x] Task 1.3: Extend Entity Data Model
  - Objective: Convert dmcp_vec2_t to dmcp_vec3_t, add angle, target_id
  - Files: `include/dmcp/doom/types.h`
  - Done when: 3D positions and facing angles

- [x] Task 1.4: Patch Chocolate Doom for DMCP
  - Objective: Add DMCP calls at D_DoomMain, G_Ticker, I_Quit
  - Files: `chocolate-doom/src/doom/d_main.c`, `g_game.c`, `i_system.c`
  - Done when: chocolate-doom builds with DMCP, server on 6060

### Phase 2: Adapter Population (Parallel after Phase 1)

- [x] Task 2.1: Populate Extended Player State
  - Objective: Extract all ammo, weapons, powerups, keys, armor, angle from player_t; convert enums to strings
  - Files: `adapters/chocolate-doom/state_player.c`
  - Done when: All player fields populated with human-friendly strings
  - Depends on: [0.1-0.4], [1.1]

- [x] Task 2.2: Populate Level/Game State
  - Objective: Extract skill, totals, leveltime, gamestate, netgame, deathmatch; use human strings
  - Files: `adapters/chocolate-doom/state_level.c`
  - Done when: Difficulty and game mode as readable strings
  - Depends on: [0.1-0.4], [1.2]

- [x] Task 2.3: Populate Entity State
  - Objective: Extract z-position, angle, target from mobj_t
  - Files: `adapters/chocolate-doom/state_enemies.c`
  - Done when: 3D positions and angles populated
  - Depends on: [0.1-0.4], [1.3]

- [x] Task 2.4: Update Serialization
  - Objective: Add JSON serialization for all new types (strings, not codes)
  - Files: `src/doom/serialization.cpp`
  - Done when: dmcp_snapshot_to_json includes all new fields with human output
  - Depends on: [1.1], [1.2], [1.3]

### Phase 3: Build Integration

- [x] Task 3.1: Build Chocolate Doom with DMCP
  - Objective: CMake config to link chocolate-doom with DMCP
  - Files: `chocolate-doom/CMakeLists.txt`, `Makefile`
  - Done when: `make headless` produces working binary
  - Depends on: [1.4]

### Phase 4: E2E Testing (Parallel after Phase 3)

- [x] Task 4.1: Create E2E Test Harness
  - Objective: Python test runner for headless Doom lifecycle
  - Files: `tests/e2e/harness.py`, `tests/e2e/conftest.py`
  - Done when: Reliable start/stop/query
  - Depends on: [3.1]

- [x] Task 4.2: E2E Tests - Player State
  - Objective: Validate weapons, ammo, powerups, keys, armor with string assertions
  - Files: `tests/e2e/test_player_state.py`
  - Done when: Tests pass for pickup, damage, death
  - Depends on: [4.1], [2.1], [2.4]

- [x] Task 4.3: E2E Tests - Level & Difficulty
  - Objective: Validate difficulty strings, totals, leveltime, gamestate strings
  - Files: `tests/e2e/test_level_state.py`
  - Done when: Tests pass for all 5 skill levels, E1M1-E1M3
  - Depends on: [4.1], [2.2], [2.4]

- [x] Task 4.4: E2E Tests - Enemy Tracking
  - Objective: Validate enemy count, types, 3D positions, angles, targets
  - Files: `tests/e2e/test_enemy_state.py`
  - Done when: Tests confirm correct enemy data
  - Depends on: [4.1], [2.3], [2.4]

- [x] Task 4.5: E2E Tests - Game Mode
  - Objective: Validate netgame/deathmatch, multiplayer states
  - Files: `tests/e2e/test_game_mode.py`
  - Done when: Tests confirm game mode reporting
  - Depends on: [4.1], [2.2], [2.4]

## Parallelization

| Phase | Tasks | Parallel |
|-------|-------|----------|
| 0 | 0.1→0.4 | Sequential (refactor first) |
| 1 | 1.1, 1.2, 1.3, 1.4 | All parallel |
| 2 | 2.1, 2.2, 2.3, 2.4 | All parallel |
| 3 | 3.1 | Sequential |
| 4 | 4.2, 4.3, 4.4, 4.5 | All parallel (after 4.1) |

## Adapter Structure (After Refactor)

```
adapters/chocolate-doom/
├── CMakeLists.txt
├── README.md
├── adapter.h              # Public API
├── adapter.c              # Lifecycle: create/destroy/tick (~80 lines)
├── state_player.c         # Player extraction (~100 lines)
├── state_level.c          # Level/game extraction (~80 lines)
├── state_enemies.c        # Enemy enumeration (~100 lines)
├── commands.c             # Command execution (~80 lines)
├── enemy_types.h          # Lookup table
└── internal/
    └── mapping.h          # Fixed→float, enum→string conversions
```

## Human-Friendly String Conventions

All enum values output as readable strings, never numeric codes.

### Difficulty Levels (skill_t)
| Enum | Output String |
|------|---------------|
| sk_baby | "I'm Too Young To Die" |
| sk_easy | "Hey, Not Too Rough" |
| sk_medium | "Hurt Me Plenty" |
| sk_hard | "Ultra-Violence" |
| sk_nightmare | "Nightmare!" |

### Player State (playerstate_t)
| Enum | Output String |
|------|---------------|
| PST_LIVE | "alive" |
| PST_DEAD | "dead" |
| PST_REBORN | "reborn" |

### Game State (gamestate_t)
| Enum | Output String |
|------|---------------|
| GS_LEVEL | "in_level" |
| GS_INTERMISSION | "intermission" |
| GS_FINALE | "finale" |
| GS_DEMOSCREEN | "demo" |

### Weapons (weapontype_t)
| Enum | Output String |
|------|---------------|
| wp_fist | "Fist" |
| wp_pistol | "Pistol" |
| wp_shotgun | "Shotgun" |
| wp_chaingun | "Chaingun" |
| wp_missile | "Rocket Launcher" |
| wp_plasma | "Plasma Rifle" |
| wp_bfg | "BFG9000" |
| wp_chainsaw | "Chainsaw" |
| wp_supershotgun | "Super Shotgun" |

### Ammo Types (ammotype_t)
| Enum | Output String |
|------|---------------|
| am_clip | "Bullets" |
| am_shell | "Shells" |
| am_cell | "Cells" |
| am_misl | "Rockets" |

### Powerups (powertype_t)
| Enum | Output String |
|------|---------------|
| pw_invulnerability | "Invulnerability" |
| pw_strength | "Berserk" |
| pw_invisibility | "Partial Invisibility" |
| pw_ironfeet | "Radiation Suit" |
| pw_allmap | "Computer Map" |
| pw_infrared | "Light Amplification" |

### Keys (card_t)
| Enum | Output String |
|------|---------------|
| it_bluecard | "Blue Keycard" |
| it_yellowcard | "Yellow Keycard" |
| it_redcard | "Red Keycard" |
| it_blueskull | "Blue Skull Key" |
| it_yellowskull | "Yellow Skull Key" |
| it_redskull | "Red Skull Key" |

### Armor Types
| Value | Output String |
|-------|---------------|
| 0 | "None" |
| 1 | "Green Armor" |
| 2 | "Blue Armor" |

### Game Mode
| Combination | Output String |
|-------------|---------------|
| netgame=false | "single_player" |
| netgame=true, deathmatch=0 | "cooperative" |
| netgame=true, deathmatch=1 | "deathmatch" |
| netgame=true, deathmatch=2 | "altdeath" |

## Enemy Type Lookup Table

```c
// enemy_types.h
typedef struct {
    int mobj_type;
    const char* name;
} dmcp_enemy_type_entry_t;

static const dmcp_enemy_type_entry_t enemy_type_table[] = {
    {MT_POSSESSED,    "Zombieman"},
    {MT_SHOTGUY,      "Shotgun Guy"},
    {MT_VILE,         "Archvile"},
    {MT_UNDEAD,       "Revenant"},
    {MT_FATSO,        "Mancubus"},
    {MT_CHAINGUY,     "Chaingunner"},
    {MT_TROOP,        "Imp"},
    {MT_SERGEANT,     "Demon"},
    {MT_SHADOWS,      "Spectre"},
    {MT_HEAD,         "Cacodemon"},
    {MT_BRUISER,      "Baron of Hell"},
    {MT_KNIGHT,       "Hell Knight"},
    {MT_SKULL,        "Lost Soul"},
    {MT_SPIDER,       "Spider Mastermind"},
    {MT_BABY,         "Arachnotron"},
    {MT_CYBORG,       "Cyberdemon"},
    {MT_PAIN,         "Pain Elemental"},
};

static inline const char* dmcp_enemy_type_name(int mobj_type) {
    for (size_t i = 0; i < ARRAY_SIZE(enemy_type_table); i++) {
        if (enemy_type_table[i].mobj_type == mobj_type) {
            return enemy_type_table[i].name;
        }
    }
    return "Unknown";
}
```

## Mapping Utilities

```c
// internal/mapping.h
#include "m_fixed.h"

// Fixed-point to float (16.16 format)
static inline float dmcp_fixed_to_float(fixed_t f) {
    return (float)f / 65536.0f;
}

// Angle to radians (0-65535 → 0-2π)
static inline float dmcp_angle_to_radians(angle_t a) {
    return (float)a * (2.0f * 3.14159265f) / 65536.0f;
}

// Skill enum to human string
static inline const char* dmcp_skill_to_string(skill_t skill) {
    switch (skill) {
        case sk_baby:      return "I'm Too Young To Die";
        case sk_easy:      return "Hey, Not Too Rough";
        case sk_medium:    return "Hurt Me Plenty";
        case sk_hard:      return "Ultra-Violence";
        case sk_nightmare: return "Nightmare!";
        default:           return "Unknown";
    }
}

// Player state to human string
static inline const char* dmcp_playerstate_to_string(playerstate_t ps) {
    switch (ps) {
        case PST_LIVE:  return "alive";
        case PST_DEAD:  return "dead";
        case PST_REBORN: return "reborn";
        default:        return "unknown";
    }
}

// Game state to human string
static inline const char* dmcp_gamestate_to_string(gamestate_t gs) {
    switch (gs) {
        case GS_LEVEL:       return "in_level";
        case GS_INTERMISSION: return "intermission";
        case GS_FINALE:      return "finale";
        case GS_DEMOSCREEN:  return "demo";
        default:             return "unknown";
    }
}
```

## Data Model Extensions

### dmcp_player_t
```c
#define DMCP_MAX_WEAPONS      9
#define DMCP_MAX_AMMO_TYPES   4
#define DMCP_MAX_POWERUPS     6
#define DMCP_MAX_KEYS         6
#define DMCP_MAX_STRING       64

typedef struct {
  float hp;
  float armor;
  char armortype[DMCP_MAX_STRING];      // "None", "Green Armor", "Blue Armor"
  dmcp_vec3_t position;
  float angle;                          // radians
  
  char readyweapon[DMCP_MAX_STRING];    // current weapon name
  char pendingweapon[DMCP_MAX_STRING];  // weapon being switched to
  int32_t weaponowned[DMCP_MAX_WEAPONS];
  
  int32_t ammo[DMCP_MAX_AMMO_TYPES];
  int32_t maxammo[DMCP_MAX_AMMO_TYPES];
  int32_t backpack;
  
  int32_t powers[DMCP_MAX_POWERUPS];    // tic timers
  int32_t cards[DMCP_MAX_KEYS];
  
  char playerstate[DMCP_MAX_STRING];    // "alive", "dead", "reborn"
  int32_t cheats;                       // god/noclip flags (keep as bits)
  int32_t damagecount;                  // pain flash timer
} dmcp_player_t;
```

### dmcp_level_t
```c
typedef struct {
  int32_t tic;
  int32_t leveltime;
  char level_id[32];
  char level_name[96];
  
  int32_t kill_count;
  int32_t item_count;
  int32_t secret_count;
  int32_t totalkills;
  int32_t totalitems;
  int32_t totalsecrets;
  
  char skill[DMCP_MAX_STRING];          // "Hurt Me Plenty", etc.
  char gamestate[DMCP_MAX_STRING];      // "in_level", "intermission", etc.
} dmcp_level_t;
```

### dmcp_game_t
```c
typedef struct {
  char mode[DMCP_MAX_STRING];           // "single_player", "cooperative", "deathmatch", "altdeath"
  int32_t respawnmonsters;              // nightmare mode flag
  int32_t consoleplayer;                // which player we control
} dmcp_game_t;
```

### dmcp_enemy_t
```c
typedef struct {
  int32_t id;
  float hp;
  float max_hp;
  dmcp_vec3_t position;
  float angle;
  int32_t target_id;                    // -1 if none
  char type[128];
} dmcp_enemy_t;
```

### dmcp_snapshot_t (extended)
```c
typedef struct dmcp_snapshot_t {
  dmcp_player_t player;
  dmcp_level_t  level;
  dmcp_game_t   game;
  
  dmcp_enemy_t enemies[256];
  uint32_t     enemy_count;
  
  dmcp_item_t inventory[64];
  uint32_t    inventory_count;
} dmcp_snapshot_t;
```

## Example JSON Output

```json
{
  "player": {
    "hp": 100.0,
    "armor": 50.0,
    "armortype": "Green Armor",
    "position": {"x": 1234.5, "y": 678.9, "z": 0.0},
    "angle": 1.57,
    "readyweapon": "Shotgun",
    "pendingweapon": "Chaingun",
    "weaponowned": [1, 1, 1, 1, 0, 0, 0, 0, 0],
    "ammo": [50, 20, 0, 5],
    "maxammo": [400, 100, 100, 50],
    "backpack": false,
    "powers": [0, 0, 0, 0, 0, 0],
    "cards": [false, false, false, false, false, false],
    "playerstate": "alive",
    "cheats": 0,
    "damagecount": 0
  },
  "level": {
    "tic": 12345,
    "leveltime": 12345,
    "level_id": "E1M1",
    "level_name": "Hangar",
    "kill_count": 5,
    "item_count": 10,
    "secret_count": 1,
    "totalkills": 14,
    "totalitems": 22,
    "totalsecrets": 3,
    "skill": "Hurt Me Plenty",
    "gamestate": "in_level"
  },
  "game": {
    "mode": "single_player",
    "respawnmonsters": false,
    "consoleplayer": 0
  },
  "enemies": [
    {"id": 0, "hp": 60.0, "max_hp": 60.0, "position": {"x": 500.0, "y": 300.0, "z": 0.0}, "angle": 3.14, "target_id": -1, "type": "Zombieman"}
  ],
  "enemy_count": 1,
  "inventory": [],
  "inventory_count": 0
}
```

## Decisions Log
- 2026-02-17: Refactor adapter to modular structure before extending (maintainability)
- 2026-02-17: Data-driven enemy type lookup instead of switch
- 2026-02-17: Real-time simulation over demo playback for e2e
- 2026-02-17: Full state exposure over minimal extension
- 2026-02-17: All enum outputs as human-readable strings, not numeric codes
- 2026-02-17: Long-form difficulty names ("Hurt Me Plenty", "Ultra-Violence")
- 2026-02-17: Player state "alive" not "live"

## Notes
- Phase 0 is sequential: refactor first, then extend
- Each state_*.c file owns one domain, <150 lines target
- mapping.h isolates Chocolate Doom type knowledge
- 35 tics = 1 second for powerup timers
- All string fields use DMCP_MAX_STRING (64) for consistency
