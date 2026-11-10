/* Written by port/tools/make_unified_values.py from regret/src/game/wpncache_game.cpp: regret's data in its namespace
 * in the one program; do not edit. */

#include "values.h"
#include "wpncache.h"
#include "gamestate.h"
#include "dtable.h"
#include "values_regret.h"

namespace regret_values {
#include "../../../regret/src/game/wpncache_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRegretData_game_wpncache_game();

void linkRegretData_game_wpncache_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (!playingRemorse) {
        ::overlayWeapons = regret_values::overlayWeapons;
    }
    return true;
}();

}
