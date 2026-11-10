/* Written by port/tools/make_unified_values.py from remorse/src/game/wpncache_game.cpp: remorse's data in its namespace
 * in the one program; do not edit. */

#include "values.h"
#include "wpncache.h"
#include "gamestate.h"
#include "dtable.h"
#include "values_remorse.h"

namespace remorse_values {
#include "../../../remorse/src/game/wpncache_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRemorseData_game_wpncache_game();

void linkRemorseData_game_wpncache_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (playingRemorse) {
        ::overlayWeapons = remorse_values::overlayWeapons;
    }
    return true;
}();

}
