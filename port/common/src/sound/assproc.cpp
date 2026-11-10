/* File name unknown: ASYLUM.DLL and its process. See
 * assproc.h. */
#include "assproc.h"
#include "gamestate.h"
#include "asylum.h"
#include "dsfxman.h"
#include "kernel.h"
#include "sndmgr.h"

u16 &sampleRate = gameGlobal<u16>(22050);
u8 &asylumStarted = gameGlobal<u8>(0);
u16 &soundVolume = gameGlobal<u16>(64);
u16 &musicVolume = gameGlobal<u16>(32);
u8 &oversampling = gameGlobal<u8>(1);
u8 &assRunning = gameGlobal<u8>(0);
/* The names of the cards (the configuration picks one); 0 ends the list. */
const char *(&cardNames)[8] = gameGlobal<const char *[8]>({"none", "sb", "sbpro", "sb16", "awe32", "pas", "gus",
                                                           nullptr});
u8 &soundEnabled = gameGlobal<u8>();
u8 &musicOn = gameGlobal<u8>();

void startASSProcess()
{
    if ((effectsStarted != 0U) && (assRunning == 0U)) {
        new ASSProcess;
    }
}

ASSProcess::ASSProcess()
{
    nameProcess(pid, "ASSProcess");
    /* The originals take 90 a second up to 32 kHz (No Remorse's at any rate);
     * the port takes 60. At 90 the kernel's timer would run at 90 Hz and
     * SystemTimer's 60 Hz ticks come two in one of the port's passes (a
     * sixtieth of a second each) and none in the next: the usecode's waits,
     * which count the passes the tick changed in (0A1D:003B), would take
     * twice as long (the spider bomb, the boxes, the doors). The fill follows the
     * emulated DMA's place, not the calls: 60 a second is far more often than
     * a half of the ring (1024 frames, 46 ms at 22 kHz) plays. */
    if (sampleRate > 32000) {
        setRate(120);
    } else {
        setRate(60);
    }
    pause();
    setPriority(ALWAYS_LEVEL);
    addEvents(EVENT_TIMER);
    makeSystem();
    startTimer(1);
    assRunning = 1;
}

void ASSProcess::cleanup()
{
    Process::cleanup();
    assRunning = 0;
}

void ASSProcess::load(File *file)
{
    Process::load(file);
    assRunning = 1;
}

void ASSProcess::run()
{
    /* A test with nothing left to do: event updates the card. */
    if (asylumStarted != 0U) {
        /* empty in the original */
    }
}

void ASSProcess::event(u32 event, EventData /*data*/)
{
    if ((event & EVENT_TIMER) == 0U) {
        return;
    }
    if (asylumStarted != 0U) {
        ASS_UpdateDAC();
    }
}

/* The last function before ASYLUM.DLL's hook: empty, and nothing calls it. */
/* UNCALLED: nothing calls it. */
void unusedAmf() {}

/* The original's hookVector followed, which ASYLUM.DLL called to hook the
 * card's interrupt (SPANKER.C), and the vector's old handlers, which
 * closeAsylum put back: the port's sound has no interrupt. */

void initAsylum(u16 port, u16 irq, u16 dma, u16 card, u16 /*unused*/)
{
    if (asylumStarted == 0U) {
        debugPrintf(50, ".");
        const i16 type = card;
        AssConfig config;

        /* No Regret's original also asked the card's name for a message its
         * verbose, which nothing sets, would print. */
        config.card = type;
        config.port = port;
        config.irq = irq;
        config.dma = dma;
        config.rate = sampleRate;
        config.unknown0c = 1;
        config.unknown0a = 1;
        /* The port's depth: none chosen, crusader.cfg's DMA channel's, set
         * as the port's so that an 8-bit DMA plays the 8-bit mix at the
         * 16-bit output's level (the module's depth 0, the DMA's output as
         * it was, is its oracle's). */
        if (ASS_GetDepth() == 0) {
            ASS_SetDepth(static_cast<s16>(dma) >= 4 ? 16 : 8);
        }
        ASS_Init(&config);
        ASS_SetMusicVolume(musicVolume);
        ASS_SetSoundVolume(soundVolume);
        ASS_RegisterCallback(channelDone);
        if (oversampling != 0U) {
            ASS_SetOversampling();
        } else {
            ASS_ClrOversampling();
        }
        new ASSProcess;
    }
    asylumStarted = 1;
}

void closeAsylum()
{
    if (asylumStarted != 0U) {
        asylumStarted = 0;
        ASS_Uninit();
    }
}
