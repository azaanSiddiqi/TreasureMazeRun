/*
 * test_game_engine_extra.c
 * -------------------------
 * Unit tests for the A3 extra GameEngine functions declared in
 * game_engine_extra.h.
 *
 * Covers:
 *   - game_engine_use_portal
 *   - game_engine_get_current_room_name
 *   - game_engine_get_current_room_id
 *   - game_engine_get_total_treasure_count
 *   - game_engine_get_room_adjacency
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "game_engine.h"
#include "game_engine_extra.h"

#define CONFIG "../assets/starter.ini"

/* ============================================================
 * game_engine_use_portal
 * ============================================================ */

START_TEST(test_use_portal_null_engine)
{
    ck_assert_int_eq(game_engine_use_portal(NULL), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_use_portal_no_adjacent_portal)
{
    /* The player starts in the middle of the room, not next to a portal.
     * Expecting ROOM_NO_PORTAL (or OK if they happen to spawn next to one). */
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);
    Status s = game_engine_use_portal(eng);
    ck_assert(s == ROOM_NO_PORTAL || s == OK);
    game_engine_destroy(eng);
}
END_TEST

/* ============================================================
 * game_engine_get_current_room_name
 * ============================================================ */

START_TEST(test_room_name_null_engine)
{
    char *name = NULL;
    ck_assert_int_eq(game_engine_get_current_room_name(NULL, &name),
                     INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_room_name_null_out)
{
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);
    ck_assert_int_eq(game_engine_get_current_room_name(eng, NULL),
                     INVALID_ARGUMENT);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_room_name_returns_string)
{
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);
    char *name = NULL;
    Status s = game_engine_get_current_room_name(eng, &name);
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(name);
    ck_assert_int_gt((int)strlen(name), 0);
    game_engine_free_string(name);
    game_engine_destroy(eng);
}
END_TEST

/* ============================================================
 * game_engine_get_current_room_id
 * ============================================================ */

START_TEST(test_room_id_null_engine)
{
    ck_assert_int_eq(game_engine_get_current_room_id(NULL), -1);
}
END_TEST

START_TEST(test_room_id_valid)
{
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);
    int id = game_engine_get_current_room_id(eng);
    ck_assert_int_ge(id, 0);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_room_id_matches_initial)
{
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);
    int id = game_engine_get_current_room_id(eng);
    /* After creation the player is in the initial room */
    ck_assert_int_eq(id, eng->initial_room_id);
    game_engine_destroy(eng);
}
END_TEST

/* ============================================================
 * game_engine_get_total_treasure_count
 * ============================================================ */

START_TEST(test_total_treasure_null_engine)
{
    int count = 0;
    ck_assert_int_eq(game_engine_get_total_treasure_count(NULL, &count),
                     INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_total_treasure_null_out)
{
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);
    ck_assert_int_eq(game_engine_get_total_treasure_count(eng, NULL),
                     INVALID_ARGUMENT);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_total_treasure_positive)
{
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);
    int count = 0;
    Status s = game_engine_get_total_treasure_count(eng, &count);
    ck_assert_int_eq(s, OK);
    /* starter.ini has treasures_per_room=3 and 3 rooms → at least 1 */
    ck_assert_int_gt(count, 0);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_total_treasure_ge_collected)
{
    /* Total should always be >= what the player has collected */
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);
    int total = 0;
    game_engine_get_total_treasure_count(eng, &total);
    int collected = player_get_collected_count(eng->player);
    ck_assert_int_ge(total, collected);
    game_engine_destroy(eng);
}
END_TEST

/* ============================================================
 * game_engine_get_room_adjacency
 * ============================================================ */

START_TEST(test_adjacency_null_args)
{
    int *m = NULL, *ids = NULL, n = 0;
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);

    ck_assert_int_eq(game_engine_get_room_adjacency(NULL, &m, &ids, &n),
                     INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_room_adjacency(eng, NULL, &ids, &n),
                     INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_room_adjacency(eng, &m, NULL, &n),
                     INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_room_adjacency(eng, &m, &ids, NULL),
                     INVALID_ARGUMENT);

    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_adjacency_returns_ok)
{
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);

    int *matrix = NULL, *ids = NULL, n = 0;
    Status s = game_engine_get_room_adjacency(eng, &matrix, &ids, &n);

    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(matrix);
    ck_assert_ptr_nonnull(ids);
    ck_assert_int_gt(n, 0);

    free(matrix);
    free(ids);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_adjacency_count_matches_room_count)
{
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);

    int *matrix = NULL, *ids = NULL, n = 0;
    game_engine_get_room_adjacency(eng, &matrix, &ids, &n);

    int room_count = 0;
    game_engine_get_room_count(eng, &room_count);
    ck_assert_int_eq(n, room_count);

    free(matrix);
    free(ids);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_adjacency_matrix_no_self_loops)
{
    /* The room graph should not have self-loops */
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);

    int *matrix = NULL, *ids = NULL, n = 0;
    game_engine_get_room_adjacency(eng, &matrix, &ids, &n);

    for (int i = 0; i < n; i++) {
        ck_assert_int_eq(matrix[i * n + i], 0);
    }

    free(matrix);
    free(ids);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_adjacency_has_some_edges)
{
    /* A valid multi-room world must have at least one edge */
    GameEngine *eng = NULL;
    game_engine_create(CONFIG, &eng);

    int *matrix = NULL, *ids = NULL, n = 0;
    game_engine_get_room_adjacency(eng, &matrix, &ids, &n);

    int edges = 0;
    for (int i = 0; i < n * n; i++) edges += matrix[i];
    ck_assert_int_gt(edges, 0);

    free(matrix);
    free(ids);
    game_engine_destroy(eng);
}
END_TEST

/* ============================================================
 * Suite registration
 * ============================================================ */

Suite *game_engine_extra_suite(void)
{
    Suite *s = suite_create("GameEngineExtra");

    TCase *tc_portal = tcase_create("UsePortal");
    tcase_add_test(tc_portal, test_use_portal_null_engine);
    tcase_add_test(tc_portal, test_use_portal_no_adjacent_portal);
    suite_add_tcase(s, tc_portal);

    TCase *tc_name = tcase_create("RoomName");
    tcase_add_test(tc_name, test_room_name_null_engine);
    tcase_add_test(tc_name, test_room_name_null_out);
    tcase_add_test(tc_name, test_room_name_returns_string);
    suite_add_tcase(s, tc_name);

    TCase *tc_id = tcase_create("RoomId");
    tcase_add_test(tc_id, test_room_id_null_engine);
    tcase_add_test(tc_id, test_room_id_valid);
    tcase_add_test(tc_id, test_room_id_matches_initial);
    suite_add_tcase(s, tc_id);

    TCase *tc_total = tcase_create("TotalTreasure");
    tcase_add_test(tc_total, test_total_treasure_null_engine);
    tcase_add_test(tc_total, test_total_treasure_null_out);
    tcase_add_test(tc_total, test_total_treasure_positive);
    tcase_add_test(tc_total, test_total_treasure_ge_collected);
    suite_add_tcase(s, tc_total);

    TCase *tc_adj = tcase_create("RoomAdjacency");
    tcase_add_test(tc_adj, test_adjacency_null_args);
    tcase_add_test(tc_adj, test_adjacency_returns_ok);
    tcase_add_test(tc_adj, test_adjacency_count_matches_room_count);
    tcase_add_test(tc_adj, test_adjacency_matrix_no_self_loops);
    tcase_add_test(tc_adj, test_adjacency_has_some_edges);
    suite_add_tcase(s, tc_adj);

    return s;
}
