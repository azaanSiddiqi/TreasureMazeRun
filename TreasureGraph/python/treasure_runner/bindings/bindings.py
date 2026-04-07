"""
bindings.py
-----------
ctypes bindings for libbackend.so (C game engine).

A2 bindings: original set of functions.
A3 additions: game_engine_extra.h functions for portal traversal,
              room metadata, total treasure count, and adjacency matrix.
"""

import ctypes
import os
from enum import IntEnum
from pathlib import Path


class Direction(IntEnum):
    NORTH = 0
    SOUTH = 1
    EAST = 2
    WEST = 3


class Status(IntEnum):
    OK = 0
    INVALID_ARGUMENT = 1
    NULL_POINTER = 2
    NO_MEMORY = 3
    BOUNDS_EXCEEDED = 4
    INTERNAL_ERROR = 5
    ROOM_IMPASSABLE = 6
    ROOM_NO_PORTAL = 7
    ROOM_NOT_FOUND = 8
    GE_NO_SUCH_ROOM = 9
    WL_ERR_CONFIG = 10
    WL_ERR_DATAGEN = 11


GameEngineStatus = Status


class Treasure(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_int),
        ("name", ctypes.c_char_p),
        ("starting_room_id", ctypes.c_int),
        ("initial_x", ctypes.c_int),
        ("initial_y", ctypes.c_int),
        ("x", ctypes.c_int),
        ("y", ctypes.c_int),
        ("collected", ctypes.c_bool),
    ]


def _find_library():
    here = Path(__file__).resolve()
    repo_root = here.parent.parent.parent.parent

    search_dirs = []
    env_path = os.getenv("TREASURE_RUNNER_DIST")
    if env_path:
        search_dirs.append(Path(env_path))
    search_dirs.append(repo_root / "dist")
    search_dirs.append(repo_root / "c" / "lib")

    # Pre-load libpuzzlegen (any variant: .so, -linux-amd64.so, etc.)
    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
        matches = sorted(search_dir.glob("libpuzzlegen*.so*"))
        if matches:
            puzzlegen_path = str(matches[0])
            ctypes.CDLL(puzzlegen_path)
            break

    # Find and return libbackend.so
    for search_dir in search_dirs:
        backend = search_dir / "libbackend.so"
        if backend.exists():
            return str(backend)

    tried = "\n".join(str(d) for d in search_dirs)
    raise RuntimeError(f"libbackend.so not found. Dirs tried:\n{tried}")


_LIB_PATH = _find_library()
lib = ctypes.CDLL(_LIB_PATH)

GameEngine = ctypes.c_void_p

# ============================================================
# A2 bindings – game engine core
# ============================================================

lib.game_engine_create.argtypes = [ctypes.c_char_p, ctypes.POINTER(GameEngine)]
lib.game_engine_create.restype = ctypes.c_int

lib.game_engine_destroy.argtypes = [GameEngine]
lib.game_engine_destroy.restype = None

lib.game_engine_get_player.argtypes = [GameEngine]
lib.game_engine_get_player.restype = ctypes.c_void_p

lib.game_engine_move_player.argtypes = [GameEngine, ctypes.c_int]
lib.game_engine_move_player.restype = ctypes.c_int

lib.game_engine_render_current_room.argtypes = [GameEngine,
                                                ctypes.POINTER(ctypes.c_char_p)]
lib.game_engine_render_current_room.restype = ctypes.c_int

lib.game_engine_get_room_count.argtypes = [GameEngine, ctypes.POINTER(ctypes.c_int)]
lib.game_engine_get_room_count.restype = ctypes.c_int

lib.game_engine_get_room_dimensions.argtypes = [GameEngine,
                                                ctypes.POINTER(ctypes.c_int),
                                                ctypes.POINTER(ctypes.c_int)]
lib.game_engine_get_room_dimensions.restype = ctypes.c_int

lib.game_engine_get_room_ids.argtypes = [GameEngine,
                                         ctypes.POINTER(ctypes.POINTER(ctypes.c_int)),
                                         ctypes.POINTER(ctypes.c_int)]
lib.game_engine_get_room_ids.restype = ctypes.c_int

lib.game_engine_reset.argtypes = [GameEngine]
lib.game_engine_reset.restype = ctypes.c_int

lib.game_engine_free_string.argtypes = [ctypes.c_void_p]
lib.game_engine_free_string.restype = None

Player = ctypes.c_void_p

lib.player_get_room.argtypes = [Player]
lib.player_get_room.restype = ctypes.c_int

lib.player_get_position.argtypes = [Player,
                                    ctypes.POINTER(ctypes.c_int),
                                    ctypes.POINTER(ctypes.c_int)]
lib.player_get_position.restype = ctypes.c_int

lib.player_get_collected_count.argtypes = [Player]
lib.player_get_collected_count.restype = ctypes.c_int

lib.player_has_collected_treasure.argtypes = [Player, ctypes.c_int]
lib.player_has_collected_treasure.restype = ctypes.c_bool

lib.player_get_collected_treasures.argtypes = [Player, ctypes.POINTER(ctypes.c_int)]
lib.player_get_collected_treasures.restype = ctypes.POINTER(ctypes.POINTER(Treasure))

lib.player_reset_to_start.argtypes = [Player, ctypes.c_int,
                                      ctypes.c_int, ctypes.c_int]
lib.player_reset_to_start.restype = ctypes.c_int

Room = ctypes.c_void_p

lib.destroy_treasure.argtypes = [ctypes.POINTER(Treasure)]
lib.destroy_treasure.restype = None

# ============================================================
# A3 extra bindings – game_engine_extra.h
# ============================================================

# Portal traversal ('>' key)
lib.game_engine_use_portal.argtypes = [GameEngine]
lib.game_engine_use_portal.restype = ctypes.c_int

# Room metadata accessors
lib.game_engine_get_current_room_name.argtypes = [
    GameEngine, ctypes.POINTER(ctypes.c_char_p)
]
lib.game_engine_get_current_room_name.restype = ctypes.c_int

lib.game_engine_get_current_room_id.argtypes = [GameEngine]
lib.game_engine_get_current_room_id.restype = ctypes.c_int

# Extended feature: Collect all the Treasure
lib.game_engine_get_total_treasure_count.argtypes = [
    GameEngine, ctypes.POINTER(ctypes.c_int)
]
lib.game_engine_get_total_treasure_count.restype = ctypes.c_int

# Extended feature: Enhanced UI / minimap (adjacency matrix)
lib.game_engine_get_room_adjacency.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.POINTER(ctypes.c_int)),   # matrix_out
    ctypes.POINTER(ctypes.POINTER(ctypes.c_int)),   # ids_out
    ctypes.POINTER(ctypes.c_int),                   # count_out
]
lib.game_engine_get_room_adjacency.restype = ctypes.c_int
