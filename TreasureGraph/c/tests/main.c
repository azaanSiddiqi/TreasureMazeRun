#include <check.h>
#include <stdlib.h>

Suite *player_suite(void);
Suite *room_suite(void);
Suite *integration_suite(void);
Suite *game_engine_extra_suite(void);   /* A3 */

int main(void)
{
    Suite *suites[] = {
        player_suite(),
        room_suite(),
        integration_suite(),
        game_engine_extra_suite(),      /* A3 */
        NULL
    };

    SRunner *runner = srunner_create(suites[0]);
    for (int i = 1; suites[i] != NULL; ++i) {
        srunner_add_suite(runner, suites[i]);
    }

    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
