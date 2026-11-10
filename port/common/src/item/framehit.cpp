/* framehit.asm (1158:01c8-028c) in C++: whether the pixel at x,y of a shape
 * frame placed with its origin at left,top is drawn.
 *
 * The frame (shape.h ShapeFrame), all little-endian dwords:
 *   +00 flags, bit 0 set when the runs are packed
 *   +04 the width, +08 the height, +0C, +10 the origin
 *   +14 the offsets of the rows, each from its own place in the table
 * A row is pairs of a skip byte and a run byte; a packed run with bit 0 set
 * is one colour byte repeated, otherwise the run's bytes follow.
 *
 * The arithmetic follows the original's 32-bit registers: sums wrap at 32
 * bits and compare signed, except that JL after ADD tests the sign of the
 * exact sum. The original reads the runs through the 16-bit SI; frames that
 * lie within one 64 KB segment, as the game's do, read the same bytes.
 *
 * The callers pass Points of 16-bit ints (DLITEM.C, GUMPSHAP.C), so the sums
 * never pass 32 bits. Past that the original goes on with a row or column
 * that wrapped negative and reads whatever lies in its segment; this version
 * misses instead. */
#include "hittest.h"

namespace {
s32 dword(const u8 *p)
{
    return static_cast<s32>(static_cast<u32>(p[0]) | static_cast<u32>(p[1]) << 8 | static_cast<u32>(p[2]) << 16 |
                            static_cast<u32>(p[3]) << 24);
}

s32 wrapAdd(s32 a, s32 b)
{
    return static_cast<s32>(static_cast<u32>(a) + static_cast<u32>(b));
}
}

bool frameHit(const u8 *frame, s32 left, s32 top, s32 x, s32 y)
{
    const s32 packed = dword(frame);
    const s32 width = dword(frame + 4);
    const s32 height = dword(frame + 8);
    s32 count = 0;

    x = static_cast<s32>(static_cast<u32>(x) - static_cast<u32>(left));
    y = static_cast<s32>(static_cast<u32>(y) - static_cast<u32>(top));
    /* ADD then JL: the sum itself is below 0. */
    if (static_cast<std::int64_t>(dword(frame + 0x10)) + y < 0) {
        return false;
    }
    const s32 row = wrapAdd(dword(frame + 0x10), y);
    if (row < 0 || row >= height) {
        return false;
    }
    if (static_cast<std::int64_t>(dword(frame + 0xc)) + x < 0) {
        return false;
    }
    const s32 column = wrapAdd(dword(frame + 0xc), x);
    if (column < 0 || column >= width) {
        return false;
    }
    const u8 *entry = frame + 0x14 + (static_cast<u32>(row) * 4);
    const u8 *p = entry + static_cast<u32>(dword(entry));
    for (;;) {
        count = wrapAdd(count, *p++);
        if (count > column || count >= width) {
            return false;
        }
        u32 run = *p++;
        u32 size = run;
        if ((packed & 1) != 0) {
            const u32 fill = run & 1;
            run >>= 1;
            size = (fill != 0U) ? 1 : run;
        }
        count = wrapAdd(count, static_cast<s32>(run));
        if (count > column) {
            return true;
        }
        p += size;
    }
}

/* The original's form (hittest.h). */
bool frameHit(ShapeFrame *frame, s32 left, s32 top, s32 x, s32 y)
{
    return frameHit(reinterpret_cast<const u8 *>(frame), left, top, x, y);
}
