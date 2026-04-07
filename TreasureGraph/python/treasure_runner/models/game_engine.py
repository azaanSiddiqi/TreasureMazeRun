"""
game_engine.py
--------------
MVC Controller: wraps the C GameEngine via ctypes.

Responsibilities:
  - Create / destroy the C engine.
  - Expose clean Python methods for every game action.
  - Map C status codes to Python exceptions via exceptions.py.
  - Never perform rendering or contain curses logic.
"""

import ctypes
from ..bindings import lib, Direction, Status
from .player import Player
from .exceptions import status_to_exception


class GameEngine:
    """Controller: coordinates the Player model with the C game engine."""

    def __init__(self, config_path: str):
        engine_ptr = ctypes.c_void_p(None)
        status = Status(lib.game_engine_create(
            config_path.encode("utf-8"), ctypes.byref(engine_ptr)))
        if status != Status.OK:
            raise status_to_exception(
                status, f"game_engine_create failed: {status}")
        self._eng = engine_ptr.value
        self._destroyed = False

        player_ptr = lib.game_engine_get_player(self._eng)
        if player_ptr is None:
            raise RuntimeError("game_engine_get_player returned NULL")
        self._player = Player(player_ptr)

    @property
    def player(self) -> Player:
        """The Player model owned by this engine."""
        return self._player

    def destroy(self) -> None:
        """Release all C-side resources. Safe to call multiple times."""
        if not self._destroyed and self._eng is not None:
            lib.game_engine_destroy(self._eng)
            self._eng = None
            self._destroyed = True

    # ------------------------------------------------------------------
    # A2 core methods
    # ------------------------------------------------------------------

    def move_player(self, direction: Direction) -> None:
        """Attempt to move the player one tile in *direction*.

        Raises:
            ImpassableError  -- wall, locked portal, or pushable blocked.
            NoPortalError    -- portal is closed.
            GameEngineError  -- any other C-side failure.
        """
        status = Status(lib.game_engine_move_player(self._eng, int(direction)))
        if status != Status.OK:
            raise status_to_exception(
                status, f"move_player failed: {status}")

    def render_current_room(self) -> str:
        """Return the current room rendered as a multi-line ASCII string."""
        buf = ctypes.c_char_p(None)
        status = Status(lib.game_engine_render_current_room(
            self._eng, ctypes.byref(buf)))
        if status != Status.OK:
            raise status_to_exception(
                status, f"render_current_room failed: {status}")
        raw = buf.value
        result = raw.decode("utf-8") if raw is not None else ""
        lib.game_engine_free_string(buf)
        return result

    def get_room_count(self) -> int:
        """Return the total number of rooms in the world."""
        count = ctypes.c_int(0)
        status = Status(lib.game_engine_get_room_count(
            self._eng, ctypes.byref(count)))
        if status != Status.OK:
            raise status_to_exception(
                status, f"get_room_count failed: {status}")
        return count.value

    def get_room_dimensions(self) -> tuple:
        """Return (width, height) of the current room."""
        width = ctypes.c_int(0)
        height = ctypes.c_int(0)
        lib.game_engine_get_room_dimensions(
            self._eng, ctypes.byref(width), ctypes.byref(height))
        return (width.value, height.value)

    def get_room_ids(self) -> list:
        """Return a list of all room IDs in the world."""
        ids_ptr = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int(0)
        status = Status(lib.game_engine_get_room_ids(
            self._eng, ctypes.byref(ids_ptr), ctypes.byref(count)))
        if status != Status.OK:
            raise status_to_exception(
                status, f"get_room_ids failed: {status}")
        result = [ids_ptr[i] for i in range(count.value)]
        lib.game_engine_free_string(ids_ptr)
        return result

    def reset(self) -> None:
        """Reset the entire world to its initial state."""
        lib.game_engine_reset(self._eng)

    # ------------------------------------------------------------------
    # A3: portal traversal
    # ------------------------------------------------------------------

    def use_portal(self) -> None:
        """Enter an adjacent open portal (the '>' key action).

        Raises:
            NoPortalError   -- no open portal is adjacent.
            GameEngineError -- any other C-side failure.
        """
        status = Status(lib.game_engine_use_portal(self._eng))
        if status != Status.OK:
            raise status_to_exception(
                status, f"use_portal failed: {status}")

    # ------------------------------------------------------------------
    # A3: room metadata
    # ------------------------------------------------------------------

    def get_current_room_name(self) -> str:
        """Return the name of the room the player is currently in."""
        buf = ctypes.c_char_p(None)
        status = Status(lib.game_engine_get_current_room_name(
            self._eng, ctypes.byref(buf)))
        if status != Status.OK:
            return "(unknown)"
        raw_bytes = buf.value
        name = raw_bytes.decode("utf-8") if raw_bytes is not None else ""
        lib.game_engine_free_string(buf)
        return name

    def get_current_room_id(self) -> int:
        """Return the ID of the room the player is currently in."""
        return lib.game_engine_get_current_room_id(self._eng)

    # ------------------------------------------------------------------
    # A3 extended: Collect all the Treasure
    # ------------------------------------------------------------------

    def get_total_treasure_count(self) -> int:
        """Return the total number of treasures across all rooms."""
        total = ctypes.c_int(0)
        status = Status(lib.game_engine_get_total_treasure_count(
            self._eng, ctypes.byref(total)))
        if status != Status.OK:
            return 0
        return total.value

    # ------------------------------------------------------------------
    # A3 extended: Enhanced UI (minimap adjacency matrix)
    # ------------------------------------------------------------------

    def get_room_adjacency(self) -> tuple:
        """Return the room graph as an adjacency matrix.

        Returns:
            (matrix, ids): matrix is a list-of-lists (n x n) where
            matrix[i][j]==1 means edge from ids[i] to ids[j].
            ids is the ordered list of room IDs.
        """
        matrix_ptr = ctypes.POINTER(ctypes.c_int)()
        ids_ptr    = ctypes.POINTER(ctypes.c_int)()
        count      = ctypes.c_int(0)

        status = Status(lib.game_engine_get_room_adjacency(
            self._eng,
            ctypes.byref(matrix_ptr),
            ctypes.byref(ids_ptr),
            ctypes.byref(count),
        ))
        if status != Status.OK:
            return [], []

        room_count = count.value
        ids = [ids_ptr[idx] for idx in range(room_count)]
        matrix = [
            [matrix_ptr[row * room_count + col] for col in range(room_count)]
            for row in range(room_count)
        ]

        lib.game_engine_free_string(matrix_ptr)
        lib.game_engine_free_string(ids_ptr)

        return matrix, ids
