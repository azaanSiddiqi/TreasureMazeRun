#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "world_loader.h"
#include "game_engine.h"
#include "player.h"
#include "graph.h"
#include "room.h"

START_TEST(test_loader_valid)
{
    Graph *graph = NULL; Room *first_room = NULL;
    int num_rooms = 0; Charset charset;
    Status status = loader_load_world("../assets/starter.ini", &graph,
                                     &first_room, &num_rooms, &charset);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(graph);
    ck_assert_ptr_nonnull(first_room);
    ck_assert_int_gt(num_rooms, 0);
    graph_destroy(graph);
}
END_TEST

START_TEST(test_loader_null_inputs)
{
    Graph *g = NULL; Room *r = NULL; int n = 0; Charset c;
    ck_assert_int_eq(loader_load_world(NULL, &g, &r, &n, &c), INVALID_ARGUMENT);
    ck_assert_int_eq(loader_load_world("../assets/starter.ini", NULL, &r, &n, &c),
                     INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_loader_invalid_config)
{
    Graph *g = NULL; Room *r = NULL; int n = 0; Charset c;
    Status status = loader_load_world("nonexistent.ini", &g, &r, &n, &c);
    ck_assert(status == WL_ERR_CONFIG || status == WL_ERR_DATAGEN);
}
END_TEST

START_TEST(test_engine_create_valid)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &eng), OK);
    ck_assert_ptr_nonnull(eng);
    ck_assert_ptr_nonnull(eng->graph);
    ck_assert_ptr_nonnull(eng->player);
    ck_assert_int_gt(eng->room_count, 0);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_engine_create_null)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(game_engine_create(NULL, &eng), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", NULL), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_destroy_null) { game_engine_destroy(NULL); ck_assert(1); }
END_TEST

START_TEST(test_engine_get_player)
{
    GameEngine *eng = NULL;
    game_engine_create("../assets/starter.ini", &eng);
    ck_assert_ptr_nonnull(game_engine_get_player(eng));
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_engine_get_room_count)
{
    GameEngine *eng = NULL;
    game_engine_create("../assets/starter.ini", &eng);
    int count = 0;
    ck_assert_int_eq(game_engine_get_room_count(eng, &count), OK);
    ck_assert_int_gt(count, 0);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_engine_get_room_dimensions)
{
    GameEngine *eng = NULL;
    game_engine_create("../assets/starter.ini", &eng);
    int w = 0, h = 0;
    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w, &h), OK);
    ck_assert_int_gt(w, 0);
    ck_assert_int_gt(h, 0);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_engine_move_player)
{
    GameEngine *eng = NULL;
    game_engine_create("../assets/starter.ini", &eng);
    Status s = game_engine_move_player(eng, DIR_EAST);
    ck_assert(s == OK || s == ROOM_IMPASSABLE);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_engine_reset)
{
    GameEngine *eng = NULL;
    game_engine_create("../assets/starter.ini", &eng);
    const Player *p = game_engine_get_player(eng);
    int init_room = player_get_room(p);
    int ix = 0, iy = 0;
    player_get_position(p, &ix, &iy);
    game_engine_move_player(eng, DIR_EAST);
    game_engine_move_player(eng, DIR_SOUTH);
    ck_assert_int_eq(game_engine_reset(eng), OK);
    ck_assert_int_eq(player_get_room(p), init_room);
    int rx = 0, ry = 0;
    player_get_position(p, &rx, &ry);
    ck_assert_int_eq(rx, ix);
    ck_assert_int_eq(ry, iy);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_engine_render_current_room)
{
    GameEngine *eng = NULL;
    game_engine_create("../assets/starter.ini", &eng);
    char *render = NULL;
    ck_assert_int_eq(game_engine_render_current_room(eng, &render), OK);
    ck_assert_ptr_nonnull(render);
    ck_assert(strchr(render, '#') != NULL);
    ck_assert(strchr(render, '@') != NULL);
    game_engine_free_string(render);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_engine_get_room_ids)
{
    GameEngine *eng = NULL;
    game_engine_create("../assets/starter.ini", &eng);
    int *ids = NULL; int count = 0;
    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, &count), OK);
    ck_assert_ptr_nonnull(ids);
    ck_assert_int_gt(count, 0);
    game_engine_free_string(ids);
    game_engine_destroy(eng);
}
END_TEST

START_TEST(test_engine_free_string_null)
{
    game_engine_free_string(NULL);
    ck_assert(1);
}
END_TEST

START_TEST(test_engine_reset_resets_pushables)
{
    GameEngine *eng = NULL;
    game_engine_create("../assets/starter.ini", &eng);
    char *r1 = NULL;
    game_engine_render_current_room(eng, &r1);
    game_engine_reset(eng);
    char *r2 = NULL;
    game_engine_render_current_room(eng, &r2);
    ck_assert_str_eq(r1, r2);
    game_engine_free_string(r1);
    game_engine_free_string(r2);
    game_engine_destroy(eng);
}
END_TEST

Suite *integration_suite(void)
{
    Suite *s = suite_create("Integration");

    TCase *tc_loader = tcase_create("WorldLoader");
    tcase_add_test(tc_loader, test_loader_valid);
    tcase_add_test(tc_loader, test_loader_null_inputs);
    tcase_add_test(tc_loader, test_loader_invalid_config);
    suite_add_tcase(s, tc_loader);

    TCase *tc_engine = tcase_create("GameEngine_Basic");
    tcase_add_test(tc_engine, test_engine_create_valid);
    tcase_add_test(tc_engine, test_engine_create_null);
    tcase_add_test(tc_engine, test_engine_destroy_null);
    tcase_add_test(tc_engine, test_engine_get_player);
    suite_add_tcase(s, tc_engine);

    TCase *tc_queries = tcase_create("GameEngine_Queries");
    tcase_add_test(tc_queries, test_engine_get_room_count);
    tcase_add_test(tc_queries, test_engine_get_room_dimensions);
    tcase_add_test(tc_queries, test_engine_get_room_ids);
    suite_add_tcase(s, tc_queries);

    TCase *tc_actions = tcase_create("GameEngine_Actions");
    tcase_add_test(tc_actions, test_engine_move_player);
    tcase_add_test(tc_actions, test_engine_reset);
    tcase_add_test(tc_actions, test_engine_reset_resets_pushables);
    suite_add_tcase(s, tc_actions);

    TCase *tc_render = tcase_create("GameEngine_Render");
    tcase_add_test(tc_render, test_engine_render_current_room);
    suite_add_tcase(s, tc_render);

    TCase *tc_a2 = tcase_create("A2_Specific");
    tcase_add_test(tc_a2, test_engine_free_string_null);
    suite_add_tcase(s, tc_a2);

    return s;
}