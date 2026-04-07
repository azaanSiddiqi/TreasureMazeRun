"""
test_profile.py
---------------
Tests for PlayerProfile and Minimap layout logic.

This file has NO dependency on the C game engine or curses, so every
test runs in any environment including CI before make dist is run.
"""

import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

# Ensure treasure_runner package is importable regardless of invocation directory.
# This handles both: python -m pytest python/tests/ (from repo root)
# and: python -m unittest discover -s tests (from python/)
_PYTHON_DIR = Path(__file__).resolve().parents[1]
if str(_PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(_PYTHON_DIR))

# Set TREASURE_RUNNER_DIST so bindings._find_library() can locate libbackend.so
# even when invoked from the repo root or a CI environment.
_DIST_DIR = _PYTHON_DIR.parent / "dist"
if _DIST_DIR.exists():
    os.environ.setdefault("TREASURE_RUNNER_DIST", str(_DIST_DIR))

from treasure_runner.models.player_profile import PlayerProfile
from treasure_runner.ui.minimap import Minimap


class TestPlayerProfileLoad(unittest.TestCase):
    """Loading a new or existing profile."""

    def _make(self, path: str) -> PlayerProfile:
        return PlayerProfile(path)

    def test_new_profile_sets_needs_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._make(os.path.join(tmp, "p.json"))
            profile.load()
            self.assertTrue(profile.needs_name)

    def test_existing_profile_clears_needs_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "p.json")
            first = self._make(path)
            first.load()
            first.player_name = "Ada"
            first.save()
            second = self._make(path)
            second.load()
            self.assertFalse(second.needs_name)

    def test_new_profile_games_played_zero(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._make(os.path.join(tmp, "p.json"))
            profile.load()
            self.assertEqual(profile.games_played, 0)

    def test_new_profile_max_treasure_zero(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._make(os.path.join(tmp, "p.json"))
            profile.load()
            self.assertEqual(profile.max_treasure_collected, 0)

    def test_new_profile_most_rooms_zero(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._make(os.path.join(tmp, "p.json"))
            profile.load()
            self.assertEqual(profile.most_rooms_world_completed, 0)

    def test_new_profile_empty_timestamp(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._make(os.path.join(tmp, "p.json"))
            profile.load()
            self.assertEqual(profile.timestamp_last_played, "")

    def test_corrupt_json_creates_default(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "p.json")
            with open(path, "w", encoding="utf-8") as fobj:
                fobj.write("{ not valid json }")
            profile = self._make(path)
            profile.load()
            self.assertTrue(profile.needs_name)

    def test_partial_json_fills_defaults(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "p.json")
            with open(path, "w", encoding="utf-8") as fobj:
                json.dump({"player_name": "Turing"}, fobj)
            profile = self._make(path)
            profile.load()
            self.assertEqual(profile.player_name, "Turing")
            self.assertEqual(profile.games_played, 0)


class TestPlayerProfileSave(unittest.TestCase):
    """Saving and reloading a profile."""

    def test_save_creates_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "p.json")
            profile = PlayerProfile(path)
            profile.load()
            profile.player_name = "Test"
            profile.save()
            self.assertTrue(Path(path).exists())

    def test_save_creates_parent_dirs(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "a", "b", "p.json")
            profile = PlayerProfile(path)
            profile.load()
            profile.save()
            self.assertTrue(Path(path).exists())

    def test_reload_preserves_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "p.json")
            profile = PlayerProfile(path)
            profile.load()
            profile.player_name = "Ada"
            profile.save()
            profile2 = PlayerProfile(path)
            profile2.load()
            self.assertEqual(profile2.player_name, "Ada")

    def test_reload_preserves_games_played(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "p.json")
            profile = PlayerProfile(path)
            profile.load()
            profile.update_after_game(5, 3)
            profile.save()
            profile2 = PlayerProfile(path)
            profile2.load()
            self.assertEqual(profile2.games_played, 1)

    def test_json_has_all_required_keys(self):
        required = [
            "player_name",
            "games_played",
            "max_treasure_collected",
            "most_rooms_world_completed",
            "timestamp_last_played",
        ]
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "p.json")
            profile = PlayerProfile(path)
            profile.load()
            profile.player_name = "Test"
            profile.update_after_game(3, 2)
            profile.save()
            with open(path, encoding="utf-8") as fobj:
                data = json.load(fobj)
            for key in required:
                self.assertIn(key, data)


class TestPlayerProfileUpdate(unittest.TestCase):
    """update_after_game statistics tracking."""

    def _loaded(self, path: str) -> PlayerProfile:
        profile = PlayerProfile(path)
        profile.load()
        return profile

    def test_increments_games_played(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._loaded(os.path.join(tmp, "p.json"))
            profile.update_after_game(0, 0)
            self.assertEqual(profile.games_played, 1)

    def test_multiple_increments(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._loaded(os.path.join(tmp, "p.json"))
            for _ in range(3):
                profile.update_after_game(0, 0)
            self.assertEqual(profile.games_played, 3)

    def test_tracks_max_treasure(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._loaded(os.path.join(tmp, "p.json"))
            profile.update_after_game(10, 1)
            profile.update_after_game(5, 1)
            profile.update_after_game(15, 1)
            self.assertEqual(profile.max_treasure_collected, 15)

    def test_does_not_lower_max_treasure(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._loaded(os.path.join(tmp, "p.json"))
            profile.update_after_game(20, 1)
            profile.update_after_game(1, 1)
            self.assertEqual(profile.max_treasure_collected, 20)

    def test_tracks_max_rooms(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._loaded(os.path.join(tmp, "p.json"))
            profile.update_after_game(1, 7)
            profile.update_after_game(1, 3)
            self.assertEqual(profile.most_rooms_world_completed, 7)

    def test_does_not_lower_max_rooms(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._loaded(os.path.join(tmp, "p.json"))
            profile.update_after_game(1, 10)
            profile.update_after_game(1, 2)
            self.assertEqual(profile.most_rooms_world_completed, 10)

    def test_sets_timestamp(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._loaded(os.path.join(tmp, "p.json"))
            profile.update_after_game(0, 0)
            self.assertGreater(len(profile.timestamp_last_played), 0)

    def test_timestamp_ends_with_z(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._loaded(os.path.join(tmp, "p.json"))
            profile.update_after_game(0, 0)
            self.assertTrue(profile.timestamp_last_played.endswith("Z"))

    def test_zero_treasure_does_not_set_max(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = self._loaded(os.path.join(tmp, "p.json"))
            profile.update_after_game(0, 0)
            self.assertEqual(profile.max_treasure_collected, 0)


class TestPlayerProfileProperties(unittest.TestCase):
    """Property getters, setters, and summary_lines."""

    def test_player_name_setter(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = PlayerProfile(os.path.join(tmp, "p.json"))
            profile.load()
            profile.player_name = "Babbage"
            self.assertEqual(profile.player_name, "Babbage")

    def test_path_stored(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "p.json")
            profile = PlayerProfile(path)
            self.assertEqual(profile.path, path)

    def test_summary_lines_is_list(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = PlayerProfile(os.path.join(tmp, "p.json"))
            profile.load()
            self.assertIsInstance(profile.summary_lines(), list)

    def test_summary_lines_nonempty(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = PlayerProfile(os.path.join(tmp, "p.json"))
            profile.load()
            self.assertGreater(len(profile.summary_lines()), 0)

    def test_summary_lines_contain_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = PlayerProfile(os.path.join(tmp, "p.json"))
            profile.load()
            profile.player_name = "UniqueXYZ123"
            found = any("UniqueXYZ123" in line for line in profile.summary_lines())
            self.assertTrue(found)

    def test_summary_lines_all_strings(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = PlayerProfile(os.path.join(tmp, "p.json"))
            profile.load()
            for line in profile.summary_lines():
                self.assertIsInstance(line, str)

    def test_summary_shows_never_for_empty_timestamp(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = PlayerProfile(os.path.join(tmp, "p.json"))
            profile.load()
            found = any("never" in line for line in profile.summary_lines())
            self.assertTrue(found)


class TestMinimapBFS(unittest.TestCase):
    """Minimap BFS level-building -- no curses window needed."""

    def test_empty_minimap_has_no_levels(self):
        minimap = Minimap([], [])
        self.assertEqual(minimap.level_count, 0)

    def test_single_room_one_level(self):
        minimap = Minimap([[0]], [5])
        self.assertEqual(minimap.level_count, 1)
        self.assertEqual(minimap.level_size(0), 1)

    def test_two_connected_rooms_two_levels(self):
        matrix = [[0, 1], [0, 0]]
        minimap = Minimap(matrix, [1, 2])
        self.assertEqual(minimap.level_count, 2)

    def test_disconnected_room_still_appears(self):
        matrix = [[0, 0], [0, 0]]
        minimap = Minimap(matrix, [1, 2])
        all_nodes = sorted(minimap.all_node_indices())
        self.assertEqual(all_nodes, [0, 1])

    def test_level_pixel_width_single_node(self):
        minimap = Minimap([[0]], [1])
        self.assertEqual(minimap.pixel_width_for_level(0), Minimap.BOX_WIDTH)

    def test_level_pixel_width_two_nodes(self):
        # Star: node 0 connects to nodes 1 and 2, so level 0 has 1 node,
        # level 1 has 2 nodes -> pixel_width_for_level(1) = 2*BOX_WIDTH+BOX_GAP
        matrix = [[0, 1, 1], [0, 0, 0], [0, 0, 0]]
        minimap = Minimap(matrix, [1, 2, 3])
        expected = 2 * Minimap.BOX_WIDTH + Minimap.BOX_GAP
        self.assertEqual(minimap.pixel_width_for_level(1), expected)

    def test_three_room_chain_three_levels(self):
        matrix = [[0, 1, 0], [0, 0, 1], [0, 0, 0]]
        minimap = Minimap(matrix, [10, 20, 30])
        self.assertEqual(minimap.level_count, 3)

    def test_all_rooms_appear_exactly_once(self):
        matrix = [[0, 1, 0], [0, 0, 1], [1, 0, 0]]
        minimap = Minimap(matrix, [1, 2, 3])
        self.assertEqual(sorted(minimap.all_node_indices()), [0, 1, 2])


if __name__ == "__main__":
    unittest.main()
