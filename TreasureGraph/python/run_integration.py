#!/usr/bin/env python3
"""Deterministic system integration test runner for Treasure Runner."""

import os
import argparse
from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameError, ImpassableError


def player_state_str(engine: GameEngine) -> str:
    room = engine.player.get_room()
    x, y = engine.player.get_position()
    cnt = engine.player.get_collected_count()
    return f"room={room}|x={x}|y={y}|collected={cnt}"


def find_entry_direction(engine: GameEngine) -> Direction:
    spawn_room = engine.player.get_room()
    spawn_pos = engine.player.get_position()

    for direction in [Direction.SOUTH, Direction.WEST,
                      Direction.NORTH, Direction.EAST]:
        engine.reset()
        try:
            engine.move_player(direction)
        except GameError:
            continue

        new_room = engine.player.get_room()
        new_pos = engine.player.get_position()

        if new_room == spawn_room and new_pos != spawn_pos:
            return direction

    raise RuntimeError("No valid entry direction found")


def do_sweep(engine: GameEngine, sweep_dir: Direction,
             phase_name: str, step: int, log_lines: list) -> int:
    log_lines.append(
        f"SWEEP_START|phase={phase_name}|dir={sweep_dir.name}")

    seen_states: set = set()
    ok_moves = 0
    reason = "BLOCKED"

    while True:
        before = player_state_str(engine)

        if before in seen_states:
            reason = "CYCLE_DETECTED"
            break
        seen_states.add(before)

        step += 1
        before_cnt = int(before.split("collected=")[1])

        try:
            engine.move_player(sweep_dir)
            after = player_state_str(engine)
            after_cnt = int(after.split("collected=")[1])
            delta = after_cnt - before_cnt

            if before == after:
                log_lines.append(
                    f"MOVE|step={step}|phase={phase_name}"
                    f"|dir={sweep_dir.name}|result=BLOCKED"
                    f"|before={before}|after={after}"
                    f"|delta_collected={delta}"
                )
                reason = "BLOCKED"
                break

            log_lines.append(
                f"MOVE|step={step}|phase={phase_name}"
                f"|dir={sweep_dir.name}|result=OK"
                f"|before={before}|after={after}"
                f"|delta_collected={delta}"
            )
            ok_moves += 1

        except ImpassableError:
            after = player_state_str(engine)
            after_cnt = int(after.split("collected=")[1])
            delta = after_cnt - before_cnt
            log_lines.append(
                f"MOVE|step={step}|phase={phase_name}"
                f"|dir={sweep_dir.name}|result=BLOCKED"
                f"|before={before}|after={after}"
                f"|delta_collected={delta}"
            )
            reason = "BLOCKED"
            break

        except GameError as exc:
            after = player_state_str(engine)
            after_cnt = int(after.split("collected=")[1])
            delta = after_cnt - before_cnt
            log_lines.append(
                f"MOVE|step={step}|phase={phase_name}"
                f"|dir={sweep_dir.name}|result=ERROR"
                f"|before={before}|after={after}"
                f"|delta_collected={delta}"
                f"|error={exc}"
            )
            reason = "BLOCKED"
            break

    log_lines.append(
        f"SWEEP_END|phase={phase_name}|reason={reason}|moves={ok_moves}")
    return step


def run_integration(config_path: str) -> list:
    log_lines: list = []
    engine = GameEngine(config_path)

    room_count = engine.get_room_count()
    width, height = engine.get_room_dimensions()

    log_lines.append(
        f"RUN_START|config={config_path}"
        f"|rooms={room_count}|room_width={width}|room_height={height}"
    )

    spawn_state = player_state_str(engine)
    log_lines.append(f"STATE|step=0|phase=SPAWN|state={spawn_state}")

    entry_dir = find_entry_direction(engine)
    log_lines.append(f"ENTRY|direction={entry_dir.name}")

    engine.reset()
    step = 0
    before = player_state_str(engine)
    before_cnt = int(before.split("collected=")[1])
    step += 1

    try:
        engine.move_player(entry_dir)
        after = player_state_str(engine)
        after_cnt = int(after.split("collected=")[1])
        delta = after_cnt - before_cnt
        log_lines.append(
            f"MOVE|step={step}|phase=ENTRY|dir={entry_dir.name}"
            f"|result=OK"
            f"|before={before}|after={after}"
            f"|delta_collected={delta}"
        )
    except GameError as exc:
        after = player_state_str(engine)
        after_cnt = int(after.split("collected=")[1])
        delta = after_cnt - before_cnt
        log_lines.append(
            f"MOVE|step={step}|phase=ENTRY|dir={entry_dir.name}"
            f"|result=ERROR"
            f"|before={before}|after={after}"
            f"|delta_collected={delta}"
            f"|error={exc}"
        )
        log_lines.append("TERMINATED: Initial Move Error")
        log_lines.append(f"RUN_END|steps={step}|collected_total=0")
        engine.destroy()
        return log_lines

    sweeps = [
        (Direction.SOUTH, "SWEEP_SOUTH"),
        (Direction.WEST,  "SWEEP_WEST"),
        (Direction.NORTH, "SWEEP_NORTH"),
        (Direction.EAST,  "SWEEP_EAST"),
    ]
    for sweep_dir, phase_name in sweeps:
        step = do_sweep(engine, sweep_dir, phase_name, step, log_lines)

    final_state = player_state_str(engine)
    log_lines.append(f"STATE|step={step}|phase=FINAL|state={final_state}")

    total_collected = engine.player.get_collected_count()
    log_lines.append(f"RUN_END|steps={step}|collected_total={total_collected}")

    engine.destroy()
    return log_lines


def parse_args():
    parser = argparse.ArgumentParser(
        description="Treasure Runner integration test logger")
    parser.add_argument("--config", required=True,
                        help="Path to generator config file")
    parser.add_argument("--log", required=True,
                        help="Output log path")
    return parser.parse_args()


def main():
    args = parse_args()
    config_path = os.path.abspath(args.config)
    log_path = os.path.abspath(args.log)

    lines = run_integration(config_path)

    with open(log_path, "w", encoding="utf-8") as fh:
        for line in lines:
            fh.write(line + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())