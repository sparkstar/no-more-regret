/* No Remorse's values (values.h) for a test of a part of the port alone, as
 * the program has them when it plays No Remorse, before the other static
 * objects start. The program's choice (values.cpp) is not linked: it takes
 * No Regret's values too. No Remorse's saved objects (savegame_game.h) name
 * the weasel's visit, which a test of a part defines (wezzy.h). As in
 * values.cpp, the choice is the first object of the test. */
#include "values.h"

bool playingRemorse = false;

namespace {

class RemorseValues {
public:
    RemorseValues()
    {
        playingRemorse = true;
        useRemorseValues();
    }
};

/* Before every static object of the default priority. */
__attribute__((init_priority(101))) const RemorseValues remorseValues;

}
