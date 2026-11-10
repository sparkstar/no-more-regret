/* ASYLUM.DLL segment 8, the Sound Blaster 16's driver (ASS_Init's card 3),
 * as the platform's output (audio.h): the DMA took the buffer's first 2000h
 * bytes round and round, 16-bit samples a word at a time (1000h words; the
 * port's 8-bit precision's too, asylib.h's wordOutput) or 8-bit ones a
 * byte (2000h bytes), two units a sample of the rate, from the
 * moment the DSP had the transfer's length; the output does the same on
 * the virtual time. The DMA's count, which the fills and ASS_GetDmaBytesLeft
 * read, is the units left of a round. See asylib.h. */
#include "asylib.h"
#include "audio.h"
#include "clock.h"

namespace asylum {

namespace {

/* The DMA's units a round. */
u32 roundUnits()
{
    return wordOutput() ? 0x1000 : 0x2000;
}

}

/* 8:0050: at most 44100 Hz, stereo; the DSP's reset, whose answer the
 * driver waits 10 ms for (Borland's delay), the rate and the output. */
void startOutput()
{
    if (data.mixRate > 0xac44) {
        data.mixRate = 0xac44;
    }
    data.tickRate = data.mixRate;
    data.rate = data.mixRate;
    data.highSpeed = 0;
    data.stereo = 1;
    clockDelay(10);
    restartOutput();
}

/* The DSP at the rate, the output from the ring's start. */
void restartOutput()
{
    data.dspRate = data.mixRate;
    audioOutputStart(data.ring, roundUnits(), data.dspRate, wordOutput());
}

/* 8:0178: the output stopped. */
void stopOutput()
{
    audioOutputStop();
}

/* The DMA's count as its register reads: the units left of the round. */
u16 dmaCount()
{
    const u32 units = roundUnits();
    return static_cast<u16>(units - 1 - static_cast<u32>(audioOutputTaken() % units));
}

}
