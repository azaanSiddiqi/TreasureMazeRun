"""
minimap.py
----------
BFS-level ASCII minimap for the Treasure Runner room graph.

This module has NO dependency on the C game engine, so it can be
imported and tested without the shared library being present.
"""

import curses
from collections import namedtuple

# Colour pair IDs used by game_ui._init_colours() for the minimap.
_CP_MAP_CUR = 9   # current room  -- black on yellow
_CP_MAP_VIS = 10  # visited room  -- black on green
_CP_MAP_UNV = 11  # unvisited     -- white on black

# Named tuple grouping the rendering context passed to render_into.
# Using a namedtuple keeps render_into's parameter count low while
# keeping the call site readable.
MapCtx = namedtuple('MapCtx',
                    ['start_col', 'max_cols', 'current_room_id',
                     'visited_ids', 'use_colour'])


class Minimap:
    """Draw the room graph as a BFS-level box diagram inside a curses window.

    Rooms appear as  [NN]  boxes (4 chars wide).
    Horizontal  --  connectors link boxes on the same BFS level.
    Vertical    |   connectors link adjacent levels.

    Colour coding (when colour is enabled):
      - Yellow background  = current room
      - Green background   = visited room
      - Black background   = unvisited room

    Usage::

        matrix, ids = engine.get_room_adjacency()
        mm = Minimap(matrix, ids)
        ctx = MapCtx(start_col, max_cols, current_room_id, visited_ids, use_colour)
        mm.render_into(stdscr, start_row=3, max_rows=10, ctx=ctx)
    """

    BOX_WIDTH = 4
    BOX_GAP   = 2

    def __init__(self, matrix: list, ids: list):
        self._matrix    = matrix
        self._ids       = ids
        self._num_rooms = len(ids)
        self._levels: list = []
        self._build_levels()

    # ----------------------------------------------------------------
    # BFS level assignment
    # ----------------------------------------------------------------

    def _build_levels(self) -> None:
        """BFS from node 0 to assign each room to a display level."""
        if self._num_rooms == 0:
            return
        visited = [False] * self._num_rooms
        queue   = [0]
        visited[0] = True
        while queue:
            current_level = list(queue)
            queue = []
            self._levels.append(current_level)
            for node in current_level:
                for nb_idx in range(self._num_rooms):
                    if self._matrix[node][nb_idx] and not visited[nb_idx]:
                        visited[nb_idx] = True
                        queue.append(nb_idx)
        for idx in range(self._num_rooms):
            if not visited[idx]:
                self._levels.append([idx])

    # ----------------------------------------------------------------
    # Public inspection helpers (used by tests)
    # ----------------------------------------------------------------

    @property
    def level_count(self) -> int:
        """Number of BFS levels in the layout."""
        return len(self._levels)

    def level_size(self, level_idx: int) -> int:
        """Number of room nodes on the given BFS level."""
        if level_idx < 0 or level_idx >= len(self._levels):
            return 0
        return len(self._levels[level_idx])

    def all_node_indices(self) -> list:
        """All matrix-index values across all levels (for testing)."""
        return [node for level in self._levels for node in level]

    def pixel_width_for_level(self, level_idx: int) -> int:
        """Character width of the rendered row for the given level."""
        if level_idx < 0 or level_idx >= len(self._levels):
            return 0
        return self._level_pixel_width(self._levels[level_idx])

    # ----------------------------------------------------------------
    # Geometry helpers
    # ----------------------------------------------------------------

    def _level_pixel_width(self, level_nodes: list) -> int:
        """Total character-width of one rendered level row."""
        node_count = len(level_nodes)
        return node_count * self.BOX_WIDTH + max(0, node_count - 1) * self.BOX_GAP

    def _node_attr(self, room_id: int, ctx: MapCtx) -> int:
        """Return the curses attribute for a room node box."""
        if not ctx.use_colour:
            return curses.A_REVERSE if room_id == ctx.current_room_id else 0
        if room_id == ctx.current_room_id:
            return curses.color_pair(_CP_MAP_CUR) | curses.A_BOLD
        if room_id in ctx.visited_ids:
            return curses.color_pair(_CP_MAP_VIS)
        return curses.color_pair(_CP_MAP_UNV)

    # ----------------------------------------------------------------
    # Safe drawing helpers
    # ----------------------------------------------------------------

    def _draw_box(self, win, row: int, col: int,
                  room_id: int, attr: int, max_col: int) -> None:
        """Draw one [NN] box, silently ignoring out-of-bounds writes."""
        if col + self.BOX_WIDTH <= max_col and row < win.getmaxyx()[0] - 1:
            try:
                win.addstr(row, col, f"[{room_id:2d}]", attr)
            except curses.error:
                pass

    def _draw_h_connector(self, win, row: int,
                          col: int, max_col: int) -> None:
        """Draw a  --  horizontal connector between boxes."""
        if col + self.BOX_GAP <= max_col and row < win.getmaxyx()[0] - 1:
            try:
                win.addstr(row, col, "--")
            except curses.error:
                pass

    def _draw_v_connectors(self, win, row: int, nodes: list,
                           next_nodes: list, base_col: int) -> None:
        """Draw | connectors from *nodes* down toward *next_nodes*."""
        for src_pos in range(len(nodes)):
            node = nodes[src_pos]
            if any(self._matrix[node][dst] for dst in next_nodes):
                if row < win.getmaxyx()[0] - 1:
                    try:
                        win.addstr(
                            row,
                            base_col + src_pos * (self.BOX_WIDTH + self.BOX_GAP) + self.BOX_WIDTH // 2,
                            "|")
                    except curses.error:
                        pass

    # ----------------------------------------------------------------
    # Level rendering helper
    # ----------------------------------------------------------------

    def _render_level_boxes(self, win, row: int, nodes: list,
                            base_col: int, ctx: MapCtx) -> None:
        """Draw all boxes and horizontal connectors for one level."""
        for pos, node_idx in enumerate(nodes):
            room_id = self._ids[node_idx]
            attr    = self._node_attr(room_id, ctx)
            box_col = base_col + pos * (self.BOX_WIDTH + self.BOX_GAP)
            self._draw_box(win, row, box_col, room_id, attr, ctx.start_col + ctx.max_cols)
            if pos < len(nodes) - 1:
                self._draw_h_connector(win, row, box_col + self.BOX_WIDTH,
                                       ctx.start_col + ctx.max_cols)

    # ----------------------------------------------------------------
    # Public render method
    # ----------------------------------------------------------------

    def render_into(self, win, start_row: int, max_rows: int,
                    ctx: MapCtx) -> None:
        """Draw the minimap inside *win*.

        Args:
            win:       curses window to draw into.
            start_row: first row to use (0-indexed).
            max_rows:  maximum number of rows to use.
            ctx:       MapCtx with position/colour/room information.
        """
        row = start_row
        for level_idx in range(len(self._levels)):
            if row >= start_row + max_rows - 1:
                break
            nodes = self._levels[level_idx]
            base_col = ctx.start_col + max(0, (ctx.max_cols - self._level_pixel_width(nodes)) // 2)
            self._render_level_boxes(win, row, nodes, base_col, ctx)
            row += 1
            if level_idx + 1 < len(self._levels) and row < start_row + max_rows - 1:
                self._draw_v_connectors(win, row, nodes, self._levels[level_idx + 1], base_col)
                row += 1
