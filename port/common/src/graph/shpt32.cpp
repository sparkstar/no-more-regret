/* GRAPH\shpt32.asm, the 32-bit drawShapeTranslucent (1000:2c5c-30d5) in
 * C++: the frame's own pixels are not drawn; each pixel of the viewport that
 * a run covers becomes its entry in table (paletteTables->translucent). See
 * drawframe.h. */
#include "drawframe.h"
#include "viewport.h"

void drawShapeTranslucent(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame, const u8 *table)
{
    drawframe::draw<false>(destination, x, y, frame, [table](u8 &out, u8) { out = table[out]; });
}

/* The original's form (x32code.h). */
void drawShapeTranslucent(Viewport *destination, s32 x, s32 y, ShapeFrame *frame, u8 *table)
{
    drawShapeTranslucent(destination->pixels(), x, y, reinterpret_cast<const u8 *>(frame), table);
}
