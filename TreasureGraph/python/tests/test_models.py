"""
test_models.py
--------------
Unit tests for A2 bindings/models (regression) and A3 model additions.

All tests here require the C library (libbackend.so).  They are skipped
automatically when the library is absent so the file stays importable
in any environment.
"""

import os
import sys
import unittest
from pathlib import Path

# Ensure treasure_runner package is importable regardless of invocation directory.
_PYTHON_DIR = Path(__file__).resolve().parents[1]
if str(_PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(_PYTHON_DIR))

# Set TREASURE_RUNNER_DIST so bindings._find_library() locates libbackend.so.
_DIST_DIR = _PYTHON_DIR.parent / "dist"
if _DIST_DIR.exists():
    os.environ.setdefault("TREASURE_RUNNER_DIST", str(_DIST_DIR))

# Guard: skip all C-dependent tests when the library isn't built yet.
_LIB_OK = False
try:
    from treasure_runner.bindings import lib, Status, Direction, Treasure, GameEngineStatus
    from treasure_runner.models.game_engine import GameEngine
    from treasure_runner.models.player import Player
    from treasure_runner.models.exceptions import (
        GameError,
        GameEngineError,
        ImpassableError,
        InvalidArgumentError,
        NoSuchRoomError,
        StatusError,
        StatusImpassableError,
        StatusInvalidArgumentError,
        status_to_exception,
        status_to_status_exception,
    )
    _LIB_OK = True
except Exception:
    pass

_SKIP = unittest.skipUnless(_LIB_OK, "C library not available")

_REPO_ROOT = Path(__file__).resolve().parents[2]
_CONFIG_CANDIDATES = [
    _REPO_ROOT / "assets" / "treasure_runner_azaan.ini",
    _REPO_ROOT / "assets" / "treasure_runner.ini",
    _REPO_ROOT / "assets" / "starter.ini",
]
_CONFIG = str(next((p for p in _CONFIG_CANDIDATES if p.exists()),
                   _CONFIG_CANDIDATES[-1]))


# ============================================================
# A2 regression: bindings
# ============================================================

@_SKIP
class TestBindingsImport(unittest.TestCase):
    def test_lib_not_none(self):
        self.assertIsNotNone(lib)

    def test_status_ok_is_zero(self):
        self.assertEqual(Status.OK, 0)

    def test_status_impassable_value(self):
        self.assertEqual(Status.ROOM_IMPASSABLE, 6)

    def test_direction_north_is_zero(self):
        self.assertEqual(Direction.NORTH, 0)

    def test_direction_east_value(self):
        self.assertEqual(Direction.EAST, 2)

    def test_game_engine_status_alias(self):
        self.assertIs(GameEngineStatus, Status)

    def test_treasure_struct_id_field(self):
        treasure = Treasure()
        treasure.id = 42
        self.assertEqual(treasure.id, 42)

    def test_treasure_struct_collected_field(self):
        treasure = Treasure()
        treasure.collected = False
        self.assertFalse(treasure.collected)

    def test_all_directions_defined(self):
        for direction in [Direction.NORTH, Direction.SOUTH,
                          Direction.EAST, Direction.WEST]:
            self.assertIsNotNone(direction)

    def test_all_status_values_defined(self):
        for status_val in [Status.OK, Status.INVALID_ARGUMENT,
                           Status.NULL_POINTER, Status.NO_MEMORY,
                           Status.INTERNAL_ERROR, Status.ROOM_IMPASSABLE,
                           Status.GE_NO_SUCH_ROOM]:
            self.assertIsNotNone(status_val)


# ============================================================
# A2 regression: GameEngine create/destroy
# ============================================================

@_SKIP
class TestGameEngineCreate(unittest.TestCase):
    def test_create_valid(self):
        eng = GameEngine(_CONFIG)
        self.assertIsNotNone(eng)
        eng.destroy()

    def test_create_invalid_config(self):
        with self.assertRaises(GameEngineError):
            GameEngine("/nonexistent/path.ini")

    def test_player_accessible(self):
        eng = GameEngine(_CONFIG)
        self.assertIsNotNone(eng.player)
        eng.destroy()

    def test_player_is_player_instance(self):
        eng = GameEngine(_CONFIG)
        self.assertIsInstance(eng.player, Player)
        eng.destroy()

    def test_destroy_idempotent(self):
        eng = GameEngine(_CONFIG)
        eng.destroy()
        eng.destroy()


# ============================================================
# A2 regression: queries
# ============================================================

@_SKIP
class TestGameEngineQueries(unittest.TestCase):
    def setUp(self):
        self.eng = GameEngine(_CONFIG)

    def tearDown(self):
        self.eng.destroy()

    def test_room_count_positive(self):
        self.assertGreater(self.eng.get_room_count(), 0)

    def test_room_dimensions_positive(self):
        width, height = self.eng.get_room_dimensions()
        self.assertGreater(width, 0)
        self.assertGreater(height, 0)

    def test_room_ids_is_list(self):
        self.assertIsInstance(self.eng.get_room_ids(), list)

    def test_room_ids_nonempty(self):
        self.assertGreater(len(self.eng.get_room_ids()), 0)

    def test_render_contains_wall(self):
        self.assertIn('#', self.eng.render_current_room())

    def test_render_contains_player(self):
        self.assertIn('@', self.eng.render_current_room())

    def test_render_has_newlines(self):
        self.assertIn('\n', self.eng.render_current_room())

    def test_render_is_string(self):
        self.assertIsInstance(self.eng.render_current_room(), str)


# ============================================================
# A2 regression: movement
# ============================================================

@_SKIP
class TestGameEngineMovement(unittest.TestCase):
    def setUp(self):
        self.eng = GameEngine(_CONFIG)

    def tearDown(self):
        self.eng.destroy()

    def test_move_or_blocked_each_direction(self):
        for direction in [Direction.EAST, Direction.WEST,
                          Direction.NORTH, Direction.SOUTH]:
            try:
                self.eng.move_player(direction)
            except ImpassableError:
                pass

    def test_reset_restores_position(self):
        initial_pos  = self.eng.player.get_position()
        initial_room = self.eng.player.get_room()
        try:
            self.eng.move_player(Direction.EAST)
        except ImpassableError:
            pass
        self.eng.reset()
        self.assertEqual(self.eng.player.get_room(), initial_room)
        self.assertEqual(self.eng.player.get_position(), initial_pos)

    def test_reset_clears_collected(self):
        self.eng.reset()
        self.assertEqual(self.eng.player.get_collected_count(), 0)


# ============================================================
# A2 regression: Player methods
# ============================================================

@_SKIP
class TestPlayerMethods(unittest.TestCase):
    def setUp(self):
        self.eng    = GameEngine(_CONFIG)
        self.player = self.eng.player

    def tearDown(self):
        self.eng.destroy()

    def test_get_room_is_int(self):
        self.assertIsInstance(self.player.get_room(), int)

    def test_get_room_nonnegative(self):
        self.assertGreaterEqual(self.player.get_room(), 0)

    def test_get_position_is_tuple(self):
        self.assertIsInstance(self.player.get_position(), tuple)

    def test_get_position_length_two(self):
        self.assertEqual(len(self.player.get_position()), 2)

    def test_position_in_bounds(self):
        width, height = self.eng.get_room_dimensions()
        pos_x, pos_y  = self.player.get_position()
        self.assertGreaterEqual(pos_x, 0)
        self.assertLess(pos_x, width)
        self.assertGreaterEqual(pos_y, 0)
        self.assertLess(pos_y, height)

    def test_collected_count_starts_zero(self):
        self.assertEqual(self.player.get_collected_count(), 0)

    def test_has_collected_false_for_unknown(self):
        self.assertFalse(self.player.has_collected_treasure(99999))

    def test_collected_treasures_is_list(self):
        self.assertIsInstance(self.player.get_collected_treasures(), list)

    def test_collected_treasures_empty_at_start(self):
        self.assertEqual(len(self.player.get_collected_treasures()), 0)


# ============================================================
# A2 regression: exceptions
# ============================================================

@_SKIP
class TestExceptions(unittest.TestCase):
    def test_impassable_maps_correctly(self):
        self.assertIsInstance(
            status_to_exception(Status.ROOM_IMPASSABLE), ImpassableError)

    def test_invalid_arg_maps_correctly(self):
        self.assertIsInstance(
            status_to_exception(Status.INVALID_ARGUMENT), InvalidArgumentError)

    def test_no_such_room_maps_correctly(self):
        self.assertIsInstance(
            status_to_exception(Status.GE_NO_SUCH_ROOM), NoSuchRoomError)

    def test_internal_error_is_game_error(self):
        self.assertIsInstance(
            status_to_exception(Status.INTERNAL_ERROR), GameError)

    def test_status_exception_invalid_arg(self):
        self.assertIsInstance(
            status_to_status_exception(Status.INVALID_ARGUMENT),
            StatusInvalidArgumentError)

    def test_status_exception_impassable(self):
        self.assertIsInstance(
            status_to_status_exception(Status.ROOM_IMPASSABLE),
            StatusImpassableError)

    def test_impassable_inherits_chain(self):
        self.assertIsInstance(ImpassableError("x"), GameEngineError)
        self.assertIsInstance(ImpassableError("x"), GameError)

    def test_status_impassable_inherits_status_error(self):
        self.assertIsInstance(StatusImpassableError("x"), StatusError)

    def test_invalid_argument_inherits_game_engine_error(self):
        self.assertIsInstance(InvalidArgumentError("x"), GameEngineError)


# ============================================================
# A3: portal traversal
# ============================================================

@_SKIP
class TestUsePortal(unittest.TestCase):
    def setUp(self):
        self.eng = GameEngine(_CONFIG)

    def tearDown(self):
        self.eng.destroy()

    def test_use_portal_raises_or_succeeds(self):
        """Portal use either succeeds (room change) or raises — never crashes."""
        try:
            self.eng.use_portal()
        except GameError:
            pass

    def test_use_portal_none_on_success(self):
        """Walk around and try the portal — success returns None."""
        for direction in [Direction.NORTH, Direction.SOUTH,
                          Direction.EAST, Direction.WEST]:
            try:
                self.eng.move_player(direction)
            except (ImpassableError, GameError):
                pass
            try:
                result = self.eng.use_portal()
                self.assertIsNone(result)
                return
            except GameError:
                pass


# ============================================================
# A3: room metadata
# ============================================================

@_SKIP
class TestRoomMetadata(unittest.TestCase):
    def setUp(self):
        self.eng = GameEngine(_CONFIG)

    def tearDown(self):
        self.eng.destroy()

    def test_room_name_is_str(self):
        self.assertIsInstance(self.eng.get_current_room_name(), str)

    def test_room_name_nonempty(self):
        self.assertGreater(len(self.eng.get_current_room_name()), 0)

    def test_room_id_is_int(self):
        self.assertIsInstance(self.eng.get_current_room_id(), int)

    def test_room_id_nonnegative(self):
        self.assertGreaterEqual(self.eng.get_current_room_id(), 0)

    def test_room_id_in_all_ids(self):
        self.assertIn(self.eng.get_current_room_id(), self.eng.get_room_ids())


# ============================================================
# A3: total treasure count
# ============================================================

@_SKIP
class TestTotalTreasure(unittest.TestCase):
    def setUp(self):
        self.eng = GameEngine(_CONFIG)

    def tearDown(self):
        self.eng.destroy()

    def test_total_is_int(self):
        self.assertIsInstance(self.eng.get_total_treasure_count(), int)

    def test_total_positive(self):
        self.assertGreater(self.eng.get_total_treasure_count(), 0)

    def test_total_ge_room_count(self):
        self.assertGreaterEqual(
            self.eng.get_total_treasure_count(), self.eng.get_room_count())

    def test_total_ge_collected(self):
        self.assertGreaterEqual(
            self.eng.get_total_treasure_count(),
            self.eng.player.get_collected_count())

    def test_total_unchanged_after_reset(self):
        before = self.eng.get_total_treasure_count()
        self.eng.reset()
        self.assertEqual(before, self.eng.get_total_treasure_count())


# ============================================================
# A3: adjacency matrix
# ============================================================

@_SKIP
class TestRoomAdjacency(unittest.TestCase):
    def setUp(self):
        self.eng = GameEngine(_CONFIG)

    def tearDown(self):
        self.eng.destroy()

    def test_returns_two_elements(self):
        self.assertEqual(len(self.eng.get_room_adjacency()), 2)

    def test_matrix_is_list(self):
        matrix, _ = self.eng.get_room_adjacency()
        self.assertIsInstance(matrix, list)

    def test_ids_nonempty(self):
        _, ids = self.eng.get_room_adjacency()
        self.assertGreater(len(ids), 0)

    def test_matrix_is_square(self):
        matrix, ids = self.eng.get_room_adjacency()
        num_rooms = len(ids)
        self.assertEqual(len(matrix), num_rooms)
        for row in matrix:
            self.assertEqual(len(row), num_rooms)

    def test_ids_count_matches_room_count(self):
        _, ids = self.eng.get_room_adjacency()
        self.assertEqual(len(ids), self.eng.get_room_count())

    def test_no_self_loops(self):
        matrix, ids = self.eng.get_room_adjacency()
        for idx in range(len(ids)):
            self.assertEqual(matrix[idx][idx], 0)

    def test_has_edges(self):
        matrix, ids = self.eng.get_room_adjacency()
        num_rooms = len(ids)
        edge_sum = sum(matrix[i][j]
                       for i in range(num_rooms) for j in range(num_rooms))
        self.assertGreater(edge_sum, 0)

    def test_values_binary(self):
        matrix, ids = self.eng.get_room_adjacency()
        for row in matrix:
            for val in row:
                self.assertIn(val, (0, 1))

    def test_current_room_in_ids(self):
        _, ids = self.eng.get_room_adjacency()
        self.assertIn(self.eng.get_current_room_id(), ids)

    def test_stable_across_calls(self):
        matrix_a, ids_a = self.eng.get_room_adjacency()
        matrix_b, ids_b = self.eng.get_room_adjacency()
        self.assertEqual(ids_a, ids_b)
        self.assertEqual(matrix_a, matrix_b)


if __name__ == "__main__":
    unittest.main()
