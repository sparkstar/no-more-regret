/* Written by port/tools/make_unified_values.py from regret/src/input/bindings_game.cpp: regret's data in its namespace
 * in the one program; do not edit. */

#include "values.h"
#include "bindings.h"
#include <iterator>
#include "values_regret.h"

namespace regret_values {
#include "../../../regret/src/input/bindings_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRegretData_input_bindings_game();

void linkRegretData_input_bindings_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (!playingRemorse) {
        ::GAME_ACTION_DEFAULTS = regret_values::GAME_ACTION_DEFAULTS;
        ::GAME_ACTION_DEFAULT_COUNT = regret_values::GAME_ACTION_DEFAULT_COUNT;
        ::GAME_MODERN_BINDINGS = regret_values::GAME_MODERN_BINDINGS;
        ::GAME_MODERN_BINDING_COUNT = regret_values::GAME_MODERN_BINDING_COUNT;
    }
    return true;
}();

}
