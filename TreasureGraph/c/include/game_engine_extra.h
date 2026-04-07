#ifndef GAME_ENGINE_EXTRA_H
#define GAME_ENGINE_EXTRA_H

/*
 * game_engine_extra.h
 * --------------------
 * Additional GameEngine API added for A3.
 *
 * Rules followed:
 *   - Does NOT edit any existing .h file.
 *   - All new declarations live here and are implemented in
 *     game_engine_extra.c (same src/ folder, picked up by the wildcard).
 *   - New C functions must be tested (tests added to test_integration.c).
 */

#include "game_engine.h"

/* ============================================================
 * Portal traversal (required for '>' key)
 * ============================================================ */

/*
 * game_engine_use_portal
 * ----------------------
 * Scan the four tiles adjacent to the player.
 * If an open (unlocked) portal is found, transition the player to its
 * target room and place them at the entry position.
 *
 * Returns:
 *   OK             – portal found and traversed
 *   ROOM_NO_PORTAL – no open portal adjacent to the player
 *   INVALID_ARGUMENT – eng is NULL
 *   GE_NO_SUCH_ROOM  – target room not found in the graph
 */
Status game_engine_use_portal(GameEngine *eng);

/* ============================================================
 * Room metadata accessors (for UI display)
 * ============================================================ */

/*
 * game_engine_get_current_room_name
 * ----------------------------------
 * Allocate and return a copy of the current room's name string.
 * The caller must free the string with game_engine_free_string().
 *
 * Returns:
 *   OK               – name_out set to a newly allocated copy
 *   INVALID_ARGUMENT – eng or name_out is NULL
 *   GE_NO_SUCH_ROOM  – current room not in the graph
 *   NO_MEMORY        – allocation failure
 */
Status game_engine_get_current_room_name(const GameEngine *eng,
                                         char **name_out);

/*
 * game_engine_get_current_room_id
 * --------------------------------
 * Return the room ID of the room the player is currently in.
 *
 * Returns:
 *   Room ID on success, -1 if eng or player is NULL.
 */
int game_engine_get_current_room_id(const GameEngine *eng);

/* ============================================================
 * Extended feature: Collect all the Treasure
 * ============================================================ */

/*
 * game_engine_get_total_treasure_count
 * -------------------------------------
 * Sum the treasure_count across every room in the world.
 * This counts ALL treasures, collected or not, giving the
 * victory condition total.
 *
 * Parameters:
 *   eng       – the game engine
 *   count_out – receives the total treasure count
 *
 * Returns:
 *   OK               – count_out set correctly
 *   INVALID_ARGUMENT – eng or count_out is NULL
 *   INTERNAL_ERROR   – graph data is invalid
 */
Status game_engine_get_total_treasure_count(const GameEngine *eng,
                                            int *count_out);

/* ============================================================
 * Extended feature: Enhanced UI (minimap)
 * ============================================================ */

/*
 * game_engine_get_room_adjacency
 * --------------------------------
 * Build and return an adjacency matrix for the room graph together
 * with the ordered room-ID array.
 *
 * The matrix is a flat (row-major) n×n integer array where
 *   matrix[i*n + j] == 1  iff  there is a directed edge from
 *                              room ids_out[i]  to  room ids_out[j].
 *
 * Parameters:
 *   eng        – the game engine
 *   matrix_out – receives a newly allocated int[n*n] (caller must free)
 *   ids_out    – receives a newly allocated int[n]   (caller must free)
 *   count_out  – receives n (number of rooms)
 *
 * Returns:
 *   OK               – matrix_out, ids_out, count_out set
 *   INVALID_ARGUMENT – any pointer argument is NULL
 *   INTERNAL_ERROR   – graph query failed
 *   NO_MEMORY        – allocation failure
 *
 * Ownership:
 *   The caller owns matrix_out and ids_out; free both with free().
 *   (They are plain int arrays, not engine strings.)
 */
Status game_engine_get_room_adjacency(const GameEngine *eng,
                                      int **matrix_out,
                                      int **ids_out,
                                      int  *count_out);

#endif /* GAME_ENGINE_EXTRA_H */
