/* GRAPH\view32.asm (1000:1aa4-2c5a) in C++: the copies between viewports and
 * the screens, the movie band frames of VFRAME.C and the VESA window.
 *
 * The SVGA screen here is svgaMemory, flat; the originals see it through a
 * 64K window at videoSelector that they move in steps of bankStep. Their
 * pixels land where these put them as long as a band frame crosses at most
 * one window's end, as the game's small bands do: after the first move the
 * banked drawer draws on without looking. The window's position is kept as
 * the originals leave it, for getWindow.
 *
 * The copies follow the original's clipping step by step, quirks included:
 * - The copy stops when the rectangle's right or bottom end on the target
 *   lies left of or above the source's bounds (not the target's).
 * - Rows are the bounds' width apart on both sides (right - left + 1), not
 *   the distance between rows 0 and 1; the two agree for a viewport whose
 *   bounds cover its buffer.
 * - The VGA copy does not cut the rectangle to the source's right and bottom
 *   and reads past them.
 * - The original stops on a count of 0 rows or columns and runs a negative
 *   count on. Viewport::blit's callers keep the rectangle's far corner right
 *   of and below the target's near edges, and copyViewport's inside the
 *   source's bounds, so none arises; these stop on either.
 * The pixels move as REP MOVSD then REP MOVSB move them, a dword at a time
 * from the start, which matters when source and target overlap. */
#include "x32code.h"
#include "gamestate.h"
#include "screen.h"
#include "svgaprim.h"
#include "viewport.h"

#include <cstring>

namespace {

const s32 VGA_WIDTH = 320;
const s32 VGA_RIGHT = 319;
const s32 VGA_BOTTOM = 199;
const s32 SVGA_WIDTH = SCREEN_WIDTH;
const s32 SVGA_RIGHT = SCREEN_RIGHT;
const s32 SVGA_BOTTOM = SCREEN_BOTTOM;

/* The window's position as the originals keep it in their code's header:
 * 0FFFFh for none yet. */
u16 &window = gameGlobal<u16>(0xffff);

/* The window after rows rows 640 apart from offset start: the originals move
 * it on each time the start of the next row reaches the next 64K. */
void windowAfterRows(u32 start, s32 rows, u16 bankStep)
{
    window = static_cast<u16>(((start + static_cast<u32>(rows) * SVGA_WIDTH) >> 16) * bankStep);
}

/* REP MOVSD for count/4 dwords, then REP MOVSB for the rest. */
void moveForward(u8 *to, const u8 *from, u32 count)
{
    u32 k = 0;
    for (; k + 4 <= count; k += 4) {
        u8 dword[4];
        std::memcpy(dword, from + k, 4);
        std::memcpy(to + k, dword, 4);
    }
    for (; k < count; k++) {
        to[k] = from[k];
    }
}

/* A band frame drawn from row, the target's rows pitch apart; see
 * drawRleFrame. */
void drawBands(const u8 *data, u8 *pixels, std::int64_t row, u32 left, u32 rowsLeft, s32 pitch, u16 width)
{
    u32 column = 0;
    u32 ecx = 0;
    auto endRow = [&]() {
        row += pitch;
        column = 0;
        return --rowsLeft != 0;
    };
    for (;;) {
        const u8 code = data[0];
        if (code < 10 || code == 0xfe) {
            u8 color;
            if (code == 0xfe) {
                color = data[1];
                ecx = (ecx & ~0xffU) | data[2];
                data += 3;
            } else {
                color = data[1];
                ecx = (ecx & ~0xffU) | code;
                data += 2;
            }
            ecx += 4;
            /* The run, a row at a time. */
            for (;;) {
                const u32 room = width - column;
                const u32 part = ecx <= room ? ecx : room;
                if (color != 0xff) {
                    std::memset(pixels + (row + left + column), color, part);
                }
                column += part;
                if (ecx <= room) {
                    if (color != 0xff) {
                        ecx = 0;
                    }
                    break;
                }
                ecx -= part;
                if (!endRow()) {
                    return;
                }
            }
        } else {
            if (code != 0xff) {
                pixels[row + left + column] = code;
            }
            data++;
            column++;
        }
        if (column == width && !endRow()) {
            return;
        }
    }
}

}

void copyViewport(const ViewportPixels &source, const ViewportPixels &destination, s32 x1, s32 y1, s32 x2, s32 y2,
                  s32 x, s32 y)
{
    if (x1 > source.right) {
        return;
    }
    if (x1 < source.left) {
        x += source.left - x1;
        x1 = source.left;
    }
    if (y1 > source.bottom) {
        return;
    }
    if (y1 < source.top) {
        y += source.top - y1;
        y1 = source.top;
    }
    if (x2 < source.left || y2 < source.top || x > destination.right || y > destination.bottom) {
        return;
    }
    s32 right = x2 - x1 + x;
    if (right < source.left) {
        return;
    }
    s32 bottom = y2 - y1 + y;
    if (bottom < source.top) {
        return;
    }
    if (x < destination.left) {
        x1 += destination.left - x;
        x = destination.left;
    }
    if (y < destination.top) {
        y1 += destination.top - y;
        y = destination.top;
    }
    if (right > destination.right) {
        x2 -= right - destination.right;
        right = destination.right;
    }
    if (bottom > destination.bottom) {
        y2 -= bottom - destination.bottom;
        bottom = destination.bottom;
    }
    if (x1 < source.left) {
        x += source.left - x1;
        x1 = source.left;
    }
    if (y1 < source.top) {
        y += source.top - y1;
        y1 = source.top;
    }
    if (x2 > source.right) {
        right -= x2 - source.right;
        x2 = source.right;
    }
    if (y2 > source.bottom) {
        bottom -= y2 - source.bottom;
        y2 = source.bottom;
    }
    s32 rows = bottom - y + 1;
    const s32 columns = right - x + 1;
    if (rows <= 0 || columns <= 0) {
        return;
    }
    const s32 destinationSkip = destination.right - destination.left + 1 - columns;
    const s32 sourceSkip = source.right - source.left + 1 - (x2 - x1 + 1);
    std::int64_t to = static_cast<std::int64_t>(destination.rows[y]) + x;
    std::int64_t from = static_cast<std::int64_t>(source.rows[y1]) + x1;
    const u32 count = static_cast<u16>(columns);
    for (; rows > 0; rows--) {
        moveForward(destination.pixels + to, source.pixels + from, count);
        to += count + destinationSkip;
        from += count + sourceSkip;
    }
}

void copyToScreen(const ViewportPixels &source, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y)
{
    if (x1 > source.right) {
        return;
    }
    if (x1 < source.left) {
        x += source.left - x1;
        x1 = source.left;
    }
    if (y1 > source.bottom) {
        return;
    }
    if (y1 < source.top) {
        y += source.top - y1;
        y1 = source.top;
    }
    if (x2 < source.left || y2 < source.top || x > VGA_RIGHT || y > VGA_BOTTOM) {
        return;
    }
    s32 right = x2 - x1 + x;
    if (right < source.left) {
        return;
    }
    s32 bottom = y2 - y1 + y;
    if (bottom < source.top) {
        return;
    }
    if (x < 0) {
        x1 -= x;
        x = 0;
    }
    if (y < 0) {
        y1 -= y;
        y = 0;
    }
    if (right > VGA_RIGHT) {
        x2 -= right - VGA_RIGHT;
        right = VGA_RIGHT;
    }
    if (bottom > VGA_BOTTOM) {
        y2 -= bottom - VGA_BOTTOM;
        bottom = VGA_BOTTOM;
    }
    s32 rows = bottom - y + 1;
    const s32 columns = right - x + 1;
    if (rows <= 0 || columns <= 0) {
        return;
    }
    const s32 screenSkip = VGA_WIDTH - columns;
    const s32 sourceSkip = source.right - source.left + 1 - (x2 - x1 + 1);
    std::int64_t to = (static_cast<std::int64_t>(y) * VGA_WIDTH) + x;
    std::int64_t from = static_cast<std::int64_t>(source.rows[y1]) + x1;
    for (; rows > 0; rows--) {
        moveForward(videoMemory + to, source.pixels + from, static_cast<u32>(columns));
        to += columns + screenSkip;
        from += columns + sourceSkip;
    }
}

u8 *&svgaMemory = gameGlobal<u8 *>(screenSvgaMemory());

/* A frame is lines rows of width pixels, packed across the rows: a byte n
 * below 10 is a run of n+4 pixels of the next byte, 0FEh a run of n+4 pixels
 * of the byte after it, where n is the byte after that; 0FFh as a pixel or as
 * a run's colour leaves the pixels as they are; any other byte is a pixel.
 * It is not clipped.
 *
 * The original keeps a run's count in ECX and loads only CL for the next
 * one. A drawn run leaves ECX at 0, but a run of 0FFh leaves it at the count
 * that went into its last row, so after one of 256 pixels or more the next
 * run is 256 (or a multiple) longer. This keeps ECX the same way. x, y and
 * lines count as unsigned words (MOVZX). A run past the last row, which the
 * original draws on into the rows below, stops here; so does lines = 0, which
 * the original takes as 2^32 rows. */
void drawRleFrame(const u8 *data, s16 x, s16 y, s16 lines, const ViewportPixels &target, u16 width)
{
    if (static_cast<u16>(lines) == 0 || width == 0) {
        return;
    }
    drawBands(data, target.pixels, target.rows[static_cast<u16>(y)], static_cast<u16>(x), static_cast<u16>(lines),
              static_cast<s32>(target.rows[1] - target.rows[0]), width);
}

/* drawRleFrame on the SVGA screen. The original draws the rows that fit
 * before the end of the window its first row starts in (its bytes left /
 * 640), then the rest while it watches for the end, where it moves the
 * window on once. lines = 0 draws nothing. */
void drawRleFrameBanked(const u8 *data, s16 x, s16 y, s16 lines, const ViewportPixels &target, u16 width, u16 bankStep)
{
    const u32 start = target.rows[static_cast<u16>(y)];
    window = static_cast<u16>((start >> 16) * bankStep);
    const u32 fit = ((0x10000 - (start & 0xffff)) >> 7) / 5;
    if (static_cast<u16>(lines) == 0 || width == 0) {
        return;
    }
    drawBands(data, target.pixels, start, static_cast<u16>(x), static_cast<u16>(lines),
              static_cast<s32>(target.rows[1] - target.rows[0]), width);
    if (static_cast<u16>(lines) > fit) {
        window = static_cast<u16>(window + bankStep);
    }
}

/* Like copyToScreen on the SVGA screen, 640x480. */
void copyToSvgaScreen(const ViewportPixels &source, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y, u16 bankStep)
{
    if (x1 > source.right) {
        return;
    }
    if (x1 < source.left) {
        x += source.left - x1;
        x1 = source.left;
    }
    if (y1 > source.bottom) {
        return;
    }
    if (y1 < source.top) {
        y += source.top - y1;
        y1 = source.top;
    }
    if (x2 < source.left || y2 < source.top || x > SVGA_RIGHT || y > SVGA_BOTTOM) {
        return;
    }
    s32 right = x2 - x1 + x;
    if (right < source.left) {
        return;
    }
    s32 bottom = y2 - y1 + y;
    if (bottom < source.top) {
        return;
    }
    if (x < 0) {
        x1 -= x;
        x = 0;
    }
    if (y < 0) {
        y1 -= y;
        y = 0;
    }
    if (right > SVGA_RIGHT) {
        x2 -= right - SVGA_RIGHT;
        right = SVGA_RIGHT;
    }
    if (bottom > SVGA_BOTTOM) {
        y2 -= bottom - SVGA_BOTTOM;
        bottom = SVGA_BOTTOM;
    }
    const s32 rows = bottom - y + 1;
    const s32 columns = right - x + 1;
    if (rows <= 0 || columns <= 0) {
        return;
    }
    const s32 sourceSkip = source.right - source.left + 1 - (x2 - x1 + 1);
    const u32 start = static_cast<u32>((y * SVGA_WIDTH) + x);
    std::int64_t from = static_cast<std::int64_t>(source.rows[y1]) + x1;
    for (s32 k = 0; k < rows; k++) {
        moveForward(svgaMemory + start + (static_cast<std::int64_t>(k) * SVGA_WIDTH), source.pixels + from,
                    static_cast<u16>(columns));
        from += static_cast<u16>(columns) + sourceSkip;
    }
    windowAfterRows(start, rows, bankStep);
}

/* Copies the rectangle (x1,y1)-(x2,y2) of the SVGA screen to (x,y) of
 * destination. Quirks of the original it keeps: a rectangle that starts
 * left of or above the destination's bounds (not the screen's) moves (x,y)
 * on and starts at 0; the stop tests of the far corner compare with 0; the
 * rectangle is not cut to the screen's right and bottom, so it reads the
 * rows past 479 (the mouse cursor at the bottom), which svgaMemory holds;
 * the destination's rows are its bounds' width apart. Callers put (x,y)
 * inside the destination's bounds, where no count comes out negative. */
void copyFromScreen(const ViewportPixels &destination, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y, u16 bankStep)
{
    if (x1 > SVGA_RIGHT) {
        return;
    }
    if (x1 < destination.left) {
        x += destination.left - x1;
        x1 = 0;
    }
    if (y1 > SVGA_BOTTOM) {
        return;
    }
    if (y1 < destination.top) {
        y += destination.top - y1;
        y1 = 0;
    }
    if (x2 < 0 || y2 < 0 || x > destination.right || y > destination.bottom) {
        return;
    }
    s32 right = x2 - x1 + x;
    if (right < 0) {
        return;
    }
    s32 bottom = y2 - y1 + y;
    if (bottom < 0) {
        return;
    }
    if (x < destination.left) {
        x1 += destination.left - x;
        x = destination.left;
    }
    if (y < destination.top) {
        y1 += destination.top - y;
        y = destination.top;
    }
    if (right > destination.right) {
        x2 -= right - destination.right;
        right = destination.right;
    }
    if (bottom > destination.bottom) {
        y2 -= bottom - destination.bottom;
        bottom = destination.bottom;
    }
    const s32 rows = bottom - y + 1;
    const s32 columns = x2 - x1 + 1;
    if (rows <= 0 || columns <= 0) {
        return;
    }
    const s32 destinationSkip = destination.right - destination.left + 1 - (right - x + 1);
    const u32 start = static_cast<u32>((y1 * SVGA_WIDTH) + x1);
    std::int64_t to = static_cast<std::int64_t>(destination.rows[y]) + x;
    for (s32 k = 0; k < rows; k++) {
        moveForward(destination.pixels + to, svgaMemory + start + (static_cast<std::int64_t>(k) * SVGA_WIDTH),
                    static_cast<u16>(columns));
        to += static_cast<u16>(columns) + destinationSkip;
    }
    windowAfterRows(start, rows, bankStep);
}

/* Copies 640x480 pixels to the SVGA screen, the original a window at a time
 * from position 0. */
void showSvgaScreen(const u8 *pixels, u16 bankStep)
{
    std::memcpy(svgaMemory, pixels, SVGA_WIDTH * SCREEN_HEIGHT);
    window = static_cast<u16>(4 * bankStep);
}

void setWindow(u16 position)
{
    window = position;
}

u16 getWindow()
{
    return window;
}

/* The original's forms (x32code.h). */
void copyViewport(Viewport *source, Viewport *destination, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y)
{
    copyViewport(source->pixels(), destination->pixels(), x1, y1, x2, y2, x, y);
}

void copyToScreen(Viewport *source, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y)
{
    copyToScreen(source->pixels(), x1, y1, x2, y2, x, y);
}

void copyFromScreen(Viewport *destination, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y, u16 bankStep)
{
    copyFromScreen(destination->pixels(), x1, y1, x2, y2, x, y, bankStep);
}

void copyToSvgaScreen(Viewport *source, s32 x1, s32 y1, s32 x2, s32 y2, s32 x, s32 y, u16 bankStep)
{
    copyToSvgaScreen(source->pixels(), x1, y1, x2, y2, x, y, bankStep);
}

void drawRleFrame(u8 *data, i16 x, i16 y, i16 lines, Viewport *target, u16 width)
{
    drawRleFrame(reinterpret_cast<const u8 *>(data), x, y, lines, target->pixels(), width);
}

void drawRleFrameBanked(u8 *data, i16 x, i16 y, i16 lines, Viewport *target, u16 width, u16 bankStep)
{
    drawRleFrameBanked(reinterpret_cast<const u8 *>(data), x, y, lines, target->pixels(), width, bankStep);
}
