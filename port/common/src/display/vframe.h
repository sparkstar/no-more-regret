/* VFRAME.C (1430): compressed frames of a screen rectangle. Two formats share
 * a head of FRAME_HEAD bytes before the data:
 * - run frames (packRuns): bands of 10 rows, each a list of run lengths that
 *   alternate between transparent (FFh) and drawn pixels, and the drawn
 *   pixels with RUN_REPEAT n v for n copies of v;
 * - band frames (packBands): bands of 20 rows of pixels in which
 *   BAND_REPEAT v n, or n v for n below BAND_SHORT, stands for n+3 copies of
 *   v. drawRleFrame draws them. */
#ifndef PORT_VFRAME_H
#define PORT_VFRAME_H
#include "common.h"
#include "viewport.h"

constexpr int FRAME_HEAD = 0x8c;
/* The lines of a band: run frames and band frames. */
constexpr int RUN_LINES = 10;
constexpr int BAND_LINES = 20;
constexpr int RUN_REPEAT = 0x80;
constexpr int BAND_REPEAT = 0xfe;
constexpr int BAND_SHORT = 10;

struct RunFrame {
    u16 width, height;
    u16 left, top;
    u16 dataSize;
    u16 runSize;
    /* Where each band starts in the data and in the runs. */
    u16 dataOffsets[32];
    u16 runOffsets[32];
};
ASSERT_SIZE(RunFrame, FRAME_HEAD);
struct BandFrame {
    u16 width, height;
    u16 left, top;
    u16 offsets[32];
};
ASSERT_SIZE(BandFrame, 0x48);

/* Draws bands bands from band (-1: all the rest). */
void drawBands(Viewport *target, BandFrame *frame, i16 x, i16 y, i16 band, i16 bands);       /* 1430:0b1c */
void drawBandsBanked(Viewport *target, BandFrame *frame, i16 x, i16 y, i16 band, i16 bands); /* 1430:0bc1 */

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
u32 packRuns(Viewport *screen, Rect area, u8 **out);
/* UNCALLED: nothing calls it. */
void unpackRuns(Viewport *target, RunFrame *frame, i16 x, i16 y, i16 firstBand, i16 lastBand);
/* UNCALLED: nothing calls it. */
u32 packBands(Viewport *screen, Rect area, u8 *out);
/* UNCALLED: nothing calls it. */
void unpackBands(Viewport *target, BandFrame *frame, i16 x, i16 y, i16 band, i16 bands);
#endif
