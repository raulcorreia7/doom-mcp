#!/usr/bin/env python3
"""Manual test runner to bypass pytest output capture issues."""

import subprocess
import sys
import os
import time

# Add tests/e2e to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "tests", "e2e"))

os.environ["SDL_VIDEODRIVER"] = "dummy"
os.environ["SDL_AUDIODRIVER"] = "dummy"
os.environ["SDL_NOMOUSE"] = "1"

from conftest import DoomInstance, DoomConfig, DOOM_BIN, WAD_FILE


def run_test():
    # Check prerequisites
    if not DOOM_BIN.exists():
        print(f"Binary not found: {DOOM_BIN}")
        return 1
    if not WAD_FILE.exists():
        print(f"WAD not found: {WAD_FILE}")
        return 1

    # Create instance and run test
    config = DoomConfig()
    instance = DoomInstance(config)
    try:
        instance.start()
        print("Instance started")

        # Run test_player_has_required_fields
        state = instance.get_state()
        player = state["player"]
        required = [
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
        for field in required:
            assert field in player, f"Missing field: {field}"
        print("TEST PASSED!")
        return 0
    except Exception as e:
        print(f"TEST FAILED: {e}")
        return 1
    finally:
        instance.stop()


if __name__ == "__main__":
    sys.exit(run_test())
