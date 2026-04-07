#include "game_engine.h"
#include "world_loader.h"
#include "graph.h"
#include "room.h"
#include "player.h"
#include <stdlib.h>
#include <string.h>

static Room *find_room_by_id(const GameEngine *eng, int room_id)
{
    if (eng == NULL || eng->graph == NULL) return NULL;
    Room key;
    key.id = room_id;
    return (Room *)graph_get_payload(eng->graph, &key);
}

static bool portal_is_open(const Room *room, int x, int y)
{
    for (int i = 0; i < room->portal_count; i++) {
        if (room->portals[i].x != x || room->portals[i].y != y) continue;
        if (!room->portals[i].gated) return true;
        int sw_id = room->portals[i].required_switch_id;
        for (int j = 0; j < room->switch_count; j++) {
            if (room->switches[j].id == sw_id) {
                return room_has_pushable_at(room,
                    room->switches[j].x, room->switches[j].y, NULL);
            }
        }
        return false;
    }
    return true;
}

static Status handle_portal_transition(GameEngine *eng, int target_room_id)
{
    Room *target_room = find_room_by_id(eng, target_room_id);
    if (target_room == NULL) return GE_NO_SUCH_ROOM;

    Status status = player_move_to_room(eng->player, target_room_id);
    if (status != OK) return status;

    int entry_x = 1;
    int entry_y = 1;
    status = room_get_start_position(target_room, &entry_x, &entry_y);
    if (status == OK) {
        player_set_position(eng->player, entry_x, entry_y);
    }
    return OK;
}

Status game_engine_create(const char *config_file_path, GameEngine **engine_out)
{
    if (config_file_path == NULL || engine_out == NULL) return INVALID_ARGUMENT;

    GameEngine *eng = malloc(sizeof(GameEngine));
    if (eng == NULL) return NO_MEMORY;

    eng->graph            = NULL;
    eng->player           = NULL;
    eng->room_count       = 0;
    eng->initial_room_id  = 0;
    eng->initial_player_x = 0;
    eng->initial_player_y = 0;

    Graph *graph      = NULL;
    Room  *first_room = NULL;
    int    num_rooms  = 0;
    Charset charset;

    Status status = loader_load_world(config_file_path, &graph, &first_room,
                                      &num_rooms, &charset);
    if (status != OK) {
        free(eng);
        return status;
    }

    eng->graph      = graph;
    eng->charset    = charset;
    eng->room_count = num_rooms;

    int start_x = 1;
    int start_y = 1;
    if (first_room != NULL) {
        eng->initial_room_id = first_room->id;
        status = room_get_start_position(first_room, &start_x, &start_y);
        if (status != OK) {
            start_x = 1;
            start_y = 1;
        }
    }

    eng->initial_player_x = start_x;
    eng->initial_player_y = start_y;

    Player *player = NULL;
    status = player_create(eng->initial_room_id, start_x, start_y, &player);
    if (status != OK) {
        graph_destroy(eng->graph);
        free(eng);
        return status;
    }

    eng->player = player;
    *engine_out = eng;
    return OK;
}

void game_engine_destroy(GameEngine *eng)
{
    if (eng == NULL) return;
    player_destroy(eng->player);
    graph_destroy(eng->graph);
    free(eng);
}

const Player *game_engine_get_player(const GameEngine *eng)
{
    if (eng == NULL) return NULL;
    return eng->player;
}

Status game_engine_move_player(GameEngine *eng, Direction dir)
{
    if (eng == NULL || eng->player == NULL) return INVALID_ARGUMENT;

    int px = 0;
    int py = 0;
    Status status = player_get_position(eng->player, &px, &py);
    if (status != OK) return status;

    int current_room_id = player_get_room(eng->player);
    Room *current_room  = find_room_by_id(eng, current_room_id);
    if (current_room == NULL) return GE_NO_SUCH_ROOM;

    int new_x = px;
    int new_y = py;
    switch (dir) {
        case DIR_NORTH: new_y = py - 1; break;
        case DIR_SOUTH: new_y = py + 1; break;
        case DIR_EAST:  new_x = px + 1; break;
        case DIR_WEST:  new_x = px - 1; break;
        default: return INVALID_ARGUMENT;
    }

    int tile_id = 0;
    RoomTileType tile_type = room_classify_tile(current_room, new_x, new_y, &tile_id);

    if (tile_type == ROOM_TILE_TREASURE) {
        Treasure *picked = NULL;
        Status ps = room_pick_up_treasure(current_room, tile_id, &picked);
        if (ps == OK && picked != NULL) {
            Treasure **new_arr = realloc(
                eng->player->collected_treasures,
                sizeof(Treasure *) * (size_t)(eng->player->collected_count + 1));
            if (new_arr != NULL) {
                eng->player->collected_treasures = new_arr;
                eng->player->collected_treasures[eng->player->collected_count] = picked;
                eng->player->collected_count++;
            }
        }
        return OK;
    }

    if (tile_type == ROOM_TILE_PORTAL && !portal_is_open(current_room, new_x, new_y)) {
        return ROOM_IMPASSABLE;
    }

    int pidx = -1;
    if (room_has_pushable_at(current_room, new_x, new_y, &pidx)) {
        Status ps = room_try_push(current_room, pidx, dir);
        if (ps != OK) return ROOM_IMPASSABLE;
    } else if (!room_is_walkable(current_room, new_x, new_y)) {
        return ROOM_IMPASSABLE;
    }

    status = player_set_position(eng->player, new_x, new_y);
    if (status != OK) return status;

    tile_type = room_classify_tile(current_room, new_x, new_y, &tile_id);
    if (tile_type == ROOM_TILE_PORTAL) {
        return handle_portal_transition(eng, tile_id);
    }

    return OK;
}

Status game_engine_get_room_count(const GameEngine *eng, int *count_out)
{
    if (eng == NULL) return INVALID_ARGUMENT;
    if (count_out == NULL) return NULL_POINTER;
    *count_out = eng->room_count;
    return OK;
}

Status game_engine_get_room_dimensions(const GameEngine *eng,
                                        int *width_out, int *height_out)
{
    if (eng == NULL) return INVALID_ARGUMENT;
    if (width_out == NULL || height_out == NULL) return NULL_POINTER;
    if (eng->player == NULL) return INTERNAL_ERROR;

    int current_room_id = player_get_room(eng->player);
    Room *current_room  = find_room_by_id(eng, current_room_id);
    if (current_room == NULL) return GE_NO_SUCH_ROOM;

    *width_out  = room_get_width(current_room);
    *height_out = room_get_height(current_room);
    return OK;
}

Status game_engine_reset(GameEngine *eng)
{
    if (eng == NULL) return INVALID_ARGUMENT;
    if (eng->player == NULL || eng->graph == NULL) return INTERNAL_ERROR;

    Status status = player_reset_to_start(eng->player,
                                          eng->initial_room_id,
                                          eng->initial_player_x,
                                          eng->initial_player_y);
    if (status != OK) return INTERNAL_ERROR;

    const void * const *all_rooms = NULL;
    int all_room_count = 0;
    GraphStatus gs = graph_get_all_payloads(eng->graph, &all_rooms, &all_room_count);
    if (gs != GRAPH_STATUS_OK) return INTERNAL_ERROR;

    for (int i = 0; i < all_room_count; i++) {
        Room *room = (Room *)all_rooms[i];
        for (int j = 0; j < room->treasure_count; j++) {
            room->treasures[j].collected = false;
            room->treasures[j].x = room->treasures[j].initial_x;
            room->treasures[j].y = room->treasures[j].initial_y;
        }
        for (int j = 0; j < room->pushable_count; j++) {
            room->pushables[j].x = room->pushables[j].initial_x;
            room->pushables[j].y = room->pushables[j].initial_y;
        }
    }

    return OK;
}

static char *build_output(const char *buffer, int width, int height)
{
    char *output = malloc((size_t)width * (size_t)height + (size_t)height + 1U);
    if (output == NULL) return NULL;
    int out_idx = 0;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            output[out_idx++] = buffer[row * width + col];
        }
        output[out_idx++] = '\n';
    }
    output[out_idx] = '\0';
    return output;
}

Status game_engine_render_current_room(const GameEngine *eng, char **str_out)
{
    if (eng == NULL || str_out == NULL) return INVALID_ARGUMENT;
    if (eng->player == NULL) return INTERNAL_ERROR;

    int current_room_id = player_get_room(eng->player);
    Room *current_room  = find_room_by_id(eng, current_room_id);
    if (current_room == NULL) return INTERNAL_ERROR;

    int width  = room_get_width(current_room);
    int height = room_get_height(current_room);

    char *buffer = malloc((size_t)width * (size_t)height);
    if (buffer == NULL) return NO_MEMORY;

    Status status = room_render(current_room, &eng->charset, buffer, width, height);
    if (status != OK) { free(buffer); return status; }

    int px = 0;
    int py = 0;
    status = player_get_position(eng->player, &px, &py);
    if (status != OK) { free(buffer); return status; }

    if (px >= 0 && px < width && py >= 0 && py < height) {
        buffer[py * width + px] = (char)eng->charset.player;
    }

    char *output = build_output(buffer, width, height);
    free(buffer);
    if (output == NULL) return NO_MEMORY;

    *str_out = output;
    return OK;
}

Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out)
{
    if (eng == NULL) return INVALID_ARGUMENT;
    if (str_out == NULL) return NULL_POINTER;

    Room *room = find_room_by_id(eng, room_id);
    if (room == NULL) return GE_NO_SUCH_ROOM;

    int width  = room_get_width(room);
    int height = room_get_height(room);

    char *buffer = malloc((size_t)width * (size_t)height);
    if (buffer == NULL) return NO_MEMORY;

    Status status = room_render(room, &eng->charset, buffer, width, height);
    if (status != OK) { free(buffer); return status; }

    char *output = build_output(buffer, width, height);
    free(buffer);
    if (output == NULL) return NO_MEMORY;

    *str_out = output;
    return OK;
}

Status game_engine_get_room_ids(const GameEngine *eng,
                                 int **ids_out, int *count_out)
{
    if (eng == NULL) return INVALID_ARGUMENT;
    if (ids_out == NULL || count_out == NULL) return NULL_POINTER;
    if (eng->graph == NULL) return INTERNAL_ERROR;

    const void * const *rooms = NULL;
    int room_count = 0;
    GraphStatus gs = graph_get_all_payloads(eng->graph, &rooms, &room_count);
    if (gs != GRAPH_STATUS_OK) return INTERNAL_ERROR;

    int *ids = malloc(sizeof(int) * (size_t)room_count);
    if (ids == NULL) return NO_MEMORY;

    for (int i = 0; i < room_count; i++) {
        ids[i] = ((const Room *)rooms[i])->id;
    }

    *ids_out   = ids;
    *count_out = room_count;
    return OK;
}

void game_engine_free_string(void *ptr)
{
    free(ptr);
}