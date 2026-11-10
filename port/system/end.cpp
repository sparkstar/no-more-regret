/* See end.h. */
#include "passdump.h"
#include "end.h"
#include "gamememory.h"
#include "host.h"
#include "pc.h"

#include <cstdio>
#include <cstdlib>

namespace {

/* hostEnd's unwinding of the game's calls, which playGame catches. */
struct RunEnd {
    int code;
};
/* Whether playGame runs the game, which catches the end. */
bool playing = false;
/* hostEnd has been called. */
bool ending = false;
/* playGame runs frame, between two steps. */
bool between = false;

}

void hostEnd(int code)
{
    const SystemSide systemSide;
    passDumpEnd();
    if (!playing) {
        std::fflush(stdout);
        std::exit(code);
    }
    ending = true;
    throw RunEnd{code};
}

void hostExit(int code)
{
    const SystemSide systemSide;
    std::exit(code);
}

bool runEnding()
{
    return ending;
}

bool betweenSteps()
{
    return between;
}

int playGame(int (*start)(int, char **), void (*step)(), void (*release)(), void (*frame)(), int argc, char **argv)
{
    int code = 0;
    playing = true;
    try {
        {
            const GameSide gameSide;
            start(argc, argv);
        }
        for (;;) {
            if (clockStepEnd()) {
                between = true;
                frame();
                between = false;
            }
            const GameSide gameSide;
            step();
        }
    } catch (const RunEnd &end) {
        between = false;
        code = end.code;
    }
    playing = false;
    {
        const GameSide gameSide;
        release();
    }
    std::fflush(stdout);
    return code;
}
