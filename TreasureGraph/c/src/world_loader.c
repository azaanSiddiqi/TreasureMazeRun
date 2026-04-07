#include "world_loader.h"
#include "datagen.h"
#include "graph.h"
#include "room.h"
#include <stdlib.h>
#include <string.h>

static int room_compare(const void *a, const void *b)
{
    const Room *ra = (const Room *)a;
    const Room *rb = (const Room *)b;
    return ra->id - rb->id;
}

static void room_destroy_wrapper(void *room)
{
    room_destroy((Room *)room);
}

static bool *copy_floor_grid(const bool *src, int size)
{
    if (src == NULL) return NULL;
    bool *dest = malloc(sizeof(bool) * (size_t)size);
    if (dest == NULL) return NULL;
    memcpy(dest, src, sizeof(bool) * (size_t)size);
    return dest;
}

static Portal *copy_portals(const DG_Portal *src, int count)
{
    if (src == NULL || count == 0) return NULL;
    Portal *portals = malloc(sizeof(Portal) * (size_t)count);
    if (portals == NULL) return NULL;

    for (int i = 0; i < count; i++) {
        portals[i].id                = src[i].id;
        portals[i].x                 = src[i].x;
        portals[i].y                 = src[i].y;
        portals[i].target_room_id    = src[i].neighbor_id;
        portals[i].gated             = (src[i].required_switch_id >= 0);
        portals[i].required_switch_id = src[i].required_switch_id;
        portals[i].name = malloc(1);
        if (portals[i].name == NULL) {
            for (int j = 0; j < i; j++) free(portals[j].name);
            free(portals);
            return NULL;
        }
        portals[i].name[0] = '\0';
    }
    return portals;
}

static Treasure *copy_treasures(const DG_Treasure *src, int count, int room_id)
{
    if (src == NULL || count == 0) return NULL;
    Treasure *treasures = malloc(sizeof(Treasure) * (size_t)count);
    if (treasures == NULL) return NULL;

    for (int i = 0; i < count; i++) {
        treasures[i].id              = src[i].global_id;
        treasures[i].starting_room_id = room_id;
        treasures[i].initial_x       = src[i].x;
        treasures[i].initial_y       = src[i].y;
        treasures[i].x               = src[i].x;
        treasures[i].y               = src[i].y;
        treasures[i].collected       = false;

        const char *tname = (src[i].name != NULL) ? src[i].name : "";
        treasures[i].name = malloc(strlen(tname) + 1);
        if (treasures[i].name == NULL) {
            for (int j = 0; j < i; j++) free(treasures[j].name);
            free(treasures);
            return NULL;
        }
        strcpy(treasures[i].name, tname);
    }
    return treasures;
}

static Pushable *copy_pushables(const DG_Pushable *src, int count)
{
    if (src == NULL || count == 0) return NULL;
    Pushable *pushables = malloc(sizeof(Pushable) * (size_t)count);
    if (pushables == NULL) return NULL;

    for (int i = 0; i < count; i++) {
        pushables[i].id        = src[i].id;
        pushables[i].initial_x = src[i].x;
        pushables[i].initial_y = src[i].y;
        pushables[i].x         = src[i].x;
        pushables[i].y         = src[i].y;

        const char *pname = (src[i].name != NULL) ? src[i].name : "";
        pushables[i].name = malloc(strlen(pname) + 1);
        if (pushables[i].name == NULL) {
            for (int j = 0; j < i; j++) free(pushables[j].name);
            free(pushables);
            return NULL;
        }
        strcpy(pushables[i].name, pname);
    }
    return pushables;
}

static Switch *copy_switches(const DG_Switch *src, int count)
{
    if (src == NULL || count == 0) return NULL;
    Switch *switches = malloc(sizeof(Switch) * (size_t)count);
    if (switches == NULL) return NULL;

    for (int i = 0; i < count; i++) {
        switches[i].id        = src[i].id;
        switches[i].x         = src[i].x;
        switches[i].y         = src[i].y;
        switches[i].portal_id = src[i].portal_id;
    }
    return switches;
}

static void setup_charset(const DG_Charset *dg_charset, Charset *charset_out)
{
    if (dg_charset != NULL) {
        charset_out->wall       = dg_charset->wall;
        charset_out->floor      = dg_charset->floor;
        charset_out->player     = dg_charset->player;
        charset_out->treasure   = dg_charset->treasure;
        charset_out->portal     = dg_charset->portal;
        charset_out->pushable   = dg_charset->pushable;
        charset_out->switch_off = dg_charset->switch_off;
        charset_out->switch_on  = dg_charset->switch_on;
    } else {
        charset_out->wall       = '#';
        charset_out->floor      = '.';
        charset_out->player     = '@';
        charset_out->treasure   = '$';
        charset_out->portal     = 'X';
        charset_out->pushable   = 'O';
        charset_out->switch_off = '=';
        charset_out->switch_on  = '+';
    }
}

static Status load_one_room(Graph *graph, const DG_Room *dg_room,
                             Room **first_room_out)
{
    Room *room = room_create(dg_room->id, NULL, dg_room->width, dg_room->height);
    if (room == NULL) return NO_MEMORY;

    bool *floor_grid = copy_floor_grid(dg_room->floor_grid,
                                       dg_room->width * dg_room->height);
    if (dg_room->floor_grid != NULL && floor_grid == NULL) {
        room_destroy(room);
        return NO_MEMORY;
    }
    room_set_floor_grid(room, floor_grid);

    Portal *portals = copy_portals(dg_room->portals, dg_room->portal_count);
    if (dg_room->portal_count > 0 && portals == NULL) {
        room_destroy(room);
        return NO_MEMORY;
    }
    room_set_portals(room, portals, dg_room->portal_count);

    Treasure *treasures = copy_treasures(dg_room->treasures,
                                         dg_room->treasure_count, dg_room->id);
    if (dg_room->treasure_count > 0 && treasures == NULL) {
        room_destroy(room);
        return NO_MEMORY;
    }
    room_set_treasures(room, treasures, dg_room->treasure_count);

    Pushable *pushables = copy_pushables(dg_room->pushables, dg_room->pushable_count);
    if (dg_room->pushable_count > 0 && pushables == NULL) {
        room_destroy(room);
        return NO_MEMORY;
    }
    room->pushables      = pushables;
    room->pushable_count = dg_room->pushable_count;

    Switch *switches = copy_switches(dg_room->switches, dg_room->switch_count);
    if (dg_room->switch_count > 0 && switches == NULL) {
        room_destroy(room);
        return NO_MEMORY;
    }
    room->switches      = switches;
    room->switch_count  = dg_room->switch_count;

    GraphStatus gs = graph_insert(graph, room);
    if (gs != GRAPH_STATUS_OK) {
        room_destroy(room);
        return NO_MEMORY;
    }

    if (*first_room_out == NULL) *first_room_out = room;
    return OK;
}

static Status connect_portals(Graph *graph,
                               const void * const *all_rooms, int count)
{
    for (int i = 0; i < count; i++) {
        const Room *from_room = (const Room *)all_rooms[i];
        for (int j = 0; j < from_room->portal_count; j++) {
            int target_id = from_room->portals[j].target_room_id;
            if (target_id < 0) continue;

            Room target_key;
            target_key.id = target_id;
            const void *to_room = graph_get_payload(graph, &target_key);
            if (to_room == NULL) continue;

            GraphStatus gs = graph_connect(graph, from_room, to_room);
            if (gs != GRAPH_STATUS_OK && gs != GRAPH_STATUS_DUPLICATE_EDGE) {
                return INTERNAL_ERROR;
            }
        }
    }
    return OK;
}

Status loader_load_world(const char *config_file,
                         Graph **graph_out,
                         Room **first_room_out,
                         int  *num_rooms_out,
                         Charset *charset_out)
{
    if (config_file == NULL || graph_out == NULL || first_room_out == NULL ||
        num_rooms_out == NULL || charset_out == NULL) {
        return INVALID_ARGUMENT;
    }

    int dg_result = start_datagen(config_file);
    if (dg_result == DG_ERR_CONFIG) return WL_ERR_CONFIG;
    if (dg_result != DG_OK) return WL_ERR_DATAGEN;

    setup_charset(dg_get_charset(), charset_out);

    Graph *graph = NULL;
    GraphStatus gs = graph_create(room_compare, room_destroy_wrapper, &graph);
    if (gs != GRAPH_STATUS_OK) {
        stop_datagen();
        return NO_MEMORY;
    }

    Room *first_room = NULL;
    int room_count = 0;

    while (has_more_rooms()) {
        DG_Room dg_room = get_next_room();
        Status status = load_one_room(graph, &dg_room, &first_room);
        if (status != OK) {
            graph_destroy(graph);
            stop_datagen();
            return status;
        }
        room_count++;
    }

    const void * const *all_rooms = NULL;
    int all_room_count = 0;
    gs = graph_get_all_payloads(graph, &all_rooms, &all_room_count);
    if (gs != GRAPH_STATUS_OK) {
        graph_destroy(graph);
        stop_datagen();
        return INTERNAL_ERROR;
    }

    Status status = connect_portals(graph, all_rooms, all_room_count);
    if (status != OK) {
        graph_destroy(graph);
        stop_datagen();
        return status;
    }

    stop_datagen();
    *graph_out      = graph;
    *first_room_out = first_room;
    *num_rooms_out  = room_count;
    return OK;
}