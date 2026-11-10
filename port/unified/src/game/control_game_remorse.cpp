/* Written by port/tools/make_unified_values.py from remorse/src/game/control_game.cpp: remorse's data in its namespace
 * in the one program; do not edit. */

#include "values.h"
#include "control.h"
#include "gamestate.h"
#include "savefields.h"
#include "values_remorse.h"

namespace remorse_values {
#include "../../../remorse/src/game/control_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRemorseData_game_control_game();

void linkRemorseData_game_control_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (playingRemorse) {
        ::CONTROLLER_GLOBALS = remorse_values::CONTROLLER_GLOBALS;
        ::CONTROLLER_GLOBAL_COUNT = remorse_values::CONTROLLER_GLOBAL_COUNT;
    }
    return true;
}();

}
