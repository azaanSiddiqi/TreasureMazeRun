/*
 * game_engine_extra.c
 * --------------------
 * Implements the additional GameEngine API declared in game_engine_extra.h.
 * Added for A3 -- no existing .h files are modified.
 */

#include "game_engine_extra.h"
#include "room.h"
#include "player.h"
#include "graph.h"

#include <stdlib.h>
#include <string.h>

/* ============================================================
 * File-local helpers
 * ============================================================ */

static Room *ge_find_room(const GameEngine *eng, int room_id)
{
    Room key;
    if (eng == NULL || eng->graph == NULL) return NULL;
    memset(&key, 0, sizeof(key));
    key.id = room_id;
    return (Room *)graph_get_payload(eng->graph, &key);
}

/* Returns true when the portal at (x,y) is open. */
static bool ge_portal_open(const Room *room, int x, int y)
{
    int i = 0;
    int j = 0;
    if (room == NULL) return false;
    for (i = 0; i < room->portal_count; i++) {
        if (room->portals[i].x != x || room->portals[i].y != y) continue;
        if (!room->portals[i].gated) return true;
        for (j = 0; j < room->switch_count; j++) {
            if (room->switches[j].id == room->portals[i].required_switch_id) {
                return room_has_pushable_at(room,
                           room->switches[j].x, room->switches[j].y, NULL);
            }
        }
        return false;
    }
    return false;
}

/* Move the player into target_room at its entry position. */
static Status ge_enter_room(GameEngine *eng, int target_room_id)
{
    int entry_x = 1;
    int entry_y = 1;
    Status s = OK;
    Room *target = ge_find_room(eng, target_room_id);
    if (target == NULL) return GE_NO_SUCH_ROOM;

    s = player_move_to_room(eng->player, target_room_id);
    if (s != OK) return s;

    if (room_get_start_position(target, &entry_x, &entry_y) != OK) {
        entry_x = 1;
        entry_y = 1;
    }
    player_set_position(eng->player, entry_x, entry_y);
    return OK;
}

/* ============================================================
 * Public API
 * ============================================================ */

Status game_engine_use_portal(GameEngine *eng)
{
    static const int DX[4] = { 0,  0, -1, 1 };
    static const int DY[4] = {-1,  1,  0, 0 };

    int px = 0;
    int py = 0;
    int i = 0;
    Room *room = NULL;
    Status s = OK;

    if (eng == NULL || eng->player == NULL) return INVALID_ARGUMENT;

    s = player_get_position(eng->player, &px, &py);
    if (s != OK) return s;

    room = ge_find_room(eng, player_get_room(eng->player));
    if (room == NULL) return GE_NO_SUCH_ROOM;

    for (i = 0; i < 4; i++) {
        int tile_id = 0;
        int nx = px + DX[i];
        int ny = py + DY[i];
        RoomTileType tile_type = room_classify_tile(room, nx, ny, &tile_id);
        if (tile_type == ROOM_TILE_PORTAL && ge_portal_open(room, nx, ny)) {
            return ge_enter_room(eng, tile_id);
        }
    }
    return ROOM_NO_PORTAL;
}

Status game_engine_get_current_room_name(const GameEngine *eng, char **name_out)
{
    const char *src = NULL;
    size_t len = 0;
    char *copy = NULL;
    Room *room = NULL;

    if (eng == NULL || name_out == NULL) return INVALID_ARGUMENT;
    if (eng->player == NULL) return INTERNAL_ERROR;

    room = ge_find_room(eng, player_get_room(eng->player));
    if (room == NULL) return GE_NO_SUCH_ROOM;

    src  = (room->name != NULL) ? room->name : "(unnamed)";
    len  = strlen(src);
    copy = malloc(len + 1);
    if (copy == NULL) return NO_MEMORY;
    memcpy(copy, src, len + 1);
    *name_out = copy;
    return OK;
}

int game_engine_get_current_room_id(const GameEngine *eng)
{
    if (eng == NULL || eng->player == NULL) return -1;
    return player_get_room(eng->player);
}

Status game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out)
{
    const void * const *rooms = NULL;
    int n = 0;
    int total = 0;
    int i = 0;
    GraphStatus gs = GRAPH_STATUS_OK;

    if (eng == NULL || count_out == NULL) return INVALID_ARGUMENT;
    if (eng->graph == NULL) return INTERNAL_ERROR;

    gs = graph_get_all_payloads(eng->graph, &rooms, &n);
    if (gs != GRAPH_STATUS_OK) return INTERNAL_ERROR;

    for (i = 0; i < n; i++) {
        const Room *r = (const Room *)rooms[i];
        if (r != NULL) total += r->treasure_count;
    }
    *count_out = total;
    return OK;
}

Status game_engine_get_room_adjacency(const GameEngine *eng,
                                       int **matrix_out,
                                       int **ids_out,
                                       int  *count_out)
{
    const void * const *rooms = NULL;
    int n = 0;
    int *ids = NULL;
    int *matrix = NULL;
    int i = 0;
    int k = 0;
    int j = 0;
    GraphStatus gs = GRAPH_STATUS_OK;

    if (eng == NULL || matrix_out == NULL ||
        ids_out == NULL || count_out == NULL) {
        return INVALID_ARGUMENT;
    }
    if (eng->graph == NULL) return INTERNAL_ERROR;

    gs = graph_get_all_payloads(eng->graph, &rooms, &n);
    if (gs != GRAPH_STATUS_OK) return INTERNAL_ERROR;

    ids    = malloc(sizeof(int) * (size_t)n);
    matrix = calloc((size_t)n * (size_t)n, sizeof(int));
    if (ids == NULL || matrix == NULL) {
        free(ids);
        free(matrix);
        return NO_MEMORY;
    }

    for (i = 0; i < n; i++) {
        ids[i] = room_get_id((const Room *)rooms[i]);
    }

    for (i = 0; i < n; i++) {
        const void * const *neighbours = NULL;
        int nc = 0;
        gs = graph_get_neighbours(eng->graph, rooms[i], &neighbours, &nc);
        if (gs != GRAPH_STATUS_OK) continue;
        for (k = 0; k < nc; k++) {
            int nid = room_get_id((const Room *)neighbours[k]);
            for (j = 0; j < n; j++) {
                if (ids[j] == nid) {
                    matrix[i * n + j] = 1;
                    break;
                }
            }
        }
    }

    *ids_out    = ids;
    *matrix_out = matrix;
    *count_out  = n;
    return OK;
}
