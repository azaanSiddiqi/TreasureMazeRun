"""
player_profile.py
-----------------
Persistent player profile stored as JSON.

This module has NO dependency on the C game engine or curses, so it
can be imported and tested independently of the rest of the system.

Minimum required JSON format (from the A3 specification)::

    {
        "player_name": "Ada",
        "games_played": 3,
        "max_treasure_collected": 41,
        "most_rooms_world_completed": 19,
        "timestamp_last_played": "2026-03-05T21:15:05Z"
    }
"""

import datetime
import json
from pathlib import Path


class PlayerProfile:
    """Load, update, and save the JSON player profile.

    Usage::

        profile = PlayerProfile("assets/ada.json")
        profile.load()           # creates defaults if file missing
        if profile.needs_name:
            profile.player_name = input("Name: ")
        profile.update_after_game(treasure_count=10, rooms_visited=3)
        profile.save()
    """

    DEFAULTS: dict = {
        "player_name":                "Player",
        "games_played":               0,
        "max_treasure_collected":     0,
        "most_rooms_world_completed": 0,
        "timestamp_last_played":      "",
    }

    def __init__(self, path: str):
        self.path: str = path
        self._data: dict = {}
        self._needs_name: bool = False

    # ------------------------------------------------------------------
    # Load / save
    # ------------------------------------------------------------------

    def load(self) -> None:
        """Load from disk, creating a default profile when the file is missing."""
        profile_path = Path(self.path)
        if profile_path.exists():
            try:
                with open(profile_path, encoding="utf-8") as file_handle:
                    self._data = json.load(file_handle)
                for key, default_value in self.DEFAULTS.items():
                    self._data.setdefault(key, default_value)
            except (json.JSONDecodeError, OSError):
                self._data = dict(self.DEFAULTS)
                self._needs_name = True
        else:
            self._data = dict(self.DEFAULTS)
            self._needs_name = True

    def save(self) -> None:
        """Write the profile to disk, creating parent directories as needed."""
        profile_path = Path(self.path)
        profile_path.parent.mkdir(parents=True, exist_ok=True)
        with open(profile_path, "w", encoding="utf-8") as file_handle:
            json.dump(self._data, file_handle, indent=2)

    # ------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------

    @property
    def needs_name(self) -> bool:
        """True when the profile was just created and a name should be prompted."""
        return self._needs_name

    @property
    def player_name(self) -> str:
        """The player's display name."""
        return self._data.get("player_name", "Player")

    @player_name.setter
    def player_name(self, value: str) -> None:
        self._data["player_name"] = value

    @property
    def games_played(self) -> int:
        """Total number of completed game sessions."""
        return self._data.get("games_played", 0)

    @property
    def max_treasure_collected(self) -> int:
        """Most treasures collected in a single session."""
        return self._data.get("max_treasure_collected", 0)

    @property
    def most_rooms_world_completed(self) -> int:
        """Most rooms visited in a single session."""
        return self._data.get("most_rooms_world_completed", 0)

    @property
    def timestamp_last_played(self) -> str:
        """ISO-8601 UTC timestamp of the most recent session."""
        return self._data.get("timestamp_last_played", "")

    # ------------------------------------------------------------------
    # Mutation
    # ------------------------------------------------------------------

    def update_after_game(self, treasure_count: int, rooms_visited: int) -> None:
        """Update statistics at the end of a game run.

        Args:
            treasure_count: treasures collected this session.
            rooms_visited:  distinct rooms visited this session.
        """
        self._data["games_played"] = self.games_played + 1
        if treasure_count > self.max_treasure_collected:
            self._data["max_treasure_collected"] = treasure_count
        if rooms_visited > self.most_rooms_world_completed:
            self._data["most_rooms_world_completed"] = rooms_visited
        self._data["timestamp_last_played"] = (
            datetime.datetime.now(datetime.timezone.utc)
            .strftime("%Y-%m-%dT%H:%M:%SZ")
        )

    # ------------------------------------------------------------------
    # Display helpers
    # ------------------------------------------------------------------

    def summary_lines(self) -> list:
        """Return display strings summarising the profile for splash screens."""
        last_played = self.timestamp_last_played or "never"
        return [
            f"  Player:        {self.player_name}",
            f"  Games played:  {self.games_played}",
            f"  Best treasure: {self.max_treasure_collected}",
            f"  Best rooms:    {self.most_rooms_world_completed}",
            f"  Last played:   {last_played}",
        ]
