/* Written by port/tools/make_unified_values.py from remorse/src/game/firetype_game.cpp: remorse's data in its namespace
 * in the one program; do not edit. */

#include "values.h"
#include "firetype.h"
#include "gamestate.h"
#include "values_remorse.h"

namespace remorse_values {
#include "../../../remorse/src/game/firetype_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRemorseData_game_firetype_game();

void linkRemorseData_game_firetype_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (playingRemorse) {
        ::fireTypes = remorse_values::fireTypes;
    }
    return true;
}();

}
