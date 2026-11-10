/* ..\\ITEM\\PRELOAD.C (Code33, 1100): loading the shapes the map needs
 * into the shape cache before the game starts. */
#ifndef PORT_PRELOAD_H
#define PORT_PRELOAD_H
#include "common.h"

/* One bit for each frame of a shape. */
class FrameSet {
public:
    u8 bits[32];

    FrameSet() { clear(); }
    void clear();        /* 1100:0000 */
    void set(u16 frame); /* 1100:0013 */
    u8 test(u16 frame);  /* 1100:0031 */
};
ASSERT_SIZE(FrameSet, 32);

void markFrame(u16 shape, u16 frame); /* 1100:010d */
/* Allocate and free the frame sets (1100:0051, 00a1); freeFrameSets gives
 * 1. */
void allocateFrameSets();
u8 freeFrameSets();
/* Marks the frames of the items on the map and in their globs in the frame
 * sets it allocates; 1 (1100:0165). */
bool markMapFrames();
/* Called by ITEMCACH.C when a map is loaded; the argument is not read
 * (1100:0529). */
void preloadMap(u8 flag);

/* Nothing calls these, in the original either. */
u8 isFrameMarked(u16 shape, u16 frame);
#endif
