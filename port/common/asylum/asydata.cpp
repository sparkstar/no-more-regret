/* ASYLUM.DLL's data as the file has it (its data segment, segment 17): the
 * first values of the fields the module keeps and the constants of its
 * tables. The rest of the segment is 0 (the tables among it) or the
 * module does not keep it (Borland's run-time library's, the DMA
 * controller's and the interrupt controllers' ports, the IRQs' vectors). */
#include "asylib.h"

namespace asylum {

/* 129h: the periods of an octave, which buildPeriods spreads over eight. */
const u16 PERIOD_BASE[12] = {0x6b0, 0x650, 0x5f4, 0x5a0, 0x54c, 0x500, 0x4b8, 0x474, 0x434, 0x3f8, 0x3c0, 0x38b};

/* 141h and 161h: the pans of E8x, by 2 * pan. */
const u8 PAN_RIGHTS[32] = {64, 0, 64, 0, 64, 0, 64, 0, 64, 0, 64, 0, 64, 0, 64, 0,
                           56, 0, 48, 0, 40, 0, 32, 0, 24, 0, 16, 0, 8,  0, 0,  0};
const u8 PAN_LEFTS[32] = {0,  0, 8,  0, 16, 0, 24, 0, 32, 0, 40, 0, 48, 0, 56, 0,
                          64, 0, 64, 0, 64, 0, 64, 0, 64, 0, 64, 0, 64, 0, 64, 0};

/* 2D0h: the vibrato's half sine. */
const u8 VIBRATO_SINE[32] = {0,   25,  50,  74,  98,  120, 142, 162, 180, 197, 212, 225, 236, 244, 250, 254,
                             255, 254, 250, 244, 236, 225, 212, 197, 180, 162, 142, 120, 98,  74,  50,  25};

/* 111h: the cards' names (at 182h-1D4h; N/A at 1D5h). */
const char *const CARD_NAMES[5] = {"None", "Sound Blaster", "Sound Blaster Pro", "Sound Blaster 16/AWE32",
                                   "Ensoniq Soundscape"};

void loadData(Data &d)
{
    d = Data{};
    /* ABh: the C2 speeds by finetune. */
    const u16 c2spds[16] = {0x20ab, 0x20dd, 0x210f, 0x2151, 0x2185, 0x21cb, 0x2213, 0x2235,
                            0x1ed7, 0x1f05, 0x1f31, 0x1f6e, 0x1fab, 0x1fe9, 0x2028, 0x2058};
    std::memcpy(d.c2spds, c2spds, sizeof(c2spds));
    d.highSpeed = 1;                                     /* D2h */
    d.mixRate = d.tickRate = d.dspRate = d.rate = 22050; /* D4h-DBh */
    d.port = 0x220;                                      /* DCh */
    d.irq = 5;
    d.dma = 1;
    d.volume = 0x40; /* F4h */
    d.music = 1;
    d.sound = 1;
    /* CB1h: the 8-bit output's sample before the first, which the first
     * oversampled one adds. */
    d.oversample8 = 0x50;
    /* The port's 8-bit precision's, the same. */
    d.oversampleWords = 0x50;
}

}
