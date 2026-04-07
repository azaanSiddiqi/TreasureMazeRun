#!/usr/bin/env python3
"""
run_game.py
-----------
Entry point for Treasure Runner A3.

Usage:
    python run_game.py --config assets/starter.ini --profile assets/my_profile.json

The --config flag specifies the world generator .ini file.
The --profile flag specifies the player profile JSON file; it is created
automatically if it does not exist (in which case the player is prompted
for their name before the game begins).
"""

import sys
import os
import argparse
from pathlib import Path

# Ensure the treasure_runner package can be found regardless of where
# the script is invoked from.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from treasure_runner.models.game_engine import GameEngine
from treasure_runner.ui.game_ui import GameUI, PlayerProfile


def parse_args():
    parser = argparse.ArgumentParser(
        description="Treasure Runner – A3",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python run_game.py --config assets/starter.ini "
            "--profile assets/ada.json\n"
        ),
    )
    parser.add_argument(
        "--config",
        required=True,
        metavar="PATH",
        help="Path to the world generator .ini config file",
    )
    parser.add_argument(
        "--profile",
        required=True,
        metavar="PATH",
        help=(
            "Path to the player profile JSON file "
            "(created automatically if missing)"
        ),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    config_path  = os.path.abspath(args.config)
    profile_path = os.path.abspath(args.profile)

    # Load (or create) the player profile
    profile = PlayerProfile(profile_path)
    profile.load()

    # Build the C game engine
    engine = GameEngine(config_path)

    try:
        ui = GameUI(engine, profile)
        ui.run()
    finally:
        engine.destroy()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
