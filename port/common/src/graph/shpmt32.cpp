/* GRAPH\shpmt32.asm, the 32-bit drawShapeMirroredTranslucent (1000:0adc-0f86)
 * in C++: drawShapeTranslucent mirrored left to right (see shpt32.cpp and
 * drawframe.h). */
#include "drawframe.h"
#include "viewport.h"

void drawShapeMirroredTranslucent(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame, const u8 *table)
{
    drawframe::draw<true>(destination, x, y, frame, [table](u8 &out, u8) { out = table[out]; });
}

/* The original's form (x32code.h). */
void drawShapeMirroredTranslucent(Viewport *destination, s32 x, s32 y, ShapeFrame *frame, u8 *table)
{
    drawShapeMirroredTranslucent(destination->pixels(), x, y, reinterpret_cast<const u8 *>(frame), table);
}
