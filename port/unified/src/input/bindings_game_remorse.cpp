/* Written by port/tools/make_unified_values.py from remorse/src/input/bindings_game.cpp: remorse's data in its
 * namespace in the one program; do not edit. */

#include "values.h"
#include "bindings.h"
#include <iterator>
#include "values_remorse.h"

namespace remorse_values {
#include "../../../remorse/src/input/bindings_game.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRemorseData_input_bindings_game();

void linkRemorseData_input_bindings_game() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (playingRemorse) {
        ::GAME_ACTION_DEFAULTS = remorse_values::GAME_ACTION_DEFAULTS;
        ::GAME_ACTION_DEFAULT_COUNT = remorse_values::GAME_ACTION_DEFAULT_COUNT;
        ::GAME_MODERN_BINDINGS = remorse_values::GAME_MODERN_BINDINGS;
        ::GAME_MODERN_BINDING_COUNT = remorse_values::GAME_MODERN_BINDING_COUNT;
    }
    return true;
}();

}
