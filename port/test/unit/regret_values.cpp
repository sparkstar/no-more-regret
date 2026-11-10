/* No Regret's values (values.h) for a test of a part of the port alone, as
 * the program has them when it plays No Regret, before the other static
 * objects start. The program's choice (values.cpp) is not linked: it takes
 * No Remorse's values too, and with them the game (No Remorse's saved
 * objects, savegame_game.h). As in values.cpp, the choice is the first object
 * of the test. */
#include "values.h"

bool playingRemorse = false;

namespace {

class RegretValues {
public:
    RegretValues() { useRegretValues(); }
};

/* Before every static object of the default priority. */
__attribute__((init_priority(101))) const RegretValues regretValues;

}
