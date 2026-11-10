/* Written by port/tools/make_unified_values.py from regret/src/world/mapjump_game.h and
 * remorse/src/world/mapjump_game.h: the one program's run-time values; do not edit. */

#ifndef PORT_MAPJUMP_GAME_H
#define PORT_MAPJUMP_GAME_H
#include <span>
#include <array>
#include "movie.h"

extern bool JUMP_SHOWS_LOADING;
extern bool JUMP_COMPACTS_MEMORY;
extern bool JUMP_RESETS_SOUND_CACHE;
extern bool JUMP_MOVES_TO_EGG;
extern std::array<MovieRequest, 2> INTRO_MOVIES;
extern u16 INTRO_PAUSE;
extern bool START_HOLDS_SILENCER;
#endif
