#include "room.h"
#include <stdlib.h>
#include <string.h>

Room *room_create(int id, const char *name, int width, int height)
{
    Room *r = malloc(sizeof(Room));
    if (r == NULL) return NULL;

    r->id     = id;
    r->width  = (width  < 1) ? 1 : width;
    r->height = (height < 1) ? 1 : height;

    if (name != NULL) {
        r->name = malloc(strlen(name) + 1);
        if (r->name == NULL) { free(r); return NULL; }
        strcpy(r->name, name);
    } else {
        r->name = NULL;
    }

    r->floor_grid     = NULL;
    r->neighbors      = NULL;
    r->neighbor_count = 0;
    r->portals        = NULL;
    r->portal_count   = 0;
    r->treasures      = NULL;
    r->treasure_count = 0;
    r->pushables      = NULL;
    r->pushable_count = 0;
    r->switches       = NULL;
    r->switch_count   = 0;

    return r;
}

int room_get_id(const Room *r)
{
    if (r == NULL) return -1;
    return r->id;
}

int room_get_width(const Room *r)
{
    if (r == NULL) return 0;
    return r->width;
}

int room_get_height(const Room *r)
{
    if (r == NULL) return 0;
    return r->height;
}

Status room_set_floor_grid(Room *r, bool *floor_grid)
{
    if (r == NULL) return INVALID_ARGUMENT;
    if (r->floor_grid != NULL) free(r->floor_grid);
    r->floor_grid = floor_grid;
    return OK;
}

Status room_set_portals(Room *r, Portal *portals, int portal_count)
{
    if (r == NULL) return INVALID_ARGUMENT;
    if (portal_count > 0 && portals == NULL) return INVALID_ARGUMENT;

    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) free(r->portals[i].name);
        free(r->portals);
    }

    r->portals      = portals;
    r->portal_count = portal_count;
    return OK;
}

Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count)
{
    if (r == NULL) return INVALID_ARGUMENT;
    if (treasure_count > 0 && treasures == NULL) return INVALID_ARGUMENT;

    if (r->treasures != NULL) {
        for (int i = 0; i < r->treasure_count; i++) free(r->treasures[i].name);
        free(r->treasures);
    }

    r->treasures      = treasures;
    r->treasure_count = treasure_count;
    return OK;
}

Status room_place_treasure(Room *r, const Treasure *treasure)
{
    if (r == NULL || treasure == NULL) return INVALID_ARGUMENT;

    Treasure *new_arr = realloc(r->treasures,
                                sizeof(Treasure) * (size_t)(r->treasure_count + 1));
    if (new_arr == NULL) return NO_MEMORY;
    r->treasures = new_arr;

    Treasure *dest         = &r->treasures[r->treasure_count];
    dest->id               = treasure->id;
    dest->starting_room_id = treasure->starting_room_id;
    dest->initial_x        = treasure->initial_x;
    dest->initial_y        = treasure->initial_y;
    dest->x                = treasure->x;
    dest->y                = treasure->y;
    dest->collected        = treasure->collected;

    if (treasure->name != NULL) {
        dest->name = malloc(strlen(treasure->name) + 1);
        if (dest->name == NULL) return NO_MEMORY;
        strcpy(dest->name, treasure->name);
    } else {
        dest->name = NULL;
    }

    r->treasure_count++;
    return OK;
}

int room_get_treasure_at(const Room *r, int x, int y)
{
    if (r == NULL) return -1;
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].x == x && r->treasures[i].y == y &&
            !r->treasures[i].collected) {
            return r->treasures[i].id;
        }
    }
    return -1;
}

int room_get_portal_destination(const Room *r, int x, int y)
{
    if (r == NULL) return -1;
    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {
            return r->portals[i].target_room_id;
        }
    }
    return -1;
}

bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out)
{
    if (r == NULL) return false;
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            if (pushable_idx_out != NULL) *pushable_idx_out = i;
            return true;
        }
    }
    return false;
}

bool room_is_walkable(const Room *r, int x, int y)
{
    if (r == NULL) return false;
    if (x < 0 || x >= r->width || y < 0 || y >= r->height) return false;
    if (room_has_pushable_at(r, x, y, NULL)) return false;

    if (r->floor_grid == NULL) {
        if (x == 0 || x == r->width - 1 || y == 0 || y == r->height - 1) {
            return false;
        }
        return true;
    }

    return r->floor_grid[y * r->width + x];
}

Status room_try_push(Room *r, int pushable_idx, Direction dir)
{
    if (r == NULL) return INVALID_ARGUMENT;
    if (pushable_idx < 0 || pushable_idx >= r->pushable_count) {
        return INVALID_ARGUMENT;
    }

    Pushable *p = &r->pushables[pushable_idx];
    int nx = p->x;
    int ny = p->y;

    switch (dir) {
        case DIR_NORTH: ny--; break;
        case DIR_SOUTH: ny++; break;
        case DIR_EAST:  nx++; break;
        case DIR_WEST:  nx--; break;
        default: return INVALID_ARGUMENT;
    }

    if (nx < 0 || nx >= r->width || ny < 0 || ny >= r->height) {
        return ROOM_IMPASSABLE;
    }

    /* Pushables cannot be pushed into portals */
    if (room_get_portal_destination(r, nx, ny) != -1) {
        return ROOM_IMPASSABLE;
    }

    int orig_x = p->x;
    int orig_y = p->y;
    p->x = -1;
    p->y = -1;

    bool can_push = room_is_walkable(r, nx, ny);

    p->x = orig_x;
    p->y = orig_y;

    if (!can_push) return ROOM_IMPASSABLE;

    p->x = nx;
    p->y = ny;
    return OK;
}

RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id)
{
    if (r == NULL) return ROOM_TILE_INVALID;
    if (x < 0 || x >= r->width || y < 0 || y >= r->height) {
        return ROOM_TILE_INVALID;
    }

    int pidx = -1;
    if (room_has_pushable_at(r, x, y, &pidx)) {
        if (out_id != NULL) *out_id = pidx;
        return ROOM_TILE_PUSHABLE;
    }

    int tid = room_get_treasure_at(r, x, y);
    if (tid != -1) {
        if (out_id != NULL) *out_id = tid;
        return ROOM_TILE_TREASURE;
    }

    int portal_dest = room_get_portal_destination(r, x, y);
    if (portal_dest != -1) {
        if (out_id != NULL) *out_id = portal_dest;
        return ROOM_TILE_PORTAL;
    }

    if (r->floor_grid == NULL) {
        if (x == 0 || x == r->width - 1 || y == 0 || y == r->height - 1) {
            return ROOM_TILE_WALL;
        }
        return ROOM_TILE_FLOOR;
    }

    if (r->floor_grid[y * r->width + x]) return ROOM_TILE_FLOOR;
    return ROOM_TILE_WALL;
}

static bool switch_is_pressed(const Room *r, int switch_idx)
{
    if (r == NULL || switch_idx < 0 || switch_idx >= r->switch_count) {
        return false;
    }
    return room_has_pushable_at(r, r->switches[switch_idx].x,
                                   r->switches[switch_idx].y, NULL);
}

static void render_base_grid(const Room *r, const Charset *charset,
                              char *buffer, int width, int height)
{
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int idx = row * width + col;
            bool is_wall = false;
            if (r->floor_grid == NULL) {
                is_wall = (col == 0 || col == width - 1 ||
                           row == 0 || row == height - 1);
            } else {
                is_wall = !r->floor_grid[idx];
            }
            if (is_wall) {
                buffer[idx] = (char)charset->wall;
            } else {
                buffer[idx] = (char)charset->floor;
            }
        }
    }
}

static void render_switches(const Room *r, const Charset *charset, char *buffer)
{
    for (int i = 0; i < r->switch_count; i++) {
        int sx = r->switches[i].x;
        int sy = r->switches[i].y;
        if (sx >= 0 && sx < r->width && sy >= 0 && sy < r->height) {
            if (switch_is_pressed(r, i)) {
                buffer[sy * r->width + sx] = (char)charset->switch_on;
            } else {
                buffer[sy * r->width + sx] = (char)charset->switch_off;
            }
        }
    }
}

static void render_treasures(const Room *r, const Charset *charset, char *buffer)
{
    for (int i = 0; i < r->treasure_count; i++) {
        if (!r->treasures[i].collected) {
            int tx = r->treasures[i].x;
            int ty = r->treasures[i].y;
            if (tx >= 0 && tx < r->width && ty >= 0 && ty < r->height) {
                buffer[ty * r->width + tx] = (char)charset->treasure;
            }
        }
    }
}

static void render_portals(const Room *r, const Charset *charset, char *buffer)
{
    for (int i = 0; i < r->portal_count; i++) {
        int px = r->portals[i].x;
        int py = r->portals[i].y;
        if (px >= 0 && px < r->width && py >= 0 && py < r->height) {
            buffer[py * r->width + px] = (char)charset->portal;
        }
    }
}

static void render_pushables(const Room *r, const Charset *charset, char *buffer)
{
    for (int i = 0; i < r->pushable_count; i++) {
        int ox = r->pushables[i].x;
        int oy = r->pushables[i].y;
        if (ox >= 0 && ox < r->width && oy >= 0 && oy < r->height) {
            buffer[oy * r->width + ox] = (char)charset->pushable;
        }
    }
}

Status room_render(const Room *r,
                   const Charset *charset,
                   char *buffer,
                   int buffer_width,
                   int buffer_height)
{
    if (r == NULL || charset == NULL || buffer == NULL) return INVALID_ARGUMENT;
    if (buffer_width != r->width || buffer_height != r->height) {
        return INVALID_ARGUMENT;
    }
    render_base_grid(r, charset, buffer, r->width, r->height);
    render_switches(r, charset, buffer);
    render_treasures(r, charset, buffer);
    render_portals(r, charset, buffer);
    render_pushables(r, charset, buffer);
    return OK;
}

Status room_get_start_position(const Room *r, int *x_out, int *y_out)
{
    if (r == NULL || x_out == NULL || y_out == NULL) return INVALID_ARGUMENT;

    if (r->portal_count > 0) {
        *x_out = r->portals[0].x;
        *y_out = r->portals[0].y;
        return OK;
    }

    for (int row = 1; row < r->height - 1; row++) {
        for (int col = 1; col < r->width - 1; col++) {
            if (r->floor_grid == NULL) {
                *x_out = col;
                *y_out = row;
                return OK;
            }
            if (r->floor_grid[row * r->width + col]) {
                *x_out = col;
                *y_out = row;
                return OK;
            }
        }
    }
    return ROOM_NOT_FOUND;
}

void room_destroy(Room *r)
{
    if (r == NULL) return;

    free(r->name);
    free(r->floor_grid);
    free(r->neighbors);

    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) free(r->portals[i].name);
        free(r->portals);
    }

    if (r->treasures != NULL) {
        for (int i = 0; i < r->treasure_count; i++) free(r->treasures[i].name);
        free(r->treasures);
    }

    if (r->pushables != NULL) {
        for (int i = 0; i < r->pushable_count; i++) free(r->pushables[i].name);
        free(r->pushables);
    }

    free(r->switches);
    free(r);
}

Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out)
{
    if (r == NULL || treasure_out == NULL) return INVALID_ARGUMENT;
    if (treasure_id < 0) return INVALID_ARGUMENT;

    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].id == treasure_id) {
            if (r->treasures[i].collected) return INVALID_ARGUMENT;
            r->treasures[i].collected = true;
            *treasure_out = &r->treasures[i];
            return OK;
        }
    }
    return ROOM_NOT_FOUND;
}

void destroy_treasure(Treasure *t)
{
    if (t == NULL) return;
    free(t->name);
    free(t);
}