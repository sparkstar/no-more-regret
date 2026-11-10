/* The SuperVGA primitives (1280, TASM, no file name): VESA BIOS calls, and
 * pixels, fills and one-pixel lines in long coordinates, clipped to a
 * viewport. In the port the pixel routines are C++; the
 * forms with a Viewport pass Viewport::pixels. The BIOS calls (vesaModeInfo
 * 1280:0000, vesaInfo 1280:0017) are the platform's screen's (screen.h). */
#ifndef PORT_SVGAPRIM_H
#define PORT_SVGAPRIM_H
#include "common.h"
#include "pixels.h"

class Viewport;
void putPixel(Viewport *viewport, s32 x, s32 y, u8 color); /* 1280:002b */
/* UNCALLED: nothing calls it. */
u8 getPixel(Viewport *viewport, s32 x, s32 y); /* 1280:00bc */
/* Copies the 64000 bytes of pixels to the mode 13h screen (1280:014d); the
 * original takes their selector (and a bank step it does not use). */
void showScreen(const u8 *pixels);
void fillRect(Viewport *viewport, s32 x1, s32 y1, s32 x2, s32 y2, u16 color); /* 1280:0171 */
void drawVertical(Viewport *viewport, s32 x, s32 y1, s32 y2, u8 color);       /* 1280:028e */
void drawHorizontal(Viewport *viewport, s32 x1, s32 y, s32 x2, u8 color);     /* 1280:0357 */

/* The forms the C++ versions take, in long coordinates. videoMemory is the
 * memory videoSelector reaches (CPROTMEM.C), which the system layer
 * provides. */
extern u8 *&videoMemory;
void putPixel(const ViewportPixels &viewport, s32 x, s32 y, u8 color);
u8 getPixel(const ViewportPixels &viewport, s32 x, s32 y);
void fillRect(const ViewportPixels &viewport, s32 x1, s32 y1, s32 x2, s32 y2, u16 color);
void drawVertical(const ViewportPixels &viewport, s32 x, s32 y1, s32 y2, u8 color);
void drawHorizontal(const ViewportPixels &viewport, s32 x1, s32 y, s32 x2, u8 color);
#endif
