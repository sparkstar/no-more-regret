/* Written by port/tools/make_unified_values.py from regret/src/game/control_game.cpp: regret's data in its namespace in
 * the one program; do not edit. */

#include "values.h"
#include "control.h"
#include "gamestate.h"
#include "savefields.h"
#include "values_regret.h"

namespace regret_values {
#include "../../../regret/src/game/control_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRegretData_game_control_game();

void linkRegretData_game_control_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (!playingRemorse) {
        ::CONTROLLER_GLOBALS = regret_values::CONTROLLER_GLOBALS;
        ::CONTROLLER_GLOBAL_COUNT = regret_values::CONTROLLER_GLOBAL_COUNT;
    }
    return true;
}();

}
