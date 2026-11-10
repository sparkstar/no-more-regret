/* The one program's choice of the game's values (values.h): before the other
 * static objects start, the game CRUSADER_GAME names (regret, remorse, or
 * the tools' noremorse), else the game of the folder the data are in
 * (files.h's fileFolderGame: a GOG install), else the last one played
 * (fileLastGame: enhanced.cfg's game=, in the directory the program runs
 * in), else No Remorse, the series' first. --game and a
 * recording's game run the program again for theirs (system/restart.h). */
#include "values.h"
#include "files.h"
#include "host.h"

#include <string>

bool playingRemorse = false;

namespace {

class GameValues {
public:
    GameValues()
    {
        const char *asked = hostEnvironment("CRUSADER_GAME");
        const std::string folder = fileFolderGame();
        std::string game;
        if (asked != nullptr) {
            game = asked;
        } else if (!folder.empty()) {
            game = folder;
        } else {
            game = fileLastGame();
        }
        if (game == "regret") {
            useRegretValues();
        } else {
            playingRemorse = true;
            useRemorseValues();
        }
    }
};

/* Before every static object of the default priority. */
__attribute__((init_priority(101))) const GameValues gameValues;

}
