/* Written by port/tools/make_unified_values.py from regret/src/game/firetype_game.cpp: regret's data in its namespace
 * in the one program; do not edit. */

#include "values.h"
#include "firetype.h"
#include "gamestate.h"
#include "values_regret.h"

namespace regret_values {
#include "../../../regret/src/game/firetype_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRegretData_game_firetype_game();

void linkRegretData_game_firetype_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (!playingRemorse) {
        ::fireTypes = regret_values::fireTypes;
    }
    return true;
}();

}
