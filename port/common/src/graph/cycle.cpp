/* File name unknown: CycleProcess, which cycles the game
 * palette. See cycle.h. */
#include "cycle.h"
#include "gamestate.h"
#include "screen.h"
#include "gamepal.h"
#include "kernel.h"
#include "random.h"

CycleProcess *&cycleProcess = gameGlobal<CycleProcess *>(nullptr);
/* The palette entries of the seven cycled colours, and how each changes:
 * 0 brightens, 1 brightens and adds a random amount when a channel wraps. */
i16 (&cycleTargets)[7] = gameGlobal<i16[7]>({8, 9, 10, 11, 12, 13, 14});
i16 (&cycleModes)[7] = gameGlobal<i16[7]>({0, 0, 0, 0, 0, 0, 1});
/* RGB has constructors, so these are set by the module's static constructor, not in the data. clang-format 20 breaks a
 * long line of a reference to an array of a class after its declarator (RGB (&x)\n[7]). */
/* clang-format off */
RGB (&cycleChannels)[7] = gameGlobal<RGB[7]>(
    {RGB(63, 0, 0), RGB(0, 0, 63), RGB(63, 0, 0), RGB(0, 0, 63), RGB(63, 63, 0), RGB(63, 63, 63), RGB(0, 63, 0)});
RGB (&cycleColors)[7] = gameGlobal<RGB[7]>(
    {RGB(0, 0, 0), RGB(0, 0, 0), RGB(31, 0, 0), RGB(0, 0, 31), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0)});
/* clang-format on */

namespace {
void pauseCycling()
{
    cycleProcess->paused = 1;
}

void resumeCycling()
{
    cycleProcess->paused = 0;
}
}

CycleProcess::CycleProcess()
{
    nameProcess(pid, "CycleProcess");
    setType(CYCLE_TYPE);
    setRate(30);
    pause();
    addEvents(EVENT_TIMER);
    makeSystem();
    paused = stopped = 0;
    cycleProcess = this;
    pauseCycling();
    resumeCycling();
    startTimer(1);
}

void CycleProcess::run() {}

void CycleProcess::event(u32 event, EventData /*data*/)
{
    i16 i;

    if ((event & EVENT_TIMER) == 0U) {
        return;
    }
    if ((paused != 0U) || (stopped != 0U)) {
        return;
    }
    if (keyboardPaused()) {
        return;
    }
    RGB *colors = gamePalette->data.get();
    RGB saved = colors[1];
    for (i = 2; i < 8; i++) {
        colors[static_cast<i16>(i - 1)] = colors[i];
    }
    colors[7] = saved;
    for (i = 0; i < 7; i++) {
        switch (cycleModes[i]) {
        case 0:
            saved = cycleColors[i];
            brighten(&saved, &cycleChannels[i]);
            cycleColors[i] = saved;
            colors[cycleTargets[i]] = saved;
            break;
        case 1:
            saved = cycleColors[i];
            if (brighten(&saved, &cycleChannels[i])) {
                saved.red = (saved.red + randomBelow(10)) & 0x3f;
                saved.green = (saved.green + randomBelow(10)) & 0x3f;
                saved.blue = (saved.blue + randomBelow(10)) & 0x3f;
            }
            cycleColors[i] = saved;
            colors[cycleTargets[i]] = saved;
            break;
        }
    }
    for (i = 1; i < 15; i++) {
        setColor(colors[i], i);
    }
}

void CycleProcess::setColor(RGB color, i16 index)
{
    u8 rgb[3] = {color.red, color.green, color.blue};

    screenSetColors(static_cast<u8>(index), 1, rgb);
}

bool CycleProcess::brighten(RGB *color, RGB *channels)
{
    char *value = reinterpret_cast<char *>(color);
    char *mask = reinterpret_cast<char *>(channels);
    bool wrapped = false;

    for (i16 i = 0; i < 3; i++) {
        if (mask[i] != 0) {
            value[i] = value[i] + 2;
        }
        if (value[i] > 63) {
            value[i] = 0;
            wrapped = true;
        }
    }
    return wrapped;
}

/* UNCALLED: nothing calls it. */
bool CycleProcess::darken(RGB *color, RGB *channels)
{
    char *value = reinterpret_cast<char *>(color);
    char *mask = reinterpret_cast<char *>(channels);
    bool wrapped = false;

    for (i16 i = 0; i < 3; i++) {
        if (mask[i] != 0) {
            value[i] = value[i] - 2;
            if (value[i] < 0) {
                value[i] = 63;
                wrapped = true;
            }
        }
    }
    return wrapped;
}

void CycleProcess::cleanup()
{
    Process::cleanup();
    /* No Remorse's original also clears stdIntHandler here, the standard
     * interrupt handler's pointer, not cycleProcess (its data follows
     * STDINT.C's), as StdIntHandler::cleanup does; the port leaves it, as No
     * Regret's original does. */
}
