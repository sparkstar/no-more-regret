/* The port's display: the video mode the BIOS and VESA calls set and
 * report, the video memory (the VGA screen at A000h and the SVGA screen,
 * flat: svgaprim.h's videoMemory and x32code.h's svgaMemory), the palette
 * and the retrace. The screen as the mode shows it goes to the window
 * (window.h) and can be written as a PNG. */
#ifndef PORT_VIDEO_H
#define PORT_VIDEO_H
#include "common.h"
#include "screen.h"

#include <string>

struct VideoState {
    /* The BIOS mode (3 is 80-column text, 13h 320x200) or the VESA mode
     * (VESA_*). */
    u16 mode;
    /* The scan lines of the text font: 16 for 25 rows, 8 for 50. */
    u16 fontHeight;
};
extern VideoState video;

/* The SVGA memory: VESA 101h's 640x480 in five banks (x32code.h's
 * SVGA_MEMORY, which view32.cpp draws on), the 16-bit mode's of the movies
 * in ten. */
const u32 SVGA_FRAMEBUFFER = 10 * 0x10000;

/* host.h's hostTextMode (conio's textmode): 80 columns of 25 rows, or of 50 with the 8-line font. */
void setTextMode(bool fiftyRows);
/* The screen the mode shows: its pixels, width x height of them with rows
 * pitch bytes apart, of bits each: 8, an index of the palette, whose 256
 * colours are red, green and blue of 8 bits; or 16, VESA 111h's colour of
 * its own (the hi-colour movies), 5 bits of red, 6 of green and 5 of blue
 * from the top, low byte first. False in a text mode. */
struct ScreenView {
    const u8 *pixels;
    u16 width, height, pitch;
    u8 bits;
    u8 palette[256 * 3];
    /* The wide screen's (viewWideScreen): the frame's width for 480 rows
     * (640: 4:3), the sides' columns at each side of the pixels (0: the
     * mode's screen alone, 4:3 in the frame's middle), and whether they
     * are dimmed. */
    u16 frameWidth = SCREEN_WIDTH;
    u16 sides = 0;
    bool dimmed = false;
};
bool viewScreen(ScreenView &view);
/* The port's wide screen (screen.h's screenSetWide): its width for 480
 * rows, which the window sets from the shape of its picture (the window,
 * over the whole screen its display) as it changes; WIDE_DEFAULT without a
 * window. Even, 640 to WIDE_MOST: the fast area reaches 768 pixels each
 * side of the screen's middle, the world beyond it is not there. */
const u16 WIDE_DEFAULT = 854;
const u16 WIDE_MOST = 1536;
void setWideWidth(u16 width);
/* The frame's width for 480 rows: the wide screen's, 640 when it is off;
 * and its mode (screen.h's), for the window thread. */
u16 wideFrameWidth();
u8 wideModeNow();
/* The screen as the window shows it: viewScreen's, and with the wide
 * screen on its frame, with the sides the game drew around mode 101h's. */
bool viewWideScreen(ScreenView &view);
/* The pixel at x, y of a view as red, green and blue of 8 bits
 * (0xRRGGBB): 16-bit colours with their top bits again below, as the
 * palette's 6 bits are. */
inline u32 viewColour(const ScreenView &view, u16 x, u16 y)
{
    if (view.bits == 16) {
        const u8 *p = view.pixels + (static_cast<size_t>(y) * view.pitch) + (2U * x);
        const u16 c = static_cast<u16>(p[0] | p[1] << 8);
        const u32 red = c >> 11;
        const u32 green = c >> 5 & 0x3f;
        const u32 blue = c & 0x1f;
        return (red << 3 | red >> 2) << 16 | (green << 2 | green >> 4) << 8 | (blue << 3 | blue >> 2);
    }
    const u8 *rgb = view.palette + (3 * view.pixels[(static_cast<size_t>(y) * view.pitch) + x]);
    return static_cast<u32>(rgb[0]) << 16 | rgb[1] << 8 | rgb[2];
}
/* The same as a PNG at path (indexed, or of red, green and blue in the
 * 16-bit mode); false in a text mode or when it cannot be written. */
bool writeScreen(const std::string &path);
#endif
