/* GRAPH\svgaprim.asm (1280:0000-0420) in C++: pixels, fills and one-pixel
 * lines clipped to a viewport, and the copy of a 320x200 screen. The VESA
 * BIOS calls (vesaModeInfo, vesaInfo) are the platform's screen's
 * (screen.h: screenVesaModeInfo, screenVesaInfo).
 *
 * Like the original the rows are found from the first one and the distance
 * between rows 0 and 1. The callers pass bounds in order (x1 <= x2, y1 <=
 * y2) and short coordinates: Viewport's pixel, hline, vline and clear. Where
 * they would not, the original runs its counts below zero (fills and lines
 * of 2^16 or 2^32 pixels); these do nothing. */
#include "svgaprim.h"
#include "gamestate.h"
#include "screen.h"
#include "viewport.h"

#include <algorithm>
#include <cstring>

u8 *&videoMemory = gameGlobal<u8 *>(screenVgaMemory());

namespace {

/* The distance between rows, which is needed only past the first row. */
s32 pitch(const ViewportPixels &viewport, s32 rows)
{
    return rows > 1 ? static_cast<s32>(viewport.rows[1] - viewport.rows[0]) : 0;
}

}

void putPixel(const ViewportPixels &viewport, s32 x, s32 y, u8 color)
{
    if (x < viewport.left || y < viewport.top || x > viewport.right || y > viewport.bottom) {
        return;
    }
    viewport.pixels[viewport.rows[y] + static_cast<std::int64_t>(x)] = color;
}

/* Outside the viewport the original returns the low byte of the coordinate
 * it tested last. No code of the game calls it. */
u8 getPixel(const ViewportPixels &viewport, s32 x, s32 y)
{
    if (x < viewport.left) {
        return static_cast<u8>(x);
    }
    if (y < viewport.top) {
        return static_cast<u8>(y);
    }
    if (x > viewport.right) {
        return static_cast<u8>(x);
    }
    if (y > viewport.bottom) {
        return static_cast<u8>(y);
    }
    return viewport.pixels[viewport.rows[y] + static_cast<std::int64_t>(x)];
}

void showScreen(const u8 *pixels)
{
    std::memcpy(videoMemory, pixels, 64000);
}

/* The low byte of color fills. */
void fillRect(const ViewportPixels &viewport, s32 x1, s32 y1, s32 x2, s32 y2, u16 color)
{
    if (x1 > viewport.right || x2 < viewport.left || y1 > viewport.bottom || y2 < viewport.top) {
        return;
    }
    y1 = std::max(y1, viewport.top);
    y2 = std::min(y2, viewport.bottom);
    x1 = std::max(x1, viewport.left);
    x2 = std::min(x2, viewport.right);
    if (x2 < x1 || y2 < y1) {
        return;
    }
    std::int64_t line = static_cast<std::int64_t>(viewport.rows[y1]) + x1;
    const s32 step = pitch(viewport, y2 - y1 + 1);
    for (s32 y = y1; y <= y2; y++, line += step) {
        std::memset(viewport.pixels + line, static_cast<u8>(color), x2 - x1 + 1);
    }
}

void drawVertical(const ViewportPixels &viewport, s32 x, s32 y1, s32 y2, u8 color)
{
    if (x > viewport.right || y1 > viewport.bottom || y2 < viewport.top || x < viewport.left) {
        return;
    }
    y2 = std::min(y2, viewport.bottom);
    y1 = std::max(y1, viewport.top);
    if (y2 < y1) {
        return;
    }
    std::int64_t at = static_cast<std::int64_t>(viewport.rows[y1]) + x;
    const s32 step = pitch(viewport, y2 - y1 + 1);
    for (s32 y = y1; y <= y2; y++, at += step) {
        viewport.pixels[at] = color;
    }
}

void drawHorizontal(const ViewportPixels &viewport, s32 x1, s32 y, s32 x2, u8 color)
{
    if (x1 > viewport.right || x2 < viewport.left || y > viewport.bottom || y < viewport.top) {
        return;
    }
    x1 = std::max(x1, viewport.left);
    x2 = std::min(x2, viewport.right);
    if (x2 < x1) {
        return;
    }
    std::memset(viewport.pixels + (viewport.rows[y] + static_cast<std::int64_t>(x1)), color, x2 - x1 + 1);
}

/* The original's forms (svgaprim.h). */
void putPixel(Viewport *viewport, s32 x, s32 y, u8 color)
{
    putPixel(viewport->pixels(), x, y, color);
}

/* UNCALLED: nothing calls it. */
u8 getPixel(Viewport *viewport, s32 x, s32 y)
{
    return getPixel(viewport->pixels(), x, y);
}

void fillRect(Viewport *viewport, s32 x1, s32 y1, s32 x2, s32 y2, u16 color)
{
    fillRect(viewport->pixels(), x1, y1, x2, y2, color);
}

void drawVertical(Viewport *viewport, s32 x, s32 y1, s32 y2, u8 color)
{
    drawVertical(viewport->pixels(), x, y1, y2, color);
}

void drawHorizontal(Viewport *viewport, s32 x1, s32 y, s32 x2, u8 color)
{
    drawHorizontal(viewport->pixels(), x1, y, x2, color);
}
