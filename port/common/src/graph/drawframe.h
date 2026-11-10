/* What the six 32-bit shape drawers share (shp32.asm and its five
 * variants): the clipping and the walk over a frame's runs. Each drawer's
 * .cpp gives the way a pixel reaches the viewport and the direction.
 *
 * A frame is its header (shape.h ShapeFrame: flags, width, height and the
 * origin, little-endian dwords) and a table of one offset per row, each from
 * its own entry. A row is runs of a byte of pixels to skip, a byte of length
 * and the pixels, until the frame's width is used up. In a frame whose flags
 * have FRAME_FILLS, a length's low bit makes the run a fill of length/2
 * pixels of the next byte; without it the run copies length/2 pixels.
 *
 * Each original has a loop for each of: pixels or fills, times not cut, cut
 * at the start of the rows (and maybe the end), or cut at the end only. They
 * do what this one does wherever each row's runs end at the width, as in all
 * the game's frames but two: frames 131 and 182 of SHAPES.FLX shape 1431
 * start with a skip past the width. Drawn uncut, the originals take the next
 * bytes as runs and go on until a count lands on 0; this version ends the
 * row at the width, as the originals' cut loops do.
 *
 * A run of length 0 draws nothing here. drawShape copies with REP MOVS and
 * does the same; the other five copy a pixel at a time with DEC ECX / JNE,
 * which runs 2^32 times on a count of 0. Such runs are in 11 frames of
 * SHAPES.FLX shape 804 and in GUMPS.FLX shapes 44-47, which the game must
 * draw with drawShape only.
 *
 * Like the originals it finds the rows from the first one it draws and the
 * distance between rows 0 and 1 (a Viewport's rows are evenly spaced). The
 * machine code returns 1 when the frame lies outside the bounds; no caller
 * reads it (x32code.h declares the drawers void).
 *
 * The port's: nothing while the host draws nothing (host.h's hostDraws, a
 * watched replay going fast). */
#ifndef PORT_DRAWFRAME_H
#define PORT_DRAWFRAME_H
#include "host.h"
#include "x32code.h"

#include <algorithm>
#include <cstdint>

namespace drawframe {

const u32 FRAME_FILLS = 1;

inline s32 dword(const u8 *p)
{
    return static_cast<s32>(static_cast<u32>(p[0]) | static_cast<u32>(p[1]) << 8 | static_cast<u32>(p[2]) << 16 |
                            static_cast<u32>(p[3]) << 24);
}

/* A pixel of the remapping drawers (shpr32.asm, shpmr32.asm): table[in] of
 * 0FFh draws in as it is; any other entry n draws table[n*256+out]. */
inline void remap(u8 &out, u8 in, const u8 *table)
{
    const u8 n = table[in];
    out = n == 0xff ? in : table[(n * 256) + out];
}

/* Draws frame with its origin at (x,y); mirrored puts column 0 at the right.
 * put(pixel of the viewport, pixel of the frame) draws one pixel. */
template <bool mirrored, class Put> void draw(const ViewportPixels &destination, s32 x, s32 y, const u8 *frame, Put put)
{
    if (!hostDraws()) {
        return;
    }
    const s32 width = dword(frame + 4);
    const s32 height = dword(frame + 8);
    /* The frame's top left corner on the destination; mirrored, the origin
     * counts from the right. */
    const s32 left = mirrored ? x - (width - 1 - dword(frame + 0xc)) : x - dword(frame + 0xc);
    const s32 top = y - dword(frame + 0x10);
    if (left > destination.right || left + width - 1 < destination.left || top > destination.bottom ||
        top + height - 1 < destination.top) {
        return;
    }

    /* The rows of the frame that show, from skipTop; the columns, from
     * first up to end. Cutting the viewport's left side cuts the start of the
     * rows, or their end when mirrored. */
    const s32 skipTop = std::max(destination.top - top, 0);
    const s32 rows = height - skipTop - std::max(top + height - 1 - destination.bottom, 0);
    const s32 cutLeft = std::max(destination.left - left, 0);
    const s32 cutRight = std::max(left + width - 1 - destination.right, 0);
    const s32 first = mirrored ? cutRight : cutLeft;
    const s32 end = width - (mirrored ? cutLeft : cutRight);
    const bool fills = dword(frame) & FRAME_FILLS;
    std::int64_t line = static_cast<std::int64_t>(destination.rows[top + skipTop]) + left;
    const s32 pitch = rows > 1 ? static_cast<s32>(destination.rows[1] - destination.rows[0]) : 0;

    const u8 *table = frame + 0x14 + (4 * skipTop);
    for (s32 k = 0; k < rows; k++, line += pitch) {
        const u8 *entry = table + (4 * k);
        const u8 *p = entry + static_cast<u32>(dword(entry));
        s32 column = 0;
        while (column < end) {
            column += *p++;
            if (column >= end) {
                break;
            }
            s32 length = *p++;
            bool fill = false;
            if (fills) {
                fill = length & 1;
                length >>= 1;
            }
            const s32 from = std::max(column, first);
            const s32 to = std::min(column + length, end);
            for (s32 c = from; c < to; c++) {
                put(destination.pixels[line + (mirrored ? width - 1 - c : c)], fill ? *p : p[c - column]);
            }
            p += fill ? 1 : length;
            column += length;
        }
    }
}

}
#endif
