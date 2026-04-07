#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "room.h"

static Room *make_room_10x10(void)
{
    return room_create(1, "Test", 10, 10);
}

static Pushable *make_pushable_at(int x, int y)
{
    Pushable *p = malloc(sizeof(Pushable));
    p->id = 0; p->x = x; p->y = y;
    p->initial_x = x; p->initial_y = y;
    p->name = malloc(1); p->name[0] = '\0';
    return p;
}

static Treasure *make_treasure(int id, int x, int y, bool collected)
{
    Treasure *t = malloc(sizeof(Treasure));
    t->id = id; t->x = x; t->y = y; t->collected = collected;
    t->initial_x = x; t->initial_y = y; t->starting_room_id = 1;
    t->name = malloc(1); t->name[0] = '\0';
    return t;
}

START_TEST(test_room_create_valid)
{
    Room *r = room_create(1, "Test Room", 10, 15);
    ck_assert_ptr_nonnull(r);
    ck_assert_int_eq(r->id, 1);
    ck_assert_str_eq(r->name, "Test Room");
    ck_assert_int_eq(r->width, 10);
    ck_assert_int_eq(r->height, 15);
    ck_assert_ptr_null(r->floor_grid);
    ck_assert_ptr_null(r->portals);
    ck_assert_int_eq(r->portal_count, 0);
    ck_assert_ptr_null(r->treasures);
    ck_assert_int_eq(r->treasure_count, 0);
    ck_assert_ptr_null(r->pushables);
    ck_assert_int_eq(r->pushable_count, 0);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_create_null_name)
{
    Room *r = room_create(1, NULL, 10, 15);
    ck_assert_ptr_nonnull(r);
    ck_assert_ptr_null(r->name);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_destroy_null) { room_destroy(NULL); ck_assert(1); }
END_TEST

START_TEST(test_room_get_width) { Room *r = make_room_10x10(); ck_assert_int_eq(room_get_width(r), 10); room_destroy(r); }
END_TEST
START_TEST(test_room_get_width_null) { ck_assert_int_eq(room_get_width(NULL), 0); }
END_TEST
START_TEST(test_room_get_height) { Room *r = make_room_10x10(); ck_assert_int_eq(room_get_height(r), 10); room_destroy(r); }
END_TEST
START_TEST(test_room_get_height_null) { ck_assert_int_eq(room_get_height(NULL), 0); }
END_TEST
START_TEST(test_room_get_id) { Room *r = room_create(42, "X", 5, 5); ck_assert_int_eq(room_get_id(r), 42); room_destroy(r); }
END_TEST
START_TEST(test_room_get_id_null) { ck_assert_int_eq(room_get_id(NULL), -1); }
END_TEST

START_TEST(test_room_set_floor_grid)
{
    Room *r = make_room_10x10();
    bool *grid = malloc(sizeof(bool) * 100);
    for (int i = 0; i < 100; i++) grid[i] = true;
    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);
    ck_assert_ptr_eq(r->floor_grid, grid);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_set_floor_grid_null_room)
{
    bool grid[9] = {true};
    ck_assert_int_eq(room_set_floor_grid(NULL, grid), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_room_is_walkable_default)
{
    Room *r = make_room_10x10();
    ck_assert(!room_is_walkable(r, 0, 0));
    ck_assert(!room_is_walkable(r, 9, 9));
    ck_assert(room_is_walkable(r, 5, 5));
    ck_assert(!room_is_walkable(r, -1, 5));
    room_destroy(r);
}
END_TEST

START_TEST(test_room_has_pushable_at_true)
{
    Room *r = make_room_10x10();
    r->pushables = make_pushable_at(5, 5);
    r->pushable_count = 1;
    int idx = -99;
    ck_assert(room_has_pushable_at(r, 5, 5, &idx));
    ck_assert_int_eq(idx, 0);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_has_pushable_at_false)
{
    Room *r = make_room_10x10();
    r->pushables = make_pushable_at(5, 5);
    r->pushable_count = 1;
    ck_assert(!room_has_pushable_at(r, 3, 3, NULL));
    room_destroy(r);
}
END_TEST

START_TEST(test_room_has_pushable_at_null) { ck_assert(!room_has_pushable_at(NULL, 5, 5, NULL)); }
END_TEST

START_TEST(test_room_is_walkable_blocked_by_pushable)
{
    Room *r = make_room_10x10();
    r->pushables = make_pushable_at(5, 5);
    r->pushable_count = 1;
    ck_assert(!room_is_walkable(r, 5, 5));
    ck_assert(room_is_walkable(r, 4, 4));
    room_destroy(r);
}
END_TEST

START_TEST(test_room_try_push_south)
{
    Room *r = make_room_10x10();
    r->pushables = make_pushable_at(5, 5);
    r->pushable_count = 1;
    ck_assert_int_eq(room_try_push(r, 0, DIR_SOUTH), OK);
    ck_assert_int_eq(r->pushables[0].y, 6);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_try_push_north)
{
    Room *r = make_room_10x10();
    r->pushables = make_pushable_at(5, 5);
    r->pushable_count = 1;
    ck_assert_int_eq(room_try_push(r, 0, DIR_NORTH), OK);
    ck_assert_int_eq(r->pushables[0].y, 4);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_try_push_east)
{
    Room *r = make_room_10x10();
    r->pushables = make_pushable_at(5, 5);
    r->pushable_count = 1;
    ck_assert_int_eq(room_try_push(r, 0, DIR_EAST), OK);
    ck_assert_int_eq(r->pushables[0].x, 6);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_try_push_west)
{
    Room *r = make_room_10x10();
    r->pushables = make_pushable_at(5, 5);
    r->pushable_count = 1;
    ck_assert_int_eq(room_try_push(r, 0, DIR_WEST), OK);
    ck_assert_int_eq(r->pushables[0].x, 4);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_try_push_blocked_by_wall)
{
    Room *r = make_room_10x10();
    r->pushables = make_pushable_at(5, 8);
    r->pushable_count = 1;
    ck_assert_int_eq(room_try_push(r, 0, DIR_SOUTH), ROOM_IMPASSABLE);
    ck_assert_int_eq(r->pushables[0].y, 8);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_try_push_blocked_by_pushable)
{
    Room *r = make_room_10x10();
    Pushable *push = malloc(sizeof(Pushable) * 2);
    push[0] = (Pushable){0, 5, 5, 5, 5, NULL}; push[0].name = malloc(1); push[0].name[0] = '\0';
    push[1] = (Pushable){1, 5, 6, 5, 6, NULL}; push[1].name = malloc(1); push[1].name[0] = '\0';
    r->pushables = push; r->pushable_count = 2;
    ck_assert_int_eq(room_try_push(r, 0, DIR_SOUTH), ROOM_IMPASSABLE);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_try_push_invalid)
{
    Room *r = make_room_10x10();
    ck_assert_int_eq(room_try_push(NULL, 0, DIR_SOUTH), INVALID_ARGUMENT);
    ck_assert_int_eq(room_try_push(r, -1, DIR_SOUTH), INVALID_ARGUMENT);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_pick_up_treasure_ok)
{
    Room *r = make_room_10x10();
    r->treasures = make_treasure(7, 5, 5, false);
    r->treasure_count = 1;
    Treasure *out = NULL;
    ck_assert_int_eq(room_pick_up_treasure(r, 7, &out), OK);
    ck_assert_ptr_nonnull(out);
    ck_assert(out->collected);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_pick_up_treasure_already_collected)
{
    Room *r = make_room_10x10();
    r->treasures = make_treasure(7, 5, 5, true);
    r->treasure_count = 1;
    Treasure *out = NULL;
    ck_assert_int_eq(room_pick_up_treasure(r, 7, &out), INVALID_ARGUMENT);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_pick_up_treasure_not_found)
{
    Room *r = make_room_10x10();
    Treasure *out = NULL;
    ck_assert_int_eq(room_pick_up_treasure(r, 99, &out), ROOM_NOT_FOUND);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_pick_up_treasure_null_args)
{
    Room *r = make_room_10x10();
    Treasure *out = NULL;
    ck_assert_int_eq(room_pick_up_treasure(NULL, 0, &out), INVALID_ARGUMENT);
    ck_assert_int_eq(room_pick_up_treasure(r, 0, NULL), INVALID_ARGUMENT);
    room_destroy(r);
}
END_TEST

START_TEST(test_destroy_treasure_null) { destroy_treasure(NULL); ck_assert(1); }
END_TEST

START_TEST(test_room_classify_tile_pushable)
{
    Room *r = make_room_10x10();
    r->pushables = make_pushable_at(5, 5);
    r->pushable_count = 1;
    int id = -1;
    ck_assert_int_eq(room_classify_tile(r, 5, 5, &id), ROOM_TILE_PUSHABLE);
    ck_assert_int_eq(id, 0);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_classify_collected_as_floor)
{
    Room *r = make_room_10x10();
    r->treasures = make_treasure(5, 5, 5, true);
    r->treasure_count = 1;
    ck_assert_int_eq(room_classify_tile(r, 5, 5, NULL), ROOM_TILE_FLOOR);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_render_pushable)
{
    Room *r = room_create(1, "T", 5, 5);
    r->pushables = make_pushable_at(2, 2);
    r->pushable_count = 1;
    Charset cs = {'#', '.', '@', 'O', '$', 'X', '=', '+'};
    char buf[25];
    ck_assert_int_eq(room_render(r, &cs, buf, 5, 5), OK);
    ck_assert_int_eq(buf[2 * 5 + 2], 'O');
    room_destroy(r);
}
END_TEST

START_TEST(test_room_render_collected_treasure_as_floor)
{
    Room *r = room_create(1, "T", 5, 5);
    r->treasures = make_treasure(1, 2, 2, true);
    r->treasure_count = 1;
    Charset cs = {'#', '.', '@', 'O', '$', 'X', '=', '+'};
    char buf[25];
    ck_assert_int_eq(room_render(r, &cs, buf, 5, 5), OK);
    ck_assert_int_eq(buf[2 * 5 + 2], '.');
    room_destroy(r);
}
END_TEST

Suite *room_suite(void)
{
    Suite *s = suite_create("Room");

    TCase *tc_a1 = tcase_create("A1_Regression");
    tcase_add_test(tc_a1, test_room_create_valid);
    tcase_add_test(tc_a1, test_room_create_null_name);
    tcase_add_test(tc_a1, test_room_destroy_null);
    tcase_add_test(tc_a1, test_room_get_width);
    tcase_add_test(tc_a1, test_room_get_width_null);
    tcase_add_test(tc_a1, test_room_get_height);
    tcase_add_test(tc_a1, test_room_get_height_null);
    tcase_add_test(tc_a1, test_room_set_floor_grid);
    tcase_add_test(tc_a1, test_room_set_floor_grid_null_room);
    tcase_add_test(tc_a1, test_room_is_walkable_default);
    suite_add_tcase(s, tc_a1);

    TCase *tc_id = tcase_create("RoomID");
    tcase_add_test(tc_id, test_room_get_id);
    tcase_add_test(tc_id, test_room_get_id_null);
    suite_add_tcase(s, tc_id);

    TCase *tc_push = tcase_create("Pushables");
    tcase_add_test(tc_push, test_room_has_pushable_at_true);
    tcase_add_test(tc_push, test_room_has_pushable_at_false);
    tcase_add_test(tc_push, test_room_has_pushable_at_null);
    tcase_add_test(tc_push, test_room_is_walkable_blocked_by_pushable);
    tcase_add_test(tc_push, test_room_try_push_south);
    tcase_add_test(tc_push, test_room_try_push_north);
    tcase_add_test(tc_push, test_room_try_push_east);
    tcase_add_test(tc_push, test_room_try_push_west);
    tcase_add_test(tc_push, test_room_try_push_blocked_by_wall);
    tcase_add_test(tc_push, test_room_try_push_blocked_by_pushable);
    tcase_add_test(tc_push, test_room_try_push_invalid);
    suite_add_tcase(s, tc_push);

    TCase *tc_treas = tcase_create("Treasure");
    tcase_add_test(tc_treas, test_room_pick_up_treasure_ok);
    tcase_add_test(tc_treas, test_room_pick_up_treasure_already_collected);
    tcase_add_test(tc_treas, test_room_pick_up_treasure_not_found);
    tcase_add_test(tc_treas, test_room_pick_up_treasure_null_args);
    tcase_add_test(tc_treas, test_destroy_treasure_null);
    suite_add_tcase(s, tc_treas);

    TCase *tc_misc = tcase_create("Classify_Render");
    tcase_add_test(tc_misc, test_room_classify_tile_pushable);
    tcase_add_test(tc_misc, test_room_classify_collected_as_floor);
    tcase_add_test(tc_misc, test_room_render_pushable);
    tcase_add_test(tc_misc, test_room_render_collected_treasure_as_floor);
    suite_add_tcase(s, tc_misc);

    return s;
}