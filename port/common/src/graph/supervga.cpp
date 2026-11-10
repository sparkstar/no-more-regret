/* GRAPH\SUPERVGA.C (1278:0446-0633): choosing and setting a VESA mode
 * through the platform's screen (screen.h), in the original's order. The original asks VESA's BIOS whether
 * it is there and what a mode is (INT 10h AX=4F00h, 4F01h); the port's
 * screen has the modes, and their windows, which the movies' drawing
 * steps through, are of 64K. See supervga.h. */
#include "supervga.h"
#include "screen.h"

#include <cstring>

SuperVga::SuperVga()
{
    mode = 0;
}

/* UNCALLED: nothing calls it. */
SuperVga::SuperVga(u16 videoMode)
{
    selectMode(videoMode);
}

bool SuperVga::selectMode(u16 videoMode)
{
    std::memset(&vbeInfo, 0, sizeof(vbeInfo));
    mode = videoMode;
    std::memset(&modeInfo, 0, sizeof(modeInfo));
    if (screenModeSize(mode, &modeInfo.width, &modeInfo.height)) {
        modeInfo.granularity = 64;
    }
    if (modeInfo.granularity == 64) {
        bankStep = 1;
    } else if (modeInfo.granularity != 0U) {
        bankStep = 16;
    } else {
        return false;
    }
    if (videoMode == 0U) {
        return false;
    }
    return true;
}

s16 SuperVga::width() const
{
    if (mode == 0U) {
        return -1;
    }
    return modeInfo.width;
}

s16 SuperVga::height() const
{
    if (mode == 0U) {
        return -1;
    }
    return modeInfo.height;
}

u16 SuperVga::granularity() const
{
    if (mode == 0U) {
        return 0;
    }
    return modeInfo.granularity;
}

void SuperVga::setMode() const
{
    bool status = false;

    if (mode == 0U) {
        haltMessage("Invalid SuperVga Mode");
    } else {
        status = !screenSetMode(mode, false);
    }
    if (status) {
        haltMessage(haltFormat("SVGA Mode %x is not supported.", mode));
    }
}
