/* Written by port/tools/make_unified_values.py from remorse/src/game/dtable_game.cpp: remorse's data in its namespace
 * in the one program; do not edit. */

#include "values.h"
#include "dtable.h"
#include "gamestate.h"
#include "values_remorse.h"

namespace remorse_values {
#include "../../../remorse/src/game/dtable_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRemorseData_game_dtable_game();

void linkRemorseData_game_dtable_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (playingRemorse) {
        ::ammoTypes = remorse_values::ammoTypes;
        ::weaponTypes = remorse_values::weaponTypes;
        ::inventoryTypes = remorse_values::inventoryTypes;
        ::bombTypes = remorse_values::bombTypes;
        ::weaponNames = remorse_values::weaponNames;
        ::inventoryNames = remorse_values::inventoryNames;
        ::ammoNames = remorse_values::ammoNames;
        ::bombNames = remorse_values::bombNames;
    }
    return true;
}();

}
