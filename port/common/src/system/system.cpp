/* SYSTEM\SYSTEM.C: the system object and the video mode, in the original's
 * order, over the platform's screen and input (screen.h, input.h). The
 * original starts and stops SPANKER.C's
 * interrupt stubs, which the port, without interrupts, has no use for; its
 * FileHandles (70 handles) is the platform's files' table (files.h). See
 * system.h. */
#include "system.h"
#include "gamestate.h"
#include "host.h"
#include "input.h"
#include "screen.h"

System *&currentSystem = gameGlobal<System *>(nullptr);
namespace {
/* The video mode at start-up and now: the mode in the low word, the font
 * height in the high word (getVideoMode). */
u32 &originalMode = gameGlobal<u32>(0x11140003L);
u32 &currentMode = gameGlobal<u32>(0x11140003L);
}
u8 &systemStarted = gameGlobal<u8>(0);
namespace {
u8 &shutDown = gameGlobal<u8>(0);
}
char *&formatBuffer = gameGlobal<char *>(nullptr);
SuperVga &superVga = gameGlobal<SuperVga>();
namespace {
/* Closing the window (host.h) ends the run at once with the original's
 * exit code, 1, as the script's quit ends it: not through systemShutdown,
 * whose teardown the original runs only where it ends itself, its modal
 * gumps closed first (the main menu's QUIT, Alt+X). Run from a window
 * frame, in the middle of a modal gump or a movie, it would delete the
 * gumps while they are open: a terminal's ComputerGump repaints in its
 * destructor, after deleting its text. The run's end
 * unwinds the game's calls first, and the game then releases what it holds
 * (gameRelease). */
const bool CLOSE_ACTION = (hostCloseAction([] { hostEnd(1); }), true);
}

void System::startup()
{
    if (systemStarted != 0U) {
        return;
    }
    systemStarted = 1;
    /* No Remorse's original does not reset the mouse driver here. */
    inputMouseReset();
    originalMode = currentMode = getVideoMode();
    currentSystem = this;
    if (formatBuffer == nullptr) {
        formatBuffer = new char[FORMAT_BUFFER_SIZE];
    }
}

bool System::setMode(u32 mode)
{
    currentMode = mode;
    if (static_cast<u16>(mode) == VESA_640X480) {
        if (!superVga.selectMode(VESA_640X480)) {
            return false;
        }
        superVga.setMode();
    } else if (static_cast<u16>(mode) == VESA_640X480_64K) {
        if (!superVga.selectMode(VESA_640X480_64K)) {
            return false;
        }
        superVga.setMode();
    } else if (static_cast<u16>(mode) == VESA_800X600) {
        if (!superVga.selectMode(VESA_800X600)) {
            return false;
        }
        superVga.setMode();
    } else if (static_cast<u16>(mode) == VESA_1024X768) {
        if (!superVga.selectMode(VESA_1024X768)) {
            return false;
        }
        superVga.setMode();
    } else if (static_cast<u16>(mode) == MODE_TEXT) {
        screenSetMode(3, false);
    } else {
        /* A BIOS mode, whose bit 7 keeps the screen (INT 10h AH=0). */
        screenSetMode(static_cast<u8>(mode) & 0x7f, (static_cast<u8>(mode) & 0x80) != 0);
    }
    return true;
}

u32 getVideoMode()
{
    i16 mode;
    i16 height;

    mode = static_cast<i16>(screenMode());
    height = static_cast<i16>(screenFontHeight());
    return (static_cast<s32>(height) << 16) | mode;
}

void System::shutdown() {}

void systemShutdown(u8 exitProgram)
{
    System *system;

    if (shutDown == 0U) {
        shutDown = 1;
        if (currentSystem != nullptr) {
            system = currentSystem;
            currentSystem = nullptr;
            if (currentMode != originalMode) {
                system->setMode(originalMode);
            }
            system->shutdown();
        }
        if (exitProgram != 0U) {
            hostEnd(1);
        }
    }
}
