/* GRAPH\shpr32.asm, the 32-bit drawShapeRemapped (1000:0f88-14ee) in C++:
 * each pixel p of the frame is drawn through table (paletteTables->remap):
 * an entry table[p] of 0FFh draws p as it is; any other entry n draws
 * table[n*256+u], where u is the pixel under it. See drawframe.h. */
#include "drawframe.h"
#include "viewport.h"

void drawShapeRemapped(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame, const u8 *table)
{
    drawframe::draw<false>(destination, x, y, frame, [table](u8 &out, u8 in) { drawframe::remap(out, in, table); });
}

/* The original's form (x32code.h). */
void drawShapeRemapped(Viewport *destination, s32 x, s32 y, ShapeFrame *frame, u8 *table)
{
    drawShapeRemapped(destination->pixels(), x, y, reinterpret_cast<const u8 *>(frame), table);
}
