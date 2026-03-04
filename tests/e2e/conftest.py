"""E2E test fixtures for DOOM MCP.

Requires:
- crispy-doom built with DMCP adapter
- doom1.wad in assets/wads/
- pytest, requests packages
"""

import json
import os
import signal
import socket
import subprocess
import time
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Any, Optional

import pytest
import requests

# Constants
PROJECT_ROOT = Path(__file__).resolve().parents[2]
WAD_FILE = PROJECT_ROOT / "assets/wads/doom1.wad"
DOOM_ENGINE = os.environ.get("DOOM_ENGINE", "crispy").strip().lower()
DEFAULT_DOOM_BIN = {
    "crispy": PROJECT_ROOT / "crispy-doom/build/src/crispy-doom",
}.get(DOOM_ENGINE, PROJECT_ROOT / "crispy-doom/build/src/crispy-doom")
DOOM_BIN = Path(os.environ.get("DOOM_BIN", str(DEFAULT_DOOM_BIN)))

# Timeouts (configurable via env vars)
FAST_MODE = os.environ.get("DMCP_E2E_FAST", "1") == "1"
STARTUP_TIMEOUT = float(
    os.environ.get("DMCP_STARTUP_TIMEOUT", "20" if FAST_MODE else "30")
)
STARTUP_INITIAL_DELAY = float(
    os.environ.get("DMCP_STARTUP_INITIAL_DELAY", "0.25" if FAST_MODE else "0.75")
)
REQUEST_TIMEOUT = float(
    os.environ.get("DMCP_REQUEST_TIMEOUT", "5" if FAST_MODE else "10")
)
SESSION_DEFAULT_PORT = int(os.environ.get("DMCP_SESSION_PORT", "0"))
START_RETRIES = int(os.environ.get("DMCP_START_RETRIES", "3" if FAST_MODE else "5"))
RUN_EXTENDED_SCENARIOS = os.environ.get("DMCP_E2E_EXTENDED_SCENARIOS", "0") == "1"
ALLOW_TOOL_FALLBACK = os.environ.get("DMCP_E2E_ALLOW_TOOL_FALLBACK", "0") == "1"


class GameState(Enum):
    """Game state enumeration matching DMCP server output."""

    MENU = "demo"
    LOADING = "loading"
    IN_LEVEL = "in_level"
    INTERMISSION = "intermission"
    FINALE = "finale"

    @classmethod
    def from_string(cls, value: str) -> Optional["GameState"]:
        for state in cls:
            if state.value == value:
                return state
        return None


# Game data constants
WEAPON_NAMES = [
    "Fist",
    "Pistol",
    "Shotgun",
    "Chaingun",
    "Rocket Launcher",
    "Plasma Rifle",
    "BFG9000",
    "Chainsaw",
    "Super Shotgun",
]
ARMOR_TYPES = ["None", "Green Armor", "Blue Armor"]
PLAYER_STATES = ["alive", "dead", "reborn"]
AMMO_NAMES = ["bullets", "shells", "cells", "rockets"]
POWERUP_NAMES = [
    "invulnerability",
    "berserk",
    "invisibility",
    "radiation suit",
    "computer map",
    "light amplification",
]
KEY_NAMES = [
    "blue keycard",
    "yellow keycard",
    "red keycard",
    "blue skull",
    "yellow skull",
    "red skull",
]
SKILL_NAMES = [
    "I'm Too Young To Die",
    "Hey, Not Too Rough",
    "Hurt Me Plenty",
    "Ultra-Violence",
    "Nightmare!",
]
GAME_MODES = ["single_player", "cooperative", "deathmatch"]


def require_extended_scenarios(reason: str) -> None:
    if not RUN_EXTENDED_SCENARIOS:
        pytest.skip(f"{reason}. Set DMCP_E2E_EXTENDED_SCENARIOS=1 to enable this test.")


def _port_available(port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(0.5)
        return sock.connect_ex(("localhost", port)) != 0


def _find_free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("localhost", 0))
        return int(sock.getsockname()[1])


@dataclass
class DoomConfig:
    """Configuration for a Doom instance."""

    episode: int = 1
    map_num: int = 1
    skill: int = 3
    port: int = 6060
    timeout: float = STARTUP_TIMEOUT

    # Advanced options
    nodraw: bool = True
    nosound: bool = True
    extra_args: list[str] = field(default_factory=list)


class DoomInstance:
    """Manages a single Doom instance with full lifecycle control."""

    def __init__(self, config: DoomConfig):
        self.config = config
        self._proc: Optional[subprocess.Popen] = None
        self._mcp_url = f"http://localhost:{self.config.port}/mcp"
        self._state_url = f"http://localhost:{self.config.port}/game/state"
        self._health_url = f"http://localhost:{self.config.port}/health"
        self._http = requests.Session()
        self._started = False
        self._session_id: Optional[str] = None

    def stop(self) -> None:
        """Stop the Doom instance gracefully."""
        if self._proc is None:
            return

        proc = self._proc
        try:
            if proc.poll() is None:
                try:
                    # We always start a dedicated process group; stop the full group.
                    os.killpg(proc.pid, signal.SIGTERM)
                except (ProcessLookupError, PermissionError):
                    proc.terminate()

                try:
                    proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(proc.pid, signal.SIGKILL)
                    except (ProcessLookupError, PermissionError):
                        proc.kill()
                    proc.wait(timeout=2)
        finally:
            self._proc = None
            self._started = False
            self._session_id = None
            self._http.close()
            self._http = requests.Session()

    def start(self) -> "DoomInstance":
        """Start the Doom instance and wait for it to be ready."""
        if self._started:
            return self

        if not DOOM_BIN.exists():
            raise FileNotFoundError(f"Binary not found: {DOOM_BIN}")
        if not WAD_FILE.exists():
            raise FileNotFoundError(f"WAD not found: {WAD_FILE}")

        if not _port_available(self.config.port):
            raise RuntimeError(f"DMCP port {self.config.port} is already in use")

        env = os.environ.copy()
        env.update(
            {
                "SDL_VIDEODRIVER": "dummy",
                "SDL_AUDIODRIVER": "dummy",
                "SDL_NOMOUSE": "1",
                "DMCP_LOG_LEVEL": os.environ.get("DMCP_LOG_LEVEL", "warn"),
            }
        )

        cmd = [
            str(DOOM_BIN),
            "-iwad",
            str(WAD_FILE),
            "-warp",
            str(self.config.episode),
            str(self.config.map_num),
            "-skill",
            str(self.config.skill),
            "-dmcp_port",
            str(self.config.port),
        ]

        if self.config.nodraw:
            cmd.extend(["-nodraw"])
        if self.config.nosound:
            cmd.extend(["-nosound", "-nomusic", "-nosfx"])

        cmd.extend(["-nograb"])
        cmd.extend(self.config.extra_args)

        last_error: Optional[Exception] = None

        for attempt in range(START_RETRIES):
            self._proc = subprocess.Popen(
                cmd,
                env=env,
                stdin=subprocess.DEVNULL,
                start_new_session=True,
            )

            try:
                self._wait_for_ready(self.config.timeout)
                self._started = True
                return self
            except Exception as exc:
                last_error = exc
                self.stop()
                if attempt < START_RETRIES - 1:
                    time.sleep(0.5)

        if last_error:
            raise last_error

        raise RuntimeError("Failed to start Doom instance")

    def _is_process_running(self) -> bool:
        """Check if the process is still running."""
        if self._proc is None:
            return False
        return self._proc.poll() is None

    def _init_mcp_session(self) -> None:
        """Initialize MCP session with handshake."""
        init_payload = {
            "jsonrpc": "2.0",
            "id": 0,
            "method": "initialize",
            "params": {
                "protocolVersion": "2025-11-25",
                "capabilities": {},
                "clientInfo": {"name": "dmcp-e2e-test", "version": "0.6.0"},
            },
        }

        resp = self._http.post(
            self._mcp_url,
            json=init_payload,
            headers={
                "Content-Type": "application/json",
                "MCP-Protocol-Version": "2025-11-25",
            },
            timeout=REQUEST_TIMEOUT,
        )
        resp.raise_for_status()
        result = resp.json()

        if "error" in result:
            raise RuntimeError(f"MCP initialize failed: {result['error']}")

        self._session_id = result.get("result", {}).get("sessionId")
        if not self._session_id:
            raise RuntimeError("No sessionId in initialize response")

        initialized_payload = {
            "jsonrpc": "2.0",
            "method": "notifications/initialized",
        }

        self._http.post(
            self._mcp_url,
            json=initialized_payload,
            headers={
                "Content-Type": "application/json",
                "MCP-Protocol-Version": "2025-11-25",
                "MCP-Session-Id": self._session_id,
            },
            timeout=REQUEST_TIMEOUT,
        )

    def _wait_for_ready(self, timeout: float) -> None:
        """Wait for server to be healthy and in-level."""
        if STARTUP_INITIAL_DELAY > 0:
            time.sleep(STARTUP_INITIAL_DELAY)

        deadline = time.time() + timeout
        last_error = "unknown startup error"

        while time.time() < deadline:
            if not self._is_process_running():
                raise RuntimeError("Doom process crashed during startup")

            try:
                health = self._http.get(self._health_url, timeout=2)
                if health.status_code == 200:
                    if not self._session_id:
                        try:
                            self._init_mcp_session()
                        except (
                            requests.RequestException,
                            ValueError,
                            RuntimeError,
                        ) as exc:
                            last_error = f"session init failed: {exc}"
                            time.sleep(0.25)
                            continue

                    try:
                        state = self._get_state_raw()
                    except RuntimeError as exc:
                        last_error = str(exc)
                        time.sleep(0.25)
                        continue

                    if state.get("level", {}).get("gamestate") == "in_level":
                        return

                    last_error = "server healthy but not yet in_level"
                else:
                    last_error = f"health returned {health.status_code}"
            except requests.RequestException as exc:
                last_error = str(exc)

            time.sleep(0.25)

        raise TimeoutError(
            f"Server failed to start within {timeout}s on port "
            f"{self.config.port}: {last_error}"
        )

    def _get_state_raw(self) -> dict[str, Any]:
        """Get game state with a fast route-first strategy."""

        def _route_state() -> dict[str, Any]:
            resp = self._http.get(self._state_url, timeout=REQUEST_TIMEOUT)
            resp.raise_for_status()
            payload = resp.json()
            if isinstance(payload, dict):
                return payload
            return {}

        def _extract_json_result(data: dict[str, Any]) -> dict[str, Any]:
            if "error" in data:
                raise RuntimeError(f"MCP error: {data['error']}")

            result = data.get("result", {})
            content = data.get("content")
            if not isinstance(content, list) and isinstance(result, dict):
                content = result.get("content")

            if isinstance(content, list):
                for item in content:
                    if item.get("type") != "text":
                        continue

                    text = item.get("text", "")
                    if text.startswith("Use SSE"):
                        return {}

                    try:
                        parsed = json.loads(text)
                        if isinstance(parsed, dict):
                            return parsed
                    except ValueError:
                        continue

            if isinstance(result, dict):
                return result

            return {}

        def _call_tool(name: str, request_id: int) -> dict[str, Any]:
            params: dict[str, Any] = {"name": name}
            payload = {
                "jsonrpc": "2.0",
                "id": request_id,
                "method": "tools/call",
                "params": params,
            }

            headers = {
                "Content-Type": "application/json",
                "MCP-Protocol-Version": "2025-11-25",
            }
            if self._session_id:
                headers["MCP-Session-Id"] = self._session_id

            resp = self._http.post(
                self._mcp_url,
                json=payload,
                headers=headers,
                timeout=REQUEST_TIMEOUT,
            )
            resp.raise_for_status()
            return _extract_json_result(resp.json())

        for attempt in range(3):
            try:
                # Fast path: direct route provides full snapshot in one request.
                route_state = _route_state()
                if isinstance(route_state, dict) and route_state:
                    return route_state

                if not ALLOW_TOOL_FALLBACK:
                    return {}

                # Fallback: granular tool calls for compatibility.
                player_payload = _call_tool("get_player", request_id=1)
                map_payload = _call_tool("get_map", request_id=2)
                game_payload = _call_tool("get_game_info", request_id=3)
                enemies_payload = _call_tool("get_enemies", request_id=4)
                entities_payload = _call_tool("get_entities", request_id=5)
                inventory_payload = _call_tool("get_inventory", request_id=6)

                state: dict[str, Any] = {}
                if isinstance(player_payload, dict) and "player" in player_payload:
                    state["player"] = player_payload["player"]
                if isinstance(map_payload, dict) and "map" in map_payload:
                    state["level"] = map_payload["map"]
                if isinstance(game_payload, dict) and "game" in game_payload:
                    state["game"] = game_payload["game"]
                if isinstance(enemies_payload, dict):
                    state["enemies"] = enemies_payload.get("enemies", [])
                    state["enemy_count"] = enemies_payload.get("enemy_count", 0)
                if isinstance(entities_payload, dict):
                    state["entities"] = entities_payload.get("entities", [])
                    state["entity_count"] = entities_payload.get("entity_count", 0)
                if isinstance(inventory_payload, dict):
                    state["inventory"] = inventory_payload.get("inventory", [])
                    state["inventory_count"] = inventory_payload.get(
                        "inventory_count", 0
                    )

                return state
            except (requests.RequestException, ValueError):
                if attempt < 2:
                    time.sleep(0.4)
                    continue
                raise

        return {}

    def get_state(self) -> dict[str, Any]:
        """Get current game state."""
        return self._get_state_raw()

    def is_healthy(self) -> bool:
        """Check if instance is still responding."""
        if not self._started or not self._is_process_running():
            return False
        try:
            resp = self._http.get(self._health_url, timeout=2)
            return resp.status_code == 200
        except requests.RequestException:
            return False

    def __enter__(self):
        """Context manager entry."""
        return self.start()

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.stop()
        return False


# ============================================================================
# Fixtures
# ============================================================================


@pytest.fixture(scope="session")
def doom_binary():
    """Provide path to doom binary, skip if not found."""
    if not DOOM_BIN.exists():
        pytest.skip(f"Binary not found: {DOOM_BIN}")
    return DOOM_BIN


@pytest.fixture(scope="session")
def wad_file():
    """Provide path to WAD file, skip if not found."""
    if not WAD_FILE.exists():
        pytest.skip(f"WAD not found: {WAD_FILE}")
    return WAD_FILE


@pytest.fixture(scope="function")
def doom_instance(doom_binary, wad_file):
    """Factory fixture for creating configured Doom instances."""
    instances = []
    reserved_ports: set[int] = set()

    def _create_instance(
        episode: int = 1,
        map_num: int = 1,
        skill: int = 3,
        port: Optional[int] = None,
        timeout: float = STARTUP_TIMEOUT,
        **kwargs,
    ) -> DoomInstance:
        if port is None:
            for _ in range(20):
                candidate = _find_free_port()
                if candidate not in reserved_ports:
                    port = candidate
                    break
            else:
                raise RuntimeError("Unable to allocate a free DMCP port")

        if port < 1 or port > 65535:
            raise ValueError(f"Invalid DMCP port: {port}")

        reserved_ports.add(port)

        config = DoomConfig(
            episode=episode,
            map_num=map_num,
            skill=skill,
            port=port,
            timeout=timeout,
            **kwargs,
        )
        instance = DoomInstance(config)
        instances.append(instance)
        return instance

    yield _create_instance

    from .mcp_rpc import close_port_session

    for instance in instances:
        close_port_session(instance.config.port)
        instance.stop()


@pytest.fixture(scope="session")
def shared_default_game(doom_binary, wad_file):
    """Session-scoped default game for read-only state tests."""
    port = SESSION_DEFAULT_PORT if SESSION_DEFAULT_PORT > 0 else _find_free_port()
    instance = DoomInstance(DoomConfig(episode=1, map_num=1, skill=3, port=port))

    try:
        instance.start()
    except (RuntimeError, TimeoutError) as exc:
        pytest.skip(f"Unable to start shared default Doom instance: {exc}")

    yield instance
    from .mcp_rpc import close_port_session

    close_port_session(instance.config.port)
    instance.stop()


@pytest.fixture(scope="session", autouse=True)
def cleanup_e2e_rpc_sessions():
    """Close any cached e2e RPC sessions at the end of the test session."""
    yield
    from .mcp_rpc import close_all_sessions

    close_all_sessions()


@pytest.fixture(scope="function")
def fresh_game(shared_default_game):
    """Default game instance for state validation tests."""
    return shared_default_game


@pytest.fixture(scope="function")
def fresh_game_skill_1(doom_instance):
    """Fresh game on E1M1, I'm Too Young To Die."""
    require_extended_scenarios("Skill 1 startup is environment-dependent")
    instance = doom_instance(episode=1, map_num=1, skill=1)
    try:
        with instance as game:
            yield game
    except (RuntimeError, TimeoutError) as exc:
        pytest.skip(
            f"Skill 1 startup is unstable in headless {DOOM_ENGINE} Doom: {exc}"
        )


@pytest.fixture(scope="function")
def fresh_game_skill_5(doom_instance):
    """Fresh game on E1M1, Nightmare!"""
    require_extended_scenarios("Nightmare startup is environment-dependent")
    instance = doom_instance(episode=1, map_num=1, skill=5)
    try:
        with instance as game:
            yield game
    except (RuntimeError, TimeoutError) as exc:
        pytest.skip(f"Nightmare mode is unstable in headless {DOOM_ENGINE} Doom: {exc}")


@pytest.fixture(scope="function")
def fresh_e1m2(doom_instance):
    """Fresh game on E1M2."""
    require_extended_scenarios("E1M2 startup is environment-dependent")
    with doom_instance(episode=1, map_num=2, skill=3) as game:
        yield game


@pytest.fixture(scope="function")
def fresh_e1m3(doom_instance):
    """Fresh game on E1M3."""
    require_extended_scenarios("E1M3 startup is environment-dependent")
    with doom_instance(episode=1, map_num=3, skill=3) as game:
        yield game


@pytest.fixture(scope="function")
def fresh_e2m1(doom_instance):
    """Fresh game on E2M1 (requires registered Doom)."""
    require_extended_scenarios("E2M1 startup is environment-dependent")
    with doom_instance(episode=2, map_num=1, skill=3) as game:
        yield game
