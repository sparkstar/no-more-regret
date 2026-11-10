/* CRUSYS.C (Code12): the game's system object, the command line and the
 * start-up checks. The code of U9SYS.C is included in it. */
#ifndef PORT_CRUSYS_H
#define PORT_CRUSYS_H
#include "common.h"
#include "kprocess.h"
#include "system.h"

class CAFont;

/* The game's system object: it starts and shuts the game
 * down around the kernel. */
class CruSystem : public System {
public:
    /* The game's font, made at start-up. */
    CAFont *font;

    /* Shows the title screen, reads the command line and starts. */
    CruSystem(i16 argc, char **argv);
    /* Clears the text screen and writes the title, the version and the
     * copyright. */
    void showTitle();
    /* Reads the options -?, -u, -debug, -setver, -vesatest, -asylum, -warp,
     * -skill, -mapoff, -egg, -demo and No Remorse's -laurie. */
    void readCommandLine(i16 argc, char **argv);
    /* Checks the machine (No Remorse: the disk space only), opens the game's
     * data and starts the game, in steps: the constructor's call opens it,
     * each call after runs a pass of its wait of 60 ticks, the last the rest
     * (startupStarted) and the start's menu; startupRunning tells whether it
     * has more to do. */
    void startup() override;
    /* startup's after its wait: the game's data, the screen and the start's
     * menu (startGame). */
    void startupStarted();
    /* startupStarted's after the joystick, which goes on after its
     * calibration. */
    void startupScreen();
    /* Closes it all down and restores the start-up palette. */
    void shutdown() override;
};
ASSERT_SIZE(CruSystem, 6);
/* The game's system object once made (gamemain.cpp): the port's, which
 * gameRelease deletes. A run that ends while it is being made (in its
 * constructor's part of the start) has none: the object went with the
 * unwinding. */
extern CruSystem *&gameSystem;
/* Whether CruSystem::startup has more to do (gamemain.cpp's gameStep). */
[[nodiscard]] bool startupRunning();

/* The cheats are on; the cheat (No Regret's LOOSECANNON16, No Remorse's
 * JASSICA16) turns them on and off, No Remorse's -laurie option on. */
extern u8 &cheatsEnabled;
/* The command line: -warp mission [x y z], -skill, -mapoff, -egg and -demo. The mission is -1 without -warp, x -1
 * without the point, the egg -1 without -egg. */
extern i16 &warpMission;
extern i16 &warpX;
extern i16 &warpY;
extern i16 &warpZ;
extern i16 &skill;
extern i16 &mapOffset;
extern i16 &destinationEgg;
extern u8 &demoMode;
/* No Remorse's -debug: the movie player draws its ring buffer. PLAYER.C
 * reads it far. */
extern u8 &debugDisplay;
/* CPROTMEM.C probes the memory. */
extern u8 &probeMemory;
/* 1 but while the computer gump is open; a new game sets it.
 * Without it the eggs do not hatch, nothing takes damage, the keys and the
 * joystick do not move the controlled NPC (the camera moves instead) and
 * No Regret's MemProfiler does not draw. */
extern u8 &worldActive;

/* "VIOLENCE=1" in the program: its last character is '1' with
 * the violence on. crusader.cfg's violence sets it. */
extern char *&violenceSetting;
/* No Regret's "V1.06", No Remorse's "V1.21": the title screen and WORLD.C's
 * Ctrl-V show it. */
extern const char *&gameVersion;
/* crusader.cfg's crupath: kept and written back (No Remorse only). */
extern char *&cruPath;

/* The violence is on: the last character of "VIOLENCE=1" in the program is
 * '1' (intrinsic 077). */
bool violenceEnabled();
/* Declared in crusys.cpp in the original; here so that the saved games'
 * schema reaches it. */
/* Counts the timer events it gets, to see that the timer interrupt runs. */
class TickCounter : public PeriodicProcess {
public:
    explicit TickCounter(Loading loading) : PeriodicProcess(loading) {}
    i16 ticks;

    TickCounter();
    void run() override {}
    void event(u32 event, EventData data) override;
};

bool releaseBuild();
/* Nothing calls it (the original's CruSystem::startup, under a flag that is
 * always 0). It calls the palette's, the sound's and the camera's functions
 * (1058:00d4). */
/* UNCALLED: nothing calls it. */
void unknown00d4();
/* Halts without the game's CD or with too little space on the hard drive
 * (1058:016f, static); CruSystem::startup calls it. */
void checkInstall();
/* No Regret's startup got through; its shutdown closes down only then. */
extern u8 &cruStarted;
/* The palette at start-up, restored at the end. */
class Palette;
extern Palette &startPalette;
#endif
