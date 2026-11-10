/* The port without a window (window.h), which the build without SDL3 and
 * the tests have. */
#include "window.h"
#include "gamememory.h"
#include "host.h"
#include "input.h"

#include <cstdlib>

/* No window to close. */
void hostCloseAction(void (* /*unused*/)())
{
    const SystemSide systemSide;
}

/* No window over the screen: the game's wish kept for it to ask. */
namespace {
bool fullscreen = false;
}

void hostSetFullscreen(bool on)
{
    const SystemSide systemSide;
    fullscreen = on;
}

bool hostFullscreen()
{
    const SystemSide systemSide;
    return fullscreen;
}

void hostFullscreenAction(void (* /*unused*/)())
{
    const SystemSide systemSide;
}

u16 displayWideWidth()
{
    return 0;
}

/* The game on this, the one thread. */
int windowMain(int (*game)(int, char **), int argc, char **argv)
{
    return game(argc, argv);
}

bool windowRun()
{
    return false;
}

bool windowFrame()
{
    return false;
}

bool windowFrameUnshown()
{
    return false;
}

bool windowPicture(std::vector<u8> & /*unused*/, int & /*unused*/, int & /*unused*/)
{
    return false;
}

void windowAudio(const u8 * /*unused*/, u32 /*unused*/, u32 /*unused*/, bool /*unused*/) {}

bool windowAudioState(WindowAudio & /*unused*/)
{
    return false;
}

void windowMessage(const char * /*unused*/) {}

void windowWarning(const char * /*unused*/) {}

void windowTitleNote(const std::string & /*unused*/) {}

void windowCall(const std::function<void()> &function)
{
    function();
}

void windowExit(int code)
{
    std::exit(code);
}

/* No window: no host actions' keys, no rewind (rewind.h's test runs one
 * itself). */
void inputHostKeys(HostAction /*unused*/, const HostKey * /*unused*/, std::size_t /*unused*/)
{
    const SystemSide systemSide;
}

bool windowRewinding()
{
    return false;
}

bool windowFastForwarding()
{
    return false;
}

bool windowPaused()
{
    return false;
}

bool windowPauseFrame()
{
    return false;
}

int windowStateAsked()
{
    return 0;
}

bool windowTakeOverAsked()
{
    return false;
}

KeysHeld windowKeysTaken()
{
    return {};
}

bool windowRewindFrame()
{
    return true;
}

void windowRewindEnd(const KeysHeld & /*unused*/) {}
