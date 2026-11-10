/* Code11 (1050:0073-00a5, file name unknown): the module of main, with the
 * game's directory and file names; the program's name and the copyright's
 * year are the game's (gamemain_game.h). */
#include "gamefile.h"
#include "gamestate.h"
#include "crusys.h"
#include "kernel.h"
#include "waits.h"
#include "gamemain_game.h"

char (&staticDirectory)[9] = gameGlobal<char[9]>("static");
char (&savegameDirectory)[9] = gameGlobal<char[9]>("savegame");
char (&soundDirectory)[9] = gameGlobal<char[9]>("sound");
char (&editorDirectory)[9] = gameGlobal<char[9]>("editor");
char (&usecodeDirectory)[80] = gameGlobal<char[80]>("usecode");
namespace {
/* The game writes into the drive and into the usecode's name (crusader.cfg's
 * cdletter, the language's letter), which lie in DGROUP in the original;
 * the port's literals are read-only, so these two are arrays. */
char (&cdDrive)[3] = gameGlobal<char[3]>("d:");
char (&usecodeFile)[13] = gameGlobal<char[13]>("eusecode.flx");
}
char *&gameDrive = gameGlobal<char *>(cdDrive);
char *&flicPath = gameGlobal<char *>(nullptr);
const char *(&gameFiles)[17] = gameGlobal<const char *[17]>({"shapes.flx", "typename.dat", "fixed.dat", "nonfixed.dat",
                                                             "glob.flx", "xformpal.dat", "music.flx", "gumps.flx",
                                                             "fonts.flx", usecodeFile, "anim.dat", "mapnames.dat",
                                                             "eggfile.flx", "gamepal.pal", "typeflag.dat", "unkds.dat",
                                                             GAME_PROGRAM_NAME});
const char *(&copyright)[2] = gameGlobal<const char *[2]>({COPYRIGHT_LINE, "All Rights Reserved."});
/* No Remorse's two after the copyright (1478:06fe, 0702), which no code
 * reads by their address. */
const char *&wimpMessage = gameGlobal<const char *>("Run you wimp!\r\n");
char (&unknown702)[56] = gameGlobal<char[56]>({0});

namespace {
/* The kernel calls it between its runs. The original adds the samples
 * SPANKER.ASM copied to its profile there (updateProfile); the port has no
 * profile. */
void idle() {}
}

/* The original's main, up to its kernel's loop, which gameStep runs a pass
 * of at a time; the port's main is in port/system/main.cpp, so that a test
 * can link the game with a main of its own. */
int gameMain(int argc, char *argv[])
{
    tableGrows = 1;
    gameSystem = new CruSystem(argc, argv);
    return 0;
}

namespace {
/* The step's work when no wait is open: the start's part, else a pass of
 * the kernel. */
void stepBase()
{
    if (startupRunning()) {
        gameSystem->startup();
        return;
    }
    runKernel(0, idle);
}
}

void gameStep()
{
    runGameStep(stepBase);
}
