/* GRAPH\shpm32.asm, the 32-bit drawShapeMirrored (1000:0678-0ad8) in C++:
 * drawShape mirrored left to right (see drawframe.h). */
#include "drawframe.h"
#include "viewport.h"

void drawShapeMirrored(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame)
{
    drawframe::draw<true>(destination, x, y, frame, [](u8 &out, u8 in) { out = in; });
}

/* The original's form (x32code.h). */
void drawShapeMirrored(Viewport *destination, s32 x, s32 y, ShapeFrame *frame)
{
    drawShapeMirrored(destination->pixels(), x, y, reinterpret_cast<const u8 *>(frame));
}
