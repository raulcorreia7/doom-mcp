from __future__ import annotations

import argparse
import asyncio
import json
import shlex
import sys
from typing import List, Optional

from . import contract, workflows
from .client import DMCPClient
from .errors import DMCPError
from .output import write


class ShellArgumentParser(argparse.ArgumentParser):
    def error(self, message: str) -> None:
        raise DMCPError(message)


def build_parser(
    parser_class: type[argparse.ArgumentParser] = argparse.ArgumentParser,
) -> argparse.ArgumentParser:
    parser = parser_class(description="DMCP MCP helper for agentic CLIs")
    parser.add_argument("--url", default="http://127.0.0.1:6060/mcp")
    parser.add_argument("--pretty", action="store_true")
    sub = parser.add_subparsers(dest="command", required=True, parser_class=parser_class)

    add_content_parser(sub)
    add_read_parser(sub)
    add_command_parsers(sub)
    add_input_parsers(sub)
    add_utility_parsers(sub)

    return parser


def add_content_parser(sub: argparse._SubParsersAction[argparse.ArgumentParser]) -> None:
    p_content = sub.add_parser("content", help="list available Doom content")
    p_content.add_argument("target", nargs="?", default="all", choices=sorted(contract.CONTENT_TOOLS))
    p_content.add_argument("--mode", choices=contract.CONTENT_MODES)
    p_content.set_defaults(func=workflows.content)

    p_brief = sub.add_parser("brief", help="compact state and content summary")
    p_brief.add_argument("--limit", type=int, default=8)
    p_brief.set_defaults(func=workflows.brief)


def add_read_parser(sub: argparse._SubParsersAction[argparse.ArgumentParser]) -> None:
    p_read = sub.add_parser("read", help="call one granular read tool")
    p_read.add_argument("target", choices=sorted(contract.READ_TOOLS))
    p_read.add_argument("--offset", type=int)
    p_read.add_argument("--limit", type=int)
    p_read.add_argument("--status", choices=contract.STATUS_VALUES)
    p_read.add_argument("--kind", choices=contract.ENTITY_KIND_VALUES)
    p_read.set_defaults(func=workflows.read)


def add_command_parsers(sub: argparse._SubParsersAction[argparse.ArgumentParser]) -> None:
    p_spawn = sub.add_parser("spawn", help="spawn one available entity")
    p_spawn.add_argument("entity_class")
    p_spawn.add_argument("--x", type=float, required=True)
    p_spawn.add_argument("--y", type=float, required=True)
    p_spawn.add_argument("--angle", type=float, default=0.0)
    p_spawn.add_argument("--tid", type=int)
    add_wait_options(p_spawn)
    p_spawn.set_defaults(func=workflows.spawn)

    p_spawn_many = sub.add_parser("spawn-many", help="batch spawn canonical entity objects")
    p_spawn_many.add_argument(
        "--spawns-json",
        required=True,
        help=(
            'JSON array: [{"entity_class":"DoomImp","x":160,"y":96,"angle":0}]'
        ),
    )
    add_wait_options(p_spawn_many)
    p_spawn_many.set_defaults(func=workflows.spawn_batch)

    p_give = sub.add_parser("give", help="give an available item")
    p_give.add_argument("item_class")
    p_give.add_argument("--amount", type=int, default=1)
    add_wait_options(p_give)
    p_give.set_defaults(func=workflows.give_item)

    p_give_many = sub.add_parser("give-many", help="batch give canonical item objects")
    p_give_many.add_argument(
        "--items-json",
        required=True,
        help='JSON array: [{"item_class":"Shotgun","amount":1},{"item_class":"Shells","amount":20}]',
    )
    add_wait_options(p_give_many)
    p_give_many.set_defaults(func=workflows.give_batch)

    p_level = sub.add_parser("level", help="change level after content validation")
    p_level.add_argument("map_name")
    p_level.add_argument("--skill-level", type=int, default=3, choices=range(1, 6))
    p_level.add_argument("--reset-inventory", action="store_true")
    add_wait_options(p_level)
    p_level.set_defaults(func=workflows.level)

    p_health = sub.add_parser("set-health", help="set player health")
    p_health.add_argument("health", type=float)
    add_wait_options(p_health)
    p_health.set_defaults(func=workflows.set_health)

    p_position = sub.add_parser("set-position", help="set player position")
    p_position.add_argument("--x", type=float, required=True)
    p_position.add_argument("--y", type=float, required=True)
    p_position.add_argument("--angle", type=float, default=0.0)
    add_wait_options(p_position)
    p_position.set_defaults(func=workflows.set_position)

    p_pause = sub.add_parser("pause", help="pause or unpause the game")
    p_pause.add_argument("paused", choices=("true", "false"))
    add_wait_options(p_pause)
    p_pause.set_defaults(func=workflows.pause)

    p_damage = sub.add_parser("damage", help="damage an entity by TID")
    p_damage.add_argument("target_tid", type=int)
    p_damage.add_argument("damage", type=float)
    p_damage.add_argument("--damage-type", default="Normal")
    add_wait_options(p_damage)
    p_damage.set_defaults(func=workflows.damage)

    p_kill = sub.add_parser("kill", help="kill an entity by TID")
    p_kill.add_argument("target_tid", type=int)
    add_wait_options(p_kill)
    p_kill.set_defaults(func=workflows.kill)

    p_console = sub.add_parser("console", help="execute an engine console command")
    p_console.add_argument("command_text")
    add_wait_options(p_console)
    p_console.set_defaults(func=workflows.console)

    p_batch = sub.add_parser("batch", help="execute canonical batch command calls")
    p_batch.add_argument(
        "--calls-json",
        required=True,
        help='JSON array: [{"name":"give_item","arguments":{"item_class":"Shotgun","amount":1}}]',
    )
    add_wait_options(p_batch)
    p_batch.set_defaults(func=workflows.batch)


def add_input_parsers(sub: argparse._SubParsersAction[argparse.ArgumentParser]) -> None:
    p_input = sub.add_parser("input", help="send one tick of player input")
    p_input.add_argument("action", choices=contract.INPUT_ACTIONS)
    p_input.add_argument(
        "--value",
        type=float,
        help=f"Aim angle or weapon slot. {contract.WEAPON_SLOT_HELP}",
    )
    p_input.set_defaults(func=workflows.input_tick)

    p_weapon = sub.add_parser("weapon", help="switch weapon by Doom slot")
    p_weapon.add_argument("slot", type=int, choices=range(1, 8))
    p_weapon.set_defaults(func=workflows.weapon)

    p_input_plan = sub.add_parser("input-plan", help="repeat one input for N ticks")
    p_input_plan.add_argument("action", choices=contract.INPUT_ACTIONS)
    p_input_plan.add_argument(
        "--value",
        type=float,
        help=f"Aim angle or weapon slot. {contract.WEAPON_SLOT_HELP}",
    )
    p_input_plan.add_argument("--ticks", type=int, default=1)
    p_input_plan.set_defaults(func=workflows.input_plan)


def add_utility_parsers(sub: argparse._SubParsersAction[argparse.ArgumentParser]) -> None:
    p_shell = sub.add_parser("shell", help="keep one MCP connection open for multiple commands")
    p_shell.add_argument("--no-prompt", action="store_true")

    p_result = sub.add_parser("result", help="read queued command result(s)")
    p_result.add_argument("--sequences-json", required=True, help="JSON array of command sequence ids")
    p_result.set_defaults(func=workflows.command_result)

    p_screenshot = sub.add_parser(
        "screenshot", help="print ASCII screenshot when get_screenshot is exposed"
    )
    p_screenshot.set_defaults(func=workflows.screenshot)

    p_tools = sub.add_parser("tools", help="list MCP tools")
    p_tools.add_argument("--compact", action="store_true")
    p_tools.add_argument("--limit", type=int, default=16)
    p_tools.set_defaults(func=workflows.tools)


def add_wait_options(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--no-wait",
        action="store_true",
        help="return queued sequence ids without polling command completion",
    )
    parser.add_argument(
        "--wait-timeout",
        type=float,
        default=workflows.DEFAULT_WAIT_TIMEOUT_SECONDS,
        help="seconds to wait for queued command completion",
    )


async def async_main(argv: Optional[List[str]] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        async with DMCPClient(args.url) as client:
            if args.command == "shell":
                return await run_shell(client, args)
            value = await args.func(client, args)
            write(value, args.pretty)
            if workflows.result_failed(value):
                return 1
    except (DMCPError, ValueError, json.JSONDecodeError, OSError) as exc:
        print(f"dmcp-agent: {exc}", file=sys.stderr)
        return 1
    return 0


async def run_shell(client: DMCPClient, args: argparse.Namespace) -> int:
    parser = build_parser(ShellArgumentParser)
    interactive = sys.stdin.isatty() and not args.no_prompt
    prompt = "dmcp> " if interactive else ""
    had_error = False

    while True:
        try:
            line = input(prompt)
        except EOFError:
            return 1 if had_error and not interactive else 0

        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line in {"exit", "quit"}:
            return 1 if had_error and not interactive else 0

        try:
            line_args = parser.parse_args(shlex.split(line))
            if line_args.command == "shell":
                raise DMCPError("nested shell sessions are not supported")
            value = await line_args.func(client, line_args)
            write(value, args.pretty or line_args.pretty)
            if workflows.result_failed(value):
                had_error = True
        except SystemExit:
            had_error = True
            continue
        except (DMCPError, ValueError, json.JSONDecodeError, OSError) as exc:
            had_error = True
            print(f"dmcp-agent: {exc}", file=sys.stderr)


def main() -> int:
    return asyncio.run(async_main())


if __name__ == "__main__":
    raise SystemExit(main())
