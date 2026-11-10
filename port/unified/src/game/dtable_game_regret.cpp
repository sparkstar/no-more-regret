/* Written by port/tools/make_unified_values.py from regret/src/game/dtable_game.cpp: regret's data in its namespace in
 * the one program; do not edit. */

#include "values.h"
#include "dtable.h"
#include "gamestate.h"
#include "values_regret.h"

namespace regret_values {
#include "../../../regret/src/game/dtable_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRegretData_game_dtable_game();

void linkRegretData_game_dtable_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (!playingRemorse) {
        ::ammoTypes = regret_values::ammoTypes;
        ::weaponTypes = regret_values::weaponTypes;
        ::inventoryTypes = regret_values::inventoryTypes;
        ::bombTypes = regret_values::bombTypes;
        ::weaponNames = regret_values::weaponNames;
        ::inventoryNames = regret_values::inventoryNames;
        ::ammoNames = regret_values::ammoNames;
        ::bombNames = regret_values::bombNames;
    }
    return true;
}();

}
