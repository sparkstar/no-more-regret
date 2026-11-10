/* CRUSYS.C (Code12, No Regret's 1058, No Remorse's 1048; the addresses are
 * No Regret's): the game's system object, the games' differences as each
 * game's values (crusys_game.h). The code of U9SYS.C is included in it. */
#include <conio.h>
#include "gamestate.h"
#include <crash.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "crusys.h"
#include "crusys_game.h"
#include "actanim.h"
#include "anchor.h"
#include "attack.h"
#include "cache.h"
#include "cafont.h"
#include "camera.h"
#include "control.h"
#include "cycle.h"
#include "dispatch.h"
#include "display.h"
#include "dlist.h"
#include "dsfxman.h"
#include "dtable.h"
#include "dumbtimr.h"
#include "fade.h"
#include "filespec.h"
#include "flag.h"
#include "gamefile.h"
#include "gamepal.h"
#include "gametime.h"
#include "globs.h"
#include "gumpfont.h"
#include "gumpshap.h"
#include "handler.h"
#include "itemcach.h"
#include "itemdam.h"
#include "joydrv.h"
#include "kernel.h"
#include "mapjump.h"
#include "diskfree.h"
#include "mousectl.h"
#include "music.h"
#include "npc.h"
#include "options.h"
#include "palette.h"
#include "pfloat.h"
#include "savegame.h"
#include "shaphand.h"
#include "snap.h"
#include "spmgr.h"
#include "status.h"
#include "systimer.h"
#include "type.h"
#include "u9mouse.h"
#include "uprocess.h"
#include "utility.h"
#include "viewport.h"
#include "world.h"
#include "wpncache.h"
#include "tracker.h"
#include "files.h"
#include "input.h"
#include "host.h"
#include "widesides.h"
#include "screen.h"

ASSERT_SIZE(TickCounter, 0x42);

u8 &probeMemory = gameGlobal<u8>(1);

u8 &worldActive = gameGlobal<u8>(1);
namespace {
const char *gameCompile = "GAME COMPILE=1";
/* crusader.cfg's violence setting writes into it: an array, as the port's
 * literals are read-only. */
char (&violence)[11] = gameGlobal<char[11]>("VIOLENCE=1");
}
char *&violenceSetting = gameGlobal<char *>(violence);

/* No Regret's startup got through; its shutdown closes down only then
 * (crusys.h). */
u8 &cruStarted = gameGlobal<u8>(0);
/* The palette at start-up, restored at the end (crusys.h). */
Palette &startPalette = gameGlobal<Palette>();
namespace {
/* -debug sets it; nothing reads it. */
u8 debugMode = 0;
}
/* -debug sets it: No Remorse's movie player showed its buffer with it, the
 * port's reads nothing (No Regret's unknown0ad5, which nothing read). */
u8 &debugDisplay = gameGlobal<u8>(0);
/* The command line options; "DONE" ends the list. */
/* The command line's options, by their place in commandOptions. */
constexpr int ARG_HELP = 0;
constexpr int ARG_USECODE = 1;
constexpr int ARG_DEBUG = 2;
constexpr int ARG_SETVER = 3;
constexpr int ARG_VESATEST = 4;
constexpr int ARG_ASYLUM = 5;
constexpr int ARG_WARP = 6;
constexpr int ARG_SKILL = 7;
constexpr int ARG_MAPOFF = 8;
constexpr int ARG_EGG = 9;
constexpr int ARG_DEMO = 10;
namespace {
const char *commandOptions[] = {"-?",    "-u",     "-debug",  "-setver", "-vesatest", "-asylum",
                                "-warp", "-skill", "-mapoff", "-egg",    "-demo",     "DONE"};
}

CruSystem *&gameSystem = gameGlobal<CruSystem *>(nullptr);
/* Where CruSystem::startup is: the original's ran it all at once, waiting
 * its 60 ticks in a loop of the kernel's passes; the port's opens the game
 * when the constructor calls it, then runs a pass of the wait at each call
 * of the game's steps (gamemain.cpp's gameStep), then the rest and the
 * start's menu. */
enum class StartupStage : u8 { OPEN, TICKS, DONE };
namespace {
StartupStage &startupStage = gameGlobal<StartupStage>(StartupStage::OPEN);
/* The wait's first tick and its counter (the original's locals). */
u32 &startupStart = gameGlobal<u32>(0);
TickCounter *&startupCounter = gameGlobal<TickCounter *>(nullptr);
}
u8 &cheatsEnabled = gameGlobal<u8>(0);
/* The title CruSystem::CruSystem sets: GAME_TITLE. */
const char *&gameTitle = gameGlobal<const char *>(nullptr);
i16 &warpMission = gameGlobal<i16>(-1);
i16 &warpX = gameGlobal<i16>(-1);
i16 &warpY = gameGlobal<i16>(-1);
i16 &warpZ = gameGlobal<i16>(-1);
i16 &skill = gameGlobal<i16>(-1);
i16 &mapOffset = gameGlobal<i16>(0);
i16 &destinationEgg = gameGlobal<i16>(-1);
u8 &demoMode = gameGlobal<u8>(0);
namespace {
/* No Regret's: nothing reads it. */
[[maybe_unused]] u32 unknown0ad8 = 0;
}
/* crusader.cfg's crupath (No Remorse's, crusys.h). */
char *&cruPath = gameGlobal<char *>(nullptr);
const char *&gameVersion = gameGlobal<const char *>(GAME_VERSION);

/* Halts without the CD (No Regret's: MISSING_CD_MESSAGE), or with too little
 * space on the hard drive: 5 MB with every shape on it, else 15. */
void checkInstall()
{
    if (MISSING_CD_MESSAGE != nullptr) {
        char *path = makePath(gameDrive, staticDirectory, "FIXED.DAT", nullptr);

        if (fileExists(path) == 0) {
            currentSystem = nullptr;
            haltMessage(MISSING_CD_MESSAGE);
        }
    }
    if (allShapes != 0U) {
        if (diskFree() < static_cast<s32>(5000000L)) {
            currentSystem = nullptr;
            haltMessage("\n\rYou need to have at least 5 Megs of free hard drive space.\n\r");
        }
    } else if (diskFree() < static_cast<s32>(16000000L)) {
        currentSystem = nullptr;
        haltMessage("\n\rYou need to have at least 15 Megs of free hard drive space.\n\r");
    }
}

TickCounter::TickCounter()
{
    ticks = 0;
    setRate(20);
    startTimer(1);
    eventMask |= EVENT_TIMER;
}

void TickCounter::event(u32 event, EventData /*data*/)
{
    switch (event) {
    case EVENT_TIMER:
        ticks++;
    }
}

bool violenceEnabled()
{
    const u8 setting = violenceSetting[9];

    return setting == '1';
}

/* The last character of "GAME COMPILE=1" in the program is '1' (1058:00c1,
 * intrinsic 056). */
bool releaseBuild()
{
    return gameCompile[13] == '1';
}

/* The title is GAME_TITLE, each game's. */
CruSystem::CruSystem(i16 argc, char **argv)
{
    gameTitle = GAME_TITLE;
    showTitle();
    readCommandLine(argc, argv);
    startup();
}

namespace {
/* DOS 3Ch: creates path; its handle (the platform's file), or -1. */
i16 createFile(char *path)
{
    const FileResult result = fileCreate(path);
    if (result.error != 0) {
        return -1;
    }
    return static_cast<i16>(result.value);
}

void closeFile(i16 handle)
{
    fileClose(handle);
}

/* Halts when DOS cannot open 20 files: creates FHACK0, FHACK1, ... until it
 * fails, then closes and removes them. The table holds 100 handles and is
 * not checked: the platform's files run out at 65 (files.h), as DOS's. */
void checkFiles()
{
    i16 *handles = new i16[100];
    i16 count = 0;
    i16 i;
    char name[32];

    memset(handles, 0, 200);
    for (;;) {
        snprintf(name, sizeof(name), "FHACK%d", count);
        const i16 handle = createFile(name);
        if (handle < 0) {
            break;
        }
        handles[count++] = handle;
    }
    for (i = 0; i < count; i++) {
        closeFile(handles[i]);
        snprintf(name, sizeof(name), "FHACK%d", i);
        fileRemove(name);
    }
    delete[] handles;
    if (count < 20) {
        currentSystem = nullptr;
        haltMessage("\n\rYou need to have the line FILES=25 in your CONFIG.SYS\n\r");
    }
}
}

bool startupRunning()
{
    return startupStage != StartupStage::DONE;
}

void CruSystem::startup()
{
    if (startupStage == StartupStage::DONE) {
        return;
    }
    if (startupStage == StartupStage::TICKS) {
        if (systemTimer->ticks <= startupStart + 60) {
            runProcesses();
            return;
        }
        startupStarted();
        return;
    }
    readOptions();
    checkInstall();
    if (START_CHECKS_FILES) {
        checkFiles();
        if (isChristmas()) {
            debugPrintf(0xff, "-----------------------------------------------------------\n\r");
            debugPrintf(0xff, "Seasons greetings!\n\r\n\r");
            debugPrintf(0xff, "Press CTRL-M anytime during the game to disable Xmas mode.\n\r\n\r");
            debugPrintf(0xff, "    - the Crusader Team\n\r");
            debugPrintf(0xff, "-----------------------------------------------------------\n\r\n\r");
        }
        System::startup();
        initKernel();
    }
    if (fileExists("static\\fixed.dat") != 0) {
        debugPrintf(0xff, "Using map patch file.\n\r");
    }
    debugPrintf(0xff, "Loading: [");
    debugPrintf(0xff, "           ]\r");
    debugPrintf(0xff, "Loading: [");
    debugPrintf(0xff, "%s", LOADING_STEP);
    if (!START_CHECKS_FILES) {
        System::startup();
        initKernel();
    }
    /* The original loads XANDRV32.DLL here and halts without it (No
     * Remorse's sets up its drawing code's 32-bit segment, init32Code); the
     * port's movie decoder is its own (port/common/xan), and it draws
     * through pointers. */
    drawViewport = nullptr;
    screenViewport = nullptr;
    font = nullptr;
    loadTrigTables(makePath(nullptr, staticDirectory, "trig", "dat"));
    startupStart = systemTimer->ticks;
    startupCounter = new TickCounter;
    startupStage = StartupStage::TICKS;
}

/* CruSystem::startup's after its wait of 60 ticks. */
void CruSystem::startupStarted()
{
    if (startupCounter->ticks < 10) {
        hostPrint("DOS interrupt error - check the install guide for more info.\n\r");
        crashReport("DOS interrupt error - check the install guide for more info.");
        currentSystem = nullptr;
        exitKernel();
        systemShutdown(1);
    }
    startupCounter->terminate(0);
    startupCounter = nullptr;
    debugPrintf(0xff, "%s", LOADING_STEP);
    svgaScreen = new Viewport(VIEWPORT_LARGE | VIEWPORT_SCREEN, 0, 0, SCREEN_RIGHT, SCREEN_BOTTOM);
    screenViewport = svgaScreen;
    drawViewport = new Viewport(VIEWPORT_LARGE, 0, 0, SCREEN_RIGHT, SCREEN_BOTTOM);
    drawViewport->clear(0);
    debugPrintf(0xff, "%s", LOADING_STEP);
    initCache(3000000L);
    initCacheHandlers();
    typeFlags.init();
    new DamageTable;
    initShapeCache();
    createGlobHandler();
    debugPrintf(0xff, "%s", LOADING_STEP);
    initUsecode();
    loadFlags(nullptr, 0);
    debugPrintf(0xff, "%s", LOADING_STEP);
    new SnapProcess;
    initDisplayList();
    startItemCache();
    allocateNpcs();
    loadDtable();
    initSprites();
    initPickups();
    loadDescriptions();
    debugPrintf(0xff, "%s", LOADING_STEP);
    initAnims();
    debugPrintf(0xff, "%s", LOADING_STEP);
    initFonts(makePath(nullptr, staticDirectory, gameFiles[GAME_FONTS], nullptr));
    font = new CAFont(0);
    setFont(font);
    debugPrintf(0xff, "%s", LOADING_STEP);
    createCamera();
    debugPrintf(0xff, "%s", LOADING_STEP);
    new U9MouseControlProcess;
    debugPrintf(0xff, "%s", LOADING_STEP);
    initGumpShapes(makePath(nullptr, staticDirectory, "GUMPS.FLX", nullptr));
    initDispatcher();
    debugPrintf(0xff, "%s\n\r", LOADING_STEP);
    startSoundEffects();
    enableMusic();
    cruStarted = 1;
    startGameTime();
    loadPalettes();
    guiDispatcher->pushModal(new WorldGump);
    new Controller;
    if (ANCHOR_PROCESS) {
        new AnchorProcess;
    }
    new DumbTimer;
    initStatus();
    setupJoystick();
    if (waitOpened()) {
        /* The joystick's calibration first. */
        giveRest([this]() {
            startupScreen();
            return false;
        });
        return;
    }
    startupScreen();
}

/* startupStarted's after the joystick: the screen, the mouse and the start's
 * menu. */
void CruSystem::startupScreen()
{
    startPalette.capture();
    if (!setMode(VESA_640X480)) {
        haltMessage("Unable to switch to VESA 640x480x256");
    }
    mouseCtl->setLimits(0x101);
    mouseCtl->show();
    mouseCtl->pushShape(STARTUP_POINTER);
    gamePalette->apply();
    new CycleProcess;
    inputMouseXRange(0, screenViewport->right() * 2);
    inputMouseYRange(0, screenViewport->bottom());
    mouseCtl->popShape();
    startupStage = StartupStage::DONE;
    startGame();
}

void CruSystem::showTitle()
{
    i16 i;
    i16 x;

    hostTextBackground(BLACK);
    hostTextColor(LIGHTGRAY);
    hostTextMode(C80);
    hostTextMode(C4350);
    hostClearText();
    hostTextBackground(TITLE_BAR_COLOUR);
    x = static_cast<ui16>(80 - (strlen(gameTitle) + strlen(gameVersion) + 1)) / 2;
    hostTextAt(1, 1);
    for (i = 0; i < 80; i++) {
        hostPrintText(" ");
    }
    hostTextAt(x + 1, 1);
    hostTextColor(YELLOW);
    if (!violenceEnabled()) {
        if (TITLE_SPACED) {
            hostPrintText("%s %sNV", gameTitle, gameVersion);
        } else {
            hostPrintText("%s%sNV", gameTitle, gameVersion);
        }
    } else if (TITLE_SPACED) {
        hostPrintText("%s %s", gameTitle, gameVersion);
    } else {
        hostPrintText("%s%s", gameTitle, gameVersion);
    }
    hostTextBackground(BLACK);
    if (PRODUCTION_LINE) {
        x = static_cast<ui16>(80 - strlen(PRODUCTION)) / 2;
        hostTextAt(x + 1, 42);
        hostTextColor(RED);
        hostPrintText("%s", PRODUCTION);
        hostTextColor(LIGHTGRAY);
    }
    x = static_cast<ui16>(80 - strlen(copyright[0])) / 2;
    hostTextAt(x + 1, 43);
    hostTextColor(COPYRIGHT_COLOUR);
    hostPrintText("%s", copyright[0]);
    hostTextColor(LIGHTGRAY);
    hostTextAt(1, 2);
    hostPrint("\r\n");
}

void CruSystem::shutdown()
{
    Process *process;

    if (SHUTDOWN_AFTER_START && cruStarted == 0U) {
        return;
    }
    stopMusic();
    disableMouseEvents();
    freeTrigTables();
    unknown07d2();
    closeDispatcher();
    closeSoundEffects();
    disableMusic();
    closeGumpShapes();
    if (font != nullptr) {
        delete font;
        font = nullptr;
        /* The port's: setFont's globals are the font (gameRelease). */
        systemFont = gumpFont = nullptr;
    }
    closeAnims();
    closeSprites();
    freeDtable();
    freeNpcs();
    deleteGlobHandler();
    closeShapeCache();
    typeFlags.close();
    closeCache();
    closeCacheHandlers();
    jumpToBlack();
    if (drawViewport != nullptr) {
        delete drawViewport;
        drawViewport = nullptr;
    }
    if (svgaScreen != nullptr) {
        delete svgaScreen;
        svgaScreen = nullptr;
    }
    if (vgaScreen != nullptr) {
        delete vgaScreen;
        vgaScreen = nullptr;
    }
    debugPrintf(0xff, "%s", cheatsEnabled != 0U ? GOODBYE_WITH_CHEATS : GOODBYE);
    if (SHUTDOWN_ENDS_FADES) {
        /* Waits for nothing: it never reads the process again, so with the
         * flag set it never ends. */
        process = firstProcess(0, FADE_TYPE);
        while ((process != nullptr) && (process->hasFlags(PROCESS_ACTIVE) != 0U)) {
            ;
        }
        killItemProcesses(0, FADE_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    }
    fadeToPaletteAndWait(&startPalette, 40);
    exitKernel();
    if (SHUTDOWN_ENDS_FADES) {
        while (inputKeyWaiting()) {
            inputKey();
        }
    }
    System::shutdown();
}

namespace {
/* gameRelease's first part, while the kernel is there: what the original's
 * shutdown closes, in its order, without what it shows or waits for (the
 * palette, the fade, the goodbye, the video mode). Each close does nothing
 * for what is closed already, so that a shutdown that stopped on its way
 * (the run ended in its fade) is finished. */
void releaseRunning()
{
    stopMusic();
    disableMouseEvents();
    freeTrigTables();
    closeDispatcher();
    closeSoundEffects();
    disableMusic();
    closeGumpShapes();
    /* The system object's font, which setFont made the globals' (the
     * object may be gone: gameSystem). */
    if (systemFont != nullptr) {
        delete systemFont;
        systemFont = gumpFont = nullptr;
    }
    closeAnims();
    closeSprites();
    freeDtable();
    /* freeDtable frees nothing, its test the wrong way round as the
     * original's. */
    if (npcTypes != nullptr) {
        delete[] npcTypes;
        npcTypes = nullptr;
    }
    freeNpcs();
    deleteGlobHandler();
    closeShapeCache();
    typeFlags.close();
    closeCache();
    closeCacheHandlers();
    if (drawViewport != nullptr) {
        delete drawViewport;
        drawViewport = nullptr;
    }
    if (svgaScreen != nullptr) {
        delete svgaScreen;
        svgaScreen = nullptr;
    }
    if (vgaScreen != nullptr) {
        delete vgaScreen;
        vgaScreen = nullptr;
    }
    screenViewport = nullptr;
    exitKernel();
}
}

/* The port's release of what the game holds at the run's end
 * (gamefile.h): after the original's shutdown when the game ended itself
 * (systemShutdown), else alone, when the run ended from outside the game
 * (the window closed, the script's quit, the replay's end). The
 * original's shutdown ends with exitKernel: while the kernel is there, it
 * did not run or did not finish (releaseRunning). After exitKernel the
 * globals that point to processes point to freed ones (mouseCtl): the
 * rest touches no process. */
void gameRelease()
{
    /* The waits left open, their gumps' states first (waits.h), and a map's
     * loading they stopped. */
    releaseWaits();
    releaseMapLoad();
    if (kernelStarted != 0U) {
        releaseRunning();
    }
    /* What the original's shutdown leaves, after the processes (a usecode
     * process gives its memory back as it goes), by the original's close
     * functions that nothing calls. */
    closeMusic();
    closeOverlays();
    freePalettes();
    closeFonts();
    closeTracker();
    closeItemCache();
    closeDisplayList2();
    freeFlags();
    closeUsecode();
    /* And what the original has no close for (the port's). */
    closeStatus();
    closeCursor();
    freeDescriptions();
    closePickups();
    closeWideSides();
    freeTactics();
    closeShapeFiles();
    closeDispatcherEvent();
    delete damageTable;
    freePaths();
    delete[] flicPath;
    flicPath = nullptr;
    delete[] formatBuffer;
    formatBuffer = nullptr;
    currentSystem = nullptr;
    if (gameSystem != nullptr) {
        gameSystem->font = nullptr;
        delete gameSystem;
        gameSystem = nullptr;
    }
    /* The start's counter went with the kernel (releaseRunning). */
    startupStage = StartupStage::OPEN;
    startupCounter = nullptr;
}

/* UNCALLED: nothing calls it. */
void unknown00d4()
{
    releaseBuild();
    violenceEnabled();
    jumpToBlack();
    jumpToGreyScale();
    jumpToWhite();
    jumpToColor(0, 0, 0);
    fadeOut();
    fadeToBlack(0);
    fadeIn();
    fadeToGamePalette(0);
    fadeToGreyScale(0);
    fadeFromGreyScale(0);
    fadeTowardGamePalette(0, 0);
    fadeTowardColor(0, 0, 0, 0, 0);
    fadeTowardBlack(0, 0);
    fadeToGamePalette(0);
    jumpToGamePalette();
    pauseEffects();
    resumeEffects();
    repaintCamera();
}

namespace {
/* An option that starts with "-laurie" turns the cheats on (1048:095c). The
 * command line gives it every option it does not know, and the null pointer
 * after the last argument when an option took it. */
void checkCheatOption(char *option)
{
    char *s = option;

    if ((s != nullptr) && s[0] == '-' && s[1] == 'l' && s[2] == 'a' && s[3] == 'u' && s[4] == 'r' && s[5] == 'i' &&
        s[6] == 'e') {
        cheatsEnabled = 1;
        debugPrintf(50, "Cheats are active.\n\r");
    }
}
}

void CruSystem::readCommandLine(i16 argc, char **argv)
{
    i16 count;
    i16 i;
    u16 option;

    if (argc < 2) {
        return;
    }
    count = argc;
    i = 1;
    if (i < count) {
        do {
            for (option = 0; stricmp(commandOptions[option], "DONE") != 0; option++) {
                if (stricmp(commandOptions[option], argv[i]) == 0) {
                    i++;
                    switch (option) {
                    case ARG_HELP:
                        if (HELP_MESSAGE != nullptr) {
                            debugPrintf(0xff, "%s", HELP_MESSAGE);
                        }
                        currentSystem = nullptr;
                        systemShutdown(1);
                        break;
                    case ARG_USECODE:
                        /* Up to 30 characters of the 80-character path. */
                        strncpy(usecodeDirectory, argv[i++], 30);
                        break;
                    case ARG_DEBUG:
                        debugLevel = 10;
                        debugPrintf(50, "Debugging mode ON.\n\r");
                        debugMode = 1;
                        debugDisplay = 1;
                        break;
                    case ARG_SETVER:
                        currentSystem = nullptr;
                        systemShutdown(1);
                        break;
                    case ARG_VESATEST:
                        break;
                    case ARG_ASYLUM:
                        debugPrintf(0xff, "Enabling ENHANCED mode. (NOT!)\r\n");
                        break;
                    case ARG_WARP:
                        warpX = -1;
                        warpMission = atoi(argv[i++]);
                        if ((argv[i] != nullptr) && argv[i][0] != '-') {
                            warpX = atoi(argv[i++]);
                            warpY = atoi(argv[i++]);
                            warpZ = atoi(argv[i++]);
                            debugPrintf(0xff, "Warping to mission %d @ x:%d y:%d z:%d.\r\n", warpMission, warpX, warpY,
                                        warpZ);
                        } else {
                            debugPrintf(0xff, "Warping to mission %d.\r\n", warpMission);
                        }
                        break;
                    case ARG_SKILL:
                        skill = atoi(argv[i++]);
                        if (skill == 0) {
                            skill = 1;
                        }
                        debugPrintf(0xff, "Defaulting to skill level %d\n\r", skill);
                        break;
                    case ARG_MAPOFF:
                        mapOffset = atoi(argv[i++]);
                        debugPrintf(0xff, "Map offset = %d\n\r", mapOffset);
                        break;
                    case ARG_EGG:
                        destinationEgg = atoi(argv[i++]);
                        debugPrintf(0xff, "Destination Egg = %d\n\r", destinationEgg);
                        break;
                    case ARG_DEMO:
                        demoMode = 1;
                        debugPrintf(0xff, "Demo mode.\n\r");
                        break;
                    }
                    option = 0;
                }
                if (i >= count) {
                    break;
                }
            }
            if (CHEAT_OPTION) {
                checkCheatOption(argv[i]);
            }
            i++;
        } while (i < count);
    }
}
