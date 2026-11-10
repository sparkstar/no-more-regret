/* The port's entry: the original's main (gamemain.cpp's gameMain). It is
 * here, in the system layer, so that a test links the game with a main of
 * its own. */
#include "gamefile.h"
#include "commandline.h"
#include "crash.h"
#include "end.h"
#include "gamefolder.h"
#include "profile.h"
#include "rewind.h"
#include "window.h"

namespace {
/* The game's run (end.h): gameMain, then gameStep until the run ends, then
 * gameRelease however it ends, with the host's work between the frames
 * (rewind.h). */
int play(int argc, char *argv[])
{
    return playGame(gameMain, gameStep, gameRelease, hostFrame, argc, argv);
}
}

int main(int argc, char *argv[])
{
    if (portVersionAsked(argc, argv)) {
        return 0;
    }
    /* Into the folder the game's data are in (a GOG install the program was
     * copied into), drive C, where the crash reports go too. */
    enterGameFolder();
    installCrashHandler(argv[0], PROFILE_NAME, PROFILE_TITLE);
    /* The port's own options out before the game reads the rest. */
    portCommandLine(argc, argv);
    /* On the game thread when the run has a window (window.h). */
    return windowMain(play, argc, argv);
}
