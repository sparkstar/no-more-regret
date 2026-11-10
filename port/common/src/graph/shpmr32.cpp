/* GRAPH\shpmr32.asm, the 32-bit drawShapeMirroredRemapped (1000:14f0-1aa1)
 * in C++: drawShapeRemapped mirrored left to right (see shpr32.cpp and
 * drawframe.h). */
#include "drawframe.h"
#include "viewport.h"

void drawShapeMirroredRemapped(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame, const u8 *table)
{
    drawframe::draw<true>(destination, x, y, frame, [table](u8 &out, u8 in) { drawframe::remap(out, in, table); });
}

/* The original's form (x32code.h). */
void drawShapeMirroredRemapped(Viewport *destination, s32 x, s32 y, ShapeFrame *frame, u8 *table)
{
    drawShapeMirroredRemapped(destination->pixels(), x, y, reinterpret_cast<const u8 *>(frame), table);
}
