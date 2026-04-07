"""Python wrapper for the C Player type."""

import ctypes
from ..bindings import lib


class Player:
    def __init__(self, ptr):
        self._ptr = ptr

    def get_room(self) -> int:
        return lib.player_get_room(self._ptr)

    def get_position(self) -> tuple:
        x = ctypes.c_int(0)
        y = ctypes.c_int(0)
        lib.player_get_position(self._ptr, ctypes.byref(x), ctypes.byref(y))
        return (x.value, y.value)

    def get_collected_count(self) -> int:
        return lib.player_get_collected_count(self._ptr)

    def has_collected_treasure(self, treasure_id: int) -> bool:
        return lib.player_has_collected_treasure(self._ptr, treasure_id)

    def get_collected_treasures(self) -> list:
        count = ctypes.c_int(0)
        arr = lib.player_get_collected_treasures(self._ptr, ctypes.byref(count))
        results = []
        for i in range(count.value):
            treasure = arr[i].contents
            results.append({
                "id": treasure.id,
                "name": treasure.name.decode("utf-8") if treasure.name else "",
                "starting_room_id": treasure.starting_room_id,
                "initial_x": treasure.initial_x,
                "initial_y": treasure.initial_y,
                "x": treasure.x,
                "y": treasure.y,
                "collected": treasure.collected,
            })
        return results
        