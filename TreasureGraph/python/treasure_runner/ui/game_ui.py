"""
game_ui.py
----------
MVC View: all curses rendering and keyboard input for Treasure Runner.

Extended features:
  1. Collect all the Treasure -- victory screen + progress bar.
  2. Enhanced UI -- colour tiles + live minimap.

No game logic lives in this module. All state is queried from GameEngine
(controller) or Player (model).

Curses structure partially inspired by CIS*3490 lecture examples
(University of Guelph course notes).
"""

import curses

from ..models.game_engine import GameEngine
from ..models.exceptions import GameError, ImpassableError
from ..models.player_profile import PlayerProfile
from ..bindings import Direction
from .minimap import Minimap, MapCtx

# ============================================================
# Constants
# ============================================================

TITLE    = "Treasure Runner"
EMAIL    = "azaan@uoguelph.ca"

CONTROLS = "Arrows/WASD: move  |  >: portal  |  r: reset  |  q: quit"
LEGEND   = "@=player  #=wall  $=gold  X=portal  o=pushable"

MIN_HEIGHT         = 20
MIN_WIDTH          = 60
SIDE_MAP_MIN_WIDTH = 100

# Colour pair IDs
_CP_WALL     = 1
_CP_PLAYER   = 2
_CP_TREASURE = 3
_CP_PORTAL   = 4
_CP_PUSHABLE = 5
_CP_FLOOR    = 6
_CP_MSG      = 7
_CP_STATUS   = 8
_CP_HEADER   = 12
_CP_VICTORY  = 13

_CHAR_TO_CP = {
    "#": _CP_WALL,
    "@": _CP_PLAYER,
    "$": _CP_TREASURE,
    "X": _CP_PORTAL,
    "x": _CP_PORTAL,
    "o": _CP_PUSHABLE,
    "O": _CP_PUSHABLE,
    ".": _CP_FLOOR,
}


def _init_colours() -> bool:
    """Initialise curses colour pairs. Returns True if colours are supported."""
    if not curses.has_colors():
        return False
    curses.start_color()
    curses.use_default_colors()

    def make_pair(pair_id, foreground, background):
        curses.init_pair(pair_id, foreground, background)

    make_pair(_CP_WALL,     curses.COLOR_BLUE,    -1)
    make_pair(_CP_PLAYER,   curses.COLOR_GREEN,   -1)
    make_pair(_CP_TREASURE, curses.COLOR_YELLOW,  -1)
    make_pair(_CP_PORTAL,   curses.COLOR_CYAN,    -1)
    make_pair(_CP_PUSHABLE, curses.COLOR_MAGENTA, -1)
    make_pair(_CP_FLOOR,    -1,                   -1)
    make_pair(_CP_MSG,      curses.COLOR_WHITE,   curses.COLOR_BLUE)
    make_pair(_CP_STATUS,   curses.COLOR_BLACK,   curses.COLOR_CYAN)
    # Minimap colour pairs (IDs 9-11) -- must match constants in minimap.py
    make_pair(9,            curses.COLOR_BLACK,   curses.COLOR_YELLOW)
    make_pair(10,           curses.COLOR_BLACK,   curses.COLOR_GREEN)
    make_pair(11,           curses.COLOR_WHITE,   curses.COLOR_BLACK)
    make_pair(_CP_HEADER,   curses.COLOR_CYAN,    -1)
    make_pair(_CP_VICTORY,  curses.COLOR_BLACK,   curses.COLOR_YELLOW)
    return True


# ============================================================
# Layout descriptor (CamelCase -- required by MVC inspector)
# ============================================================

class Layout:
    """Value object: screen dimensions computed once per frame."""

    def __init__(self, term_height: int, term_width: int,
                 room_lines: list, has_minimap: bool):
        self.term_height = term_height
        self.term_width  = term_width
        self.room_lines  = room_lines
        self.room_height = len(room_lines)
        self.side_by_side = (term_width >= SIDE_MAP_MIN_WIDTH and has_minimap)
        room_w = max((len(line) for line in room_lines), default=0)
        if self.side_by_side:
            self.map_start_col = room_w + 4
            self.map_width     = term_width - self.map_start_col - 1
        else:
            self.map_start_col = 0
            self.map_width     = term_width - 1


# ============================================================
# GameUI -- the View
# ============================================================

class GameUI:
    """MVC View: curses rendering and keyboard input.

    Extended features:
      - Collect all the Treasure (progress bar + victory screen)
      - Enhanced UI (colour tiles + live minimap)
    """

    _DIRECTION_KEYS = {
        curses.KEY_UP:    Direction.NORTH,
        ord('w'):         Direction.NORTH,
        ord('W'):         Direction.NORTH,
        curses.KEY_DOWN:  Direction.SOUTH,
        ord('s'):         Direction.SOUTH,
        ord('S'):         Direction.SOUTH,
        curses.KEY_RIGHT: Direction.EAST,
        ord('d'):         Direction.EAST,
        ord('D'):         Direction.EAST,
        curses.KEY_LEFT:  Direction.WEST,
        ord('a'):         Direction.WEST,
        ord('A'):         Direction.WEST,
    }

    def __init__(self, engine: GameEngine, profile: PlayerProfile):
        self._engine           = engine
        self._profile          = profile
        self._message          = "Welcome! Use arrow keys or WASD to move."
        self._rooms_visited: set = set()
        self._use_colour       = False
        self._minimap          = None
        self._total_treasure   = 0
        self._victory          = False
        self._stdscr           = None   # set in _main

    # ============================================================
    # Public entry point
    # ============================================================

    def run(self) -> None:
        """Initialise curses and run the full game."""
        curses.wrapper(self._main)

    # ============================================================
    # Top-level curses flow
    # ============================================================

    def _main(self, stdscr) -> None:
        self._stdscr = stdscr
        curses.curs_set(0)
        stdscr.keypad(True)

        self._check_terminal_size(stdscr)
        self._use_colour = _init_colours()

        self._total_treasure = self._engine.get_total_treasure_count()
        matrix, ids = self._engine.get_room_adjacency()
        if ids:
            self._minimap = Minimap(matrix, ids)

        if self._profile.needs_name:
            self._profile.player_name = self._prompt_name(stdscr)
            self._profile.save()

        self._show_splash(stdscr)
        self._game_loop(stdscr)

        treasure = self._engine.player.get_collected_count()
        self._profile.update_after_game(treasure, len(self._rooms_visited))
        self._profile.save()

        self._show_end_screen(stdscr)

    # ============================================================
    # Terminal-size guard
    # ============================================================

    def _check_terminal_size(self, stdscr) -> None:
        term_height, term_width = stdscr.getmaxyx()
        if term_height < MIN_HEIGHT or term_width < MIN_WIDTH:
            curses.endwin()
            raise RuntimeError(
                f"Terminal too small: need {MIN_WIDTH}x{MIN_HEIGHT}, "
                f"got {term_width}x{term_height}. Resize and retry."
            )

    # ============================================================
    # Safe write helpers
    # ============================================================

    def _put(self, win, row: int, col: int, text: str, attr: int = 0) -> None:
        """addstr that silently ignores writes outside the window."""
        term_height, term_width = win.getmaxyx()
        if row < 0 or row >= term_height - 1 or col < 0:
            return
        try:
            win.addstr(row, col, text[: max(0, term_width - col - 1)], attr)
        except curses.error:
            pass

    def _put_full_row(self, win, row: int, text: str, attr: int = 0) -> None:
        """Write *text* padded to fill the full row width."""
        term_height, term_width = win.getmaxyx()
        if row < 0 or row >= term_height - 1:
            return
        try:
            win.addstr(row, 0, text[: term_width - 1].ljust(term_width - 1), attr)
        except curses.error:
            pass

    # ============================================================
    # Generic info-screen helper
    # ============================================================

    def _render_lines(self, stdscr, lines: list,
                      banner_row: int, banner_attr: int) -> None:
        """Print *lines* centred on screen; *banner_row* gets *banner_attr*."""
        term_height, term_width = stdscr.getmaxyx()
        start_row = max(0, (term_height - len(lines)) // 2)
        for idx, line in enumerate(lines):
            self._put(stdscr, start_row + idx, 0, line[:term_width - 1],
                      banner_attr if idx == banner_row else 0)

    # ============================================================
    # Splash / name-prompt
    # ============================================================

    def _build_splash_lines(self) -> list:
        return [
            "",
            "  +================================+",
            "  |       " + TITLE + "          |",
            "  +================================+",
            "",
            "  Player Profile:",
        ] + self._profile.summary_lines() + [
            "",
            "  Press any key to start...",
        ]

    def _show_splash(self, stdscr) -> None:
        """Startup splash screen showing the player profile."""
        stdscr.clear()
        self._render_lines(stdscr, self._build_splash_lines(), 2, curses.A_BOLD)
        stdscr.refresh()
        stdscr.getch()

    def _prompt_name(self, stdscr) -> str:
        """Prompt the player for their name (new profile only)."""
        stdscr.clear()
        self._put(stdscr, 2, 2, "New profile -- please enter your player name:")
        self._put(stdscr, 4, 2, "Name: ")
        curses.echo()
        curses.curs_set(1)
        stdscr.refresh()
        try:
            name_bytes = stdscr.getstr(4, 8, 30)
            name = name_bytes.decode("utf-8").strip()
        except (curses.error, UnicodeDecodeError):
            name = ""
        curses.noecho()
        curses.curs_set(0)
        return name or "Player"

    # ============================================================
    # Game loop
    # ============================================================

    def _game_loop(self, stdscr) -> None:
        self._rooms_visited.add(self._engine.player.get_room())
        while True:
            self._draw_game(stdscr)
            key = stdscr.getch()
            if key == ord('q'):
                break
            self._handle_key(key, stdscr)
            self._rooms_visited.add(self._engine.player.get_room())
            if self._check_victory():
                self._draw_game(stdscr)
                self._show_victory_screen(stdscr)
                break

    def _check_victory(self) -> bool:
        """Return True (once) when all treasure is collected."""
        if self._victory:
            return False
        collected = self._engine.player.get_collected_count()
        if self._total_treasure > 0 and collected >= self._total_treasure:
            self._victory = True
            return True
        return False

    # ============================================================
    # Input handling
    # ============================================================

    def _handle_key(self, key: int, stdscr) -> None:
        direction = self._DIRECTION_KEYS.get(key)
        if direction is not None:
            self._do_move(direction)
        elif key == ord('>'):
            self._do_use_portal()
        elif key == ord('r'):
            self._do_reset()
        else:
            self._check_terminal_size(stdscr)

    def _do_move(self, direction: Direction) -> None:
        try:
            self._engine.move_player(direction)
            collected = self._engine.player.get_collected_count()
            self._message = f"Moved. Treasure: {collected}/{self._total_treasure}"
        except ImpassableError:
            self._message = "Blocked -- you can't go that way."
        except GameError as exc:
            self._message = f"Move error: {exc}"

    def _do_use_portal(self) -> None:
        try:
            self._engine.use_portal()
            self._message = f"Entered portal to room {self._engine.player.get_room()}."
        except GameError:
            self._message = "No open portal nearby (walk adjacent to X first)."

    def _do_reset(self) -> None:
        self._engine.reset()
        self._rooms_visited.clear()
        self._rooms_visited.add(self._engine.player.get_room())
        self._victory = False
        self._message = "Game reset to the beginning."

    # ============================================================
    # Main draw
    # ============================================================

    def _get_room_lines(self) -> list:
        try:
            return self._engine.render_current_room().splitlines()
        except GameError:
            return ["(render error)"]

    def _draw_game(self, stdscr) -> None:
        """Render the full game frame."""
        stdscr.erase()
        term_height, term_width = stdscr.getmaxyx()
        layout = Layout(term_height, term_width, self._get_room_lines(),
                        self._minimap is not None)
        msg_attr    = curses.color_pair(_CP_MSG)    if self._use_colour else curses.A_REVERSE
        status_attr = curses.color_pair(_CP_STATUS) if self._use_colour else curses.A_REVERSE
        header_attr = (curses.color_pair(_CP_HEADER) | curses.A_BOLD
                       if self._use_colour else curses.A_BOLD)
        self._draw_message_bar(stdscr, msg_attr)
        self._draw_room_header(stdscr, layout, header_attr)
        self._draw_room_tiles(stdscr, layout)
        self._draw_minimap_panel(stdscr, layout)
        self._draw_controls_legend(stdscr, layout)
        self._draw_status_bar(stdscr, layout, status_attr)
        self._draw_footer(stdscr, layout)
        stdscr.refresh()

    def _draw_message_bar(self, stdscr, attr: int) -> None:
        self._put_full_row(stdscr, 0, f" {self._message} ", attr)

    def _draw_room_header(self, stdscr, layout: Layout, attr: int) -> None:
        try:
            room_id   = self._engine.get_current_room_id()
            room_name = self._engine.get_current_room_name()
        except GameError:
            room_id   = "?"
            room_name = "unknown"
        total_rooms = self._engine.get_room_count()
        header = f" Room {room_id} / {total_rooms - 1}: {room_name} "
        self._put(stdscr, 1, 0, header[:layout.term_width - 1], attr)

    def _draw_room_tiles(self, stdscr, layout: Layout) -> None:
        for line_idx, line in enumerate(layout.room_lines):
            row = 2 + line_idx
            if row >= layout.term_height - 4:
                break
            if self._use_colour:
                self._draw_coloured_line(stdscr, row, line)
            else:
                self._put(stdscr, row, 0, line[:layout.term_width - 1])

    def _draw_coloured_line(self, stdscr, row: int, line: str) -> None:
        for col_idx, tile_char in enumerate(line):
            pair_id   = _CHAR_TO_CP.get(tile_char, _CP_FLOOR)
            tile_attr = curses.color_pair(pair_id)
            if tile_char == "@":
                tile_attr |= curses.A_BOLD
            try:
                stdscr.addch(row, col_idx, tile_char, tile_attr)
            except curses.error:
                pass

    def _draw_minimap_panel(self, stdscr, layout: Layout) -> None:
        if self._minimap is None:
            return
        ctx = MapCtx(layout.map_start_col, layout.map_width,
                     self._engine.player.get_room(),
                     self._rooms_visited, self._use_colour)
        if layout.side_by_side:
            self._put(stdscr, 2, layout.map_start_col, "-- MINIMAP --")
            self._minimap.render_into(stdscr, 3,
                                      layout.term_height - 7, ctx)
        else:
            map_row = 2 + layout.room_height + 1
            if map_row < layout.term_height - 6:
                self._put(stdscr, map_row, 0, "-- MINIMAP --")
                self._minimap.render_into(stdscr, map_row + 1,
                                          layout.term_height - map_row - 5, ctx)

    def _draw_controls_legend(self, stdscr, layout: Layout) -> None:
        self._put(stdscr, layout.term_height - 4, 0,
                  ("Controls: " + CONTROLS)[:layout.term_width - 1])
        self._put(stdscr, layout.term_height - 3, 0,
                  ("Legend:   " + LEGEND)[:layout.term_width - 1])

    def _draw_status_bar(self, stdscr, layout: Layout, attr: int) -> None:
        collected   = self._engine.player.get_collected_count()
        total_rooms = self._engine.get_room_count()
        status_text = (
            f" {self._profile.player_name}"
            f"  |  Gold: {collected}/{self._total_treasure}"
            f"  |  Rooms: {len(self._rooms_visited)}/{total_rooms} "
        )
        self._put_full_row(stdscr, layout.term_height - 2, status_text, attr)

    def _draw_footer(self, stdscr, layout: Layout) -> None:
        left  = f" {TITLE}"
        right = f"{EMAIL} "
        pad   = layout.term_width - len(left) - len(right) - 1
        self._put(stdscr, layout.term_height - 1, 0,
                  (left + " " * max(0, pad) + right)[:layout.term_width - 1])

    # ============================================================
    # Victory screen
    # ============================================================

    def _build_victory_lines(self) -> list:
        collected = self._engine.player.get_collected_count()
        rooms     = len(self._rooms_visited)
        return [
            "",
            "  *** ALL TREASURE COLLECTED! ***",
            "",
            f"  Treasure: {collected}/{self._total_treasure}",
            f"  Rooms explored: {rooms}",
            "",
            "  Updated Profile:",
        ] + self._profile.summary_lines() + [
            "",
            "  Press any key to continue...",
        ]

    def _show_victory_screen(self, stdscr) -> None:
        """Full-screen victory banner when all treasure is collected."""
        banner_attr = (curses.color_pair(_CP_VICTORY) | curses.A_BOLD
                       if self._use_colour else curses.A_BOLD)
        stdscr.clear()
        self._render_lines(stdscr, self._build_victory_lines(), 1, banner_attr)
        stdscr.refresh()
        stdscr.getch()

    # ============================================================
    # End screen
    # ============================================================

    def _build_end_lines(self) -> list:
        collected = self._engine.player.get_collected_count()
        rooms     = len(self._rooms_visited)
        return [
            "",
            f"  {TITLE} -- Game Over",
            "",
            f"  Treasure collected: {collected}/{self._total_treasure}",
            f"  Rooms visited:      {rooms}",
            "",
            "  Updated Profile:",
        ] + self._profile.summary_lines() + [
            "",
            "  Thanks for playing! Press any key to exit.",
        ]

    def _show_end_screen(self, stdscr) -> None:
        """End screen shown when the player quits."""
        banner_attr = (curses.color_pair(_CP_HEADER) | curses.A_BOLD
                       if self._use_colour else curses.A_BOLD)
        stdscr.clear()
        self._render_lines(stdscr, self._build_end_lines(), 1, banner_attr)
        stdscr.refresh()
        stdscr.getch()
