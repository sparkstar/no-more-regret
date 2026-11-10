/* GRAPH\shp32.asm, the 32-bit drawShape (1000:025c-0674) in C++: draws a
 * frame of a shape on a viewport, clipped to the viewport's bounds (see
 * drawframe.h). */
#include "drawframe.h"
#include "viewport.h"

void drawShape(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame)
{
    drawframe::draw<false>(destination, x, y, frame, [](u8 &out, u8 in) { out = in; });
}

/* The original's form (x32code.h). */
void drawShape(Viewport *destination, s32 x, s32 y, ShapeFrame *frame)
{
    drawShape(destination->pixels(), x, y, reinterpret_cast<const u8 *>(frame));
}
