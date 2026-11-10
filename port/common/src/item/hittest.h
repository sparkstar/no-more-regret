/* Code44 (1158, file name unknown), the parts the game uses: overlap tests
 * of boxes and of shape frames. */
#ifndef PORT_HITTEST_H
#define PORT_HITTEST_H
#include "common.h"
#include "rect.h"
#include "shape.h"

/* 1 when the pixel at (x,y) of frame, placed with its origin at (left,top),
 * is drawn (1158:01c8). */
bool frameHit(ShapeFrame *frame, s32 left, s32 top, s32 x, s32 y);
/* The same on the frame's bytes, which the C++ version of framehit.asm
 * reads. */
bool frameHit(const u8 *frame, s32 left, s32 top, s32 x, s32 y);
inline bool frameHit(ShapeFrame *frame, Point origin, Point at)
{
    return frameHit(frame, origin.x, origin.y, at.x, at.y);
}
#endif
