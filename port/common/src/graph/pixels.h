/* The part of a Viewport (viewport.h) that the drawing code reads: the
 * pixels, the offset of each row from them (VPORT.C buildRows: row y at y
 * * width) and the bounds that clip the drawing. The original holds a
 * selector for the pixels; the port a pointer (Viewport::pixels). The C++
 * versions of the drawing modules take it. */
#ifndef PORT_PIXELS_H
#define PORT_PIXELS_H
#include "common.h"

struct ViewportPixels {
    u8 *pixels;
    const u32 *rows;
    s32 left, top, right, bottom;
};
#endif
