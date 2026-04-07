#include <check.h>
#include <stdlib.h>
#include "player.h"

static Treasure make_treasure(int id)
{
    Treasure t;
    t.id = id; t.x = 5; t.y = 5; t.collected = false;
    t.initial_x = 5; t.initial_y = 5; t.starting_room_id = 1;
    t.name = NULL;
    return t;
}

START_TEST(test_player_create_valid)
{
    Player *p = NULL;
    Status status = player_create(1, 5, 10, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);
    ck_assert_int_eq(p->room_id, 1);
    ck_assert_int_eq(p->x, 5);
    ck_assert_int_eq(p->y, 10);
    ck_assert_ptr_null(p->collected_treasures);
    ck_assert_int_eq(p->collected_count, 0);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_create_null_output)
{
    ck_assert_int_eq(player_create(1, 5, 10, NULL), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_player_destroy_null)
{
    player_destroy(NULL);
    ck_assert(1);
}
END_TEST

START_TEST(test_player_get_room)
{
    Player *p = NULL;
    player_create(42, 1, 2, &p);
    ck_assert_int_eq(player_get_room(p), 42);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_get_room_null)
{
    ck_assert_int_eq(player_get_room(NULL), -1);
}
END_TEST

START_TEST(test_player_get_position)
{
    Player *p = NULL;
    player_create(1, 7, 9, &p);
    int x = 0, y = 0;
    ck_assert_int_eq(player_get_position(p, &x, &y), OK);
    ck_assert_int_eq(x, 7);
    ck_assert_int_eq(y, 9);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_get_position_null)
{
    Player *p = NULL;
    player_create(1, 5, 5, &p);
    int x = 0, y = 0;
    ck_assert_int_eq(player_get_position(NULL, &x, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(player_get_position(p, NULL, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(player_get_position(p, &x, NULL), INVALID_ARGUMENT);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_set_position)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    ck_assert_int_eq(player_set_position(p, 15, 20), OK);
    int x = 0, y = 0;
    player_get_position(p, &x, &y);
    ck_assert_int_eq(x, 15);
    ck_assert_int_eq(y, 20);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_set_position_null)
{
    ck_assert_int_eq(player_set_position(NULL, 5, 5), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_player_move_to_room)
{
    Player *p = NULL;
    player_create(1, 5, 5, &p);
    ck_assert_int_eq(player_move_to_room(p, 99), OK);
    ck_assert_int_eq(player_get_room(p), 99);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_reset_to_start)
{
    Player *p = NULL;
    player_create(1, 5, 5, &p);
    player_move_to_room(p, 10);
    player_set_position(p, 20, 25);
    ck_assert_int_eq(player_reset_to_start(p, 1, 5, 5), OK);
    ck_assert_int_eq(player_get_room(p), 1);
    int x = 0, y = 0;
    player_get_position(p, &x, &y);
    ck_assert_int_eq(x, 5);
    ck_assert_int_eq(y, 5);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_reset_null)
{
    ck_assert_int_eq(player_reset_to_start(NULL, 1, 5, 5), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_player_try_collect_ok)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    Treasure t = make_treasure(10);
    ck_assert_int_eq(player_try_collect(p, &t), OK);
    ck_assert_int_eq(p->collected_count, 1);
    ck_assert(t.collected);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_try_collect_multiple)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    Treasure t1 = make_treasure(1);
    Treasure t2 = make_treasure(2);
    Treasure t3 = make_treasure(3);
    player_try_collect(p, &t1);
    player_try_collect(p, &t2);
    player_try_collect(p, &t3);
    ck_assert_int_eq(player_get_collected_count(p), 3);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_try_collect_null)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    Treasure t = make_treasure(1);
    ck_assert_int_eq(player_try_collect(NULL, &t), NULL_POINTER);
    ck_assert_int_eq(player_try_collect(p, NULL), NULL_POINTER);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_try_collect_already_collected)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    Treasure t = make_treasure(1);
    t.collected = true;
    ck_assert_int_eq(player_try_collect(p, &t), INVALID_ARGUMENT);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_has_collected_treasure)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    Treasure t = make_treasure(42);
    player_try_collect(p, &t);
    ck_assert(player_has_collected_treasure(p, 42));
    ck_assert(!player_has_collected_treasure(p, 99));
    player_destroy(p);
}
END_TEST

START_TEST(test_player_has_collected_treasure_null)
{
    ck_assert(!player_has_collected_treasure(NULL, 0));
}
END_TEST

START_TEST(test_player_get_collected_count_zero)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    ck_assert_int_eq(player_get_collected_count(p), 0);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_get_collected_count_null)
{
    ck_assert_int_eq(player_get_collected_count(NULL), 0);
}
END_TEST

START_TEST(test_player_get_collected_treasures)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    Treasure t1 = make_treasure(5);
    Treasure t2 = make_treasure(6);
    player_try_collect(p, &t1);
    player_try_collect(p, &t2);
    int count = 0;
    const Treasure * const *arr = player_get_collected_treasures(p, &count);
    ck_assert_ptr_nonnull(arr);
    ck_assert_int_eq(count, 2);
    ck_assert_int_eq(arr[0]->id, 5);
    ck_assert_int_eq(arr[1]->id, 6);
    player_destroy(p);
}
END_TEST

START_TEST(test_player_get_collected_treasures_null)
{
    int count = 0;
    ck_assert_ptr_null(player_get_collected_treasures(NULL, &count));
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    ck_assert_ptr_null(player_get_collected_treasures(p, NULL));
    player_destroy(p);
}
END_TEST

START_TEST(test_player_reset_clears_collected)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    Treasure t1 = make_treasure(1);
    Treasure t2 = make_treasure(2);
    player_try_collect(p, &t1);
    player_try_collect(p, &t2);
    player_reset_to_start(p, 1, 0, 0);
    ck_assert_int_eq(player_get_collected_count(p), 0);
    ck_assert_ptr_null(p->collected_treasures);
    player_destroy(p);
}
END_TEST

Suite *player_suite(void)
{
    Suite *s = suite_create("Player");

    TCase *tc_a1 = tcase_create("A1_Regression");
    tcase_add_test(tc_a1, test_player_create_valid);
    tcase_add_test(tc_a1, test_player_create_null_output);
    tcase_add_test(tc_a1, test_player_destroy_null);
    tcase_add_test(tc_a1, test_player_get_room);
    tcase_add_test(tc_a1, test_player_get_room_null);
    tcase_add_test(tc_a1, test_player_get_position);
    tcase_add_test(tc_a1, test_player_get_position_null);
    tcase_add_test(tc_a1, test_player_set_position);
    tcase_add_test(tc_a1, test_player_set_position_null);
    tcase_add_test(tc_a1, test_player_move_to_room);
    tcase_add_test(tc_a1, test_player_reset_to_start);
    tcase_add_test(tc_a1, test_player_reset_null);
    suite_add_tcase(s, tc_a1);

    TCase *tc_collect = tcase_create("TreasureCollection");
    tcase_add_test(tc_collect, test_player_try_collect_ok);
    tcase_add_test(tc_collect, test_player_try_collect_multiple);
    tcase_add_test(tc_collect, test_player_try_collect_null);
    tcase_add_test(tc_collect, test_player_try_collect_already_collected);
    tcase_add_test(tc_collect, test_player_has_collected_treasure);
    tcase_add_test(tc_collect, test_player_has_collected_treasure_null);
    tcase_add_test(tc_collect, test_player_get_collected_count_zero);
    tcase_add_test(tc_collect, test_player_get_collected_count_null);
    tcase_add_test(tc_collect, test_player_get_collected_treasures);
    tcase_add_test(tc_collect, test_player_get_collected_treasures_null);
    tcase_add_test(tc_collect, test_player_reset_clears_collected);
    suite_add_tcase(s, tc_collect);

    return s;
}