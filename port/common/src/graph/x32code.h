/* The 32-bit drawing code (1290:0000-0290, TASM, no file name). The routines
 * lie in Code1 (1000:025c-2c57); copy32Code moves them into a buffer that
 * the game's start turns into a 32-bit code segment, and each entry point
 * jumps to its copy through the table at 1480:4f48. The entries that draw
 * on the screen load AX with videoSelector first. In the port the
 * routines are C++: the forms of the original, with a Viewport, pass
 * Viewport::pixels to the forms that take the part of a Viewport they read
 * (pixels.h), which the comparison with the original checks. copy32Code
 * and code32Size are only declared: the port has no 32-bit segment, and
 * nothing calls them. */
#ifndef PORT_X32CODE_H
#define PORT_X32CODE_H
#include "common.h"
#include "pixels.h"

class Viewport;
struct ShapeFrame;

/* The bytes the copies need, with the 8-byte header (1290:0000). */
u16 code32Size();
/* Copies the routines to buffer and fills the jump table (1290:00a9). */
void copy32Code(void *buffer);

/* Draws a frame with its origin at (x,y) (1290:0061), mirrored
 * (1290:0065). */
void drawShape(Viewport *destination, s32 x, s32 y, ShapeFrame *frame);
void drawShapeMirrored(Viewport *destination, s32 x, s32 y, ShapeFrame *frame);
/* The same with each pixel mixed with the one under it through table
 * (1290:0069, 006d) or looked up in table (1290:0071, 0075). */
void drawShapeTranslucent(Viewport *destination, s32 x, s32 y, ShapeFrame *frame, u8 *table);
void drawShapeMirroredTranslucent(Viewport *destination, s32 x, s32 y, ShapeFrame *frame, u8 *table);
void drawShapeRemapped(Viewport *destination, s32 x, s32 y, ShapeFrame *frame, u8 *table);
void drawShapeMirroredRemapped(Viewport *destination, s32 x, s32 y, ShapeFrame *frame, u8 *table);
/* Viewport copies (1290:0079, 0080, 0084, 008b); the screen is banked
 * by bankStep. */
void copyFromScreen(Viewport *destination, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y, u16 bankStep);
void copyViewport(Viewport *source, Viewport *destination, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y);
void copyToScreen(Viewport *source, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y);
void copyToSvgaScreen(Viewport *source, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y, u16 bankStep);
/* Copies a whole buffer to the banked screen (1290:0092); the original
 * takes the buffer's selector. */
void showSvgaScreen(const u8 *pixels, u16 bankStep);
/* Moves the VESA window (1290:0099) and reads where it is (1290:00a1):
 * where the originals would have left it. */
void setWindow(u16 position);
u16 getWindow();
/* Draws lines rows of a VFRAME.C band frame, width pixels each, at (x,y)
 * (1290:009d), and the same on the banked screen (1290:00a5). */
void drawRleFrame(u8 *data, i16 x, i16 y, i16 lines, Viewport *target, u16 width);
void drawRleFrameBanked(u8 *data, i16 x, i16 y, i16 lines, Viewport *target, u16 width, u16 bankStep);

/* The forms the C++ versions take. A frame is its bytes (shape.h
 * ShapeFrame). shpt32 and shpmt32 draw no pixel of the frame but turn each
 * pixel under it into its entry in table; shpr32 and shpmr32 draw each pixel
 * p through table: as it is where table[p] is 0FFh, else as
 * table[table[p]*256+u] of the pixel u under it. */
void drawShape(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame);
void drawShapeMirrored(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame);
void drawShapeTranslucent(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame, const u8 *table);
void drawShapeMirroredTranslucent(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame, const u8 *table);
void drawShapeRemapped(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame, const u8 *table);
void drawShapeMirroredRemapped(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame, const u8 *table);
/* copyViewport copies the rectangle (x1,y1)-(x2,y2) of source to (x,y) of
 * destination, copyToScreen to (x,y) of the VGA screen (videoMemory,
 * 320x200); drawRleFrame draws a band frame at (x,y) of target. See
 * view32.cpp for the original's quirks they keep. */
void copyViewport(const ViewportPixels &source, const ViewportPixels &destination, s32 x1, s32 y1, s32 x2, s32 y2,
                  s32 x, s32 y);
void copyToScreen(const ViewportPixels &source, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y);
void drawRleFrame(const u8 *data, s16 x, s16 y, s16 lines, const ViewportPixels &target, u16 width);
/* The same on the SVGA screen: svgaMemory, which the system layer provides,
 * 640 pixels a row and SVGA_MEMORY bytes (512 rows, as the video memory's
 * first 5 windows hold, for the reads past row 479). bankStep and the window
 * position are the original's VESA window. */
extern u8 *&svgaMemory;
const u32 SVGA_MEMORY = 5 * 0x10000;
void copyFromScreen(const ViewportPixels &destination, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y, u16 bankStep);
void copyToSvgaScreen(const ViewportPixels &source, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y, u16 bankStep);
void drawRleFrameBanked(const u8 *data, s16 x, s16 y, s16 lines, const ViewportPixels &target, u16 width, u16 bankStep);
#endif
