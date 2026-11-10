/* XANDRV32.DLL 0FD5h-2010h and 25F2h-2737h: the frame on the screen, for
 * formats 2 and 3. See xanlib.h.
 *
 * The routines go through the frame four pixels at a time: two words of
 * the chroma (each for two pixels across, and a row of them for two rows)
 * and four bytes of the luma; a pixel is the colour table's entry at the
 * chroma's word or'd with the luma. So a row takes ((width/2)+1)/2 times
 * four pixels, two more than width/2*2 when width/2 is odd, and the next
 * row starts there. The original counts what is left of the 64K VGA window
 * in its groups of bytes and at its end changes the bank (0FD5h), after a
 * group that ends it or a row's skip that goes past it; the port draws on
 * the screen's memory as a whole (xanlib.h). Flag 8 draws upside down, from
 * the last line up. A grey decoder of format 3 draws its luma as it is.
 * The doubled routines write each pixel twice across, the second first,
 * and every other line. */
#include "xandrv.h"
#include "xanlib.h"

namespace xan {

namespace {

/* The colour table's index of a pixel: the luma's byte or'd with the
 * chroma's word, read in this order or the other. */
u32 lumaFirst(const u8 *luma, const u8 *chroma)
{
    u32 index = byte(luma);
    index |= word(chroma);
    return index;
}

u32 chromaFirst(const u8 *chroma, const u8 *luma)
{
    const u32 index = word(chroma);
    return byte(luma) | index;
}

}

/* 25F2h */
u32 draw(u8 *decoder, u8 *screen, u32 pitch, u32 flags)
{
    if (!isDecoder(decoder)) {
        return 0;
    }
    if (screen == nullptr) {
        return 0;
    }
    if ((flags & 1) != 0U) {
        if (dword(decoder + TABLE) == 0U) {
            data.error = 1;
            return 0;
        }
        const u32 area = dword(decoder + AREA);
        u8 *at = plane(decoder, LUMA);
        for (u32 k = area >> 2; k != 0U; k--) {
            setDword(at, 0x2f2f2f2f);
            at += 4;
        }
        for (u32 k = area & 3; k != 0U; k--) {
            setByte(at++, 0x2f);
        }
    }
    u32 step = pitch;
    if ((flags & 4) == 0U) {
        step = dword(decoder + WIDTH);
        if ((flags & 2) != 0U) {
            step += step;
        }
    }
    switch (dword(decoder + FORMAT)) {
    case FORMAT_16:
        if ((flags & 2) != 0U) {
            draw16Double(decoder, screen, step, flags);
        } else {
            draw16(decoder, screen, step, flags);
        }
        break;
    case FORMAT_8:
        if ((flags & 2) != 0U) {
            draw8Double(decoder, screen, step, flags);
        } else {
            draw8(decoder, screen, step, flags);
        }
        break;
    default:
        break;
    }
    return 1;
}

/* 0FFAh: format 3, pitch bytes a line. */
void draw8(u8 *decoder, u8 *screen, u32 pitch, u32 flags)
{
    const u32 height = dword(decoder + HEIGHT);
    const u32 width = dword(decoder + WIDTH);
    const u8 *chroma = plane(decoder, CHROMA);
    const u8 *table = plane(decoder, TABLE);
    const u8 *luma = plane(decoder, LUMA);
    const u32 kind = dword(decoder + FLAGS);
    u8 *at = screen;
    if ((kind & 1) != 0U) {
        if ((flags & 8) != 0U) {
            if (height == 0U) {
                return;
            }
            u8 *line = at + ((height - 1) * pitch);
            u32 rows = height;
            do {
                u8 *to = line;
                const u8 *from = luma;
                for (u32 k = width >> 2; k != 0U; k--) {
                    setDword(to, dword(from));
                    to += 4;
                    from += 4;
                }
                for (u32 k = width & 3; k != 0U; k--) {
                    setByte(to++, byte(from++));
                }
                luma += width;
                line -= pitch;
            } while (--rows != 0U);
            return;
        }
        const u32 half = width >> 1;
        const u32 skip = pitch - (2 * half);
        if (height == 0U) {
            return;
        }
        u32 row = 0;
        do {
            if (half != 0U) {
                u32 groups = (half + 1) >> 1;
                do {
                    const u32 four = dword(luma);
                    at += 4;
                    setDword(at - 4, four);
                    luma += 4;
                } while (--groups != 0U);
            }
            at += skip;
        } while (++row < height);
        return;
    }
    const u32 half = width >> 1;
    const u32 rowBytes = 2 * half;
    const u8 *line = chroma - rowBytes;
    if ((flags & 8) != 0U) {
        if (height == 0U) {
            return;
        }
        at += (height - 1) * pitch;
        const u32 back = pitch + (2 * half);
        u32 row = 0;
        do {
            if ((row & 1) == 0U) {
                line += rowBytes;
            }
            if (half != 0U) {
                const u8 *c = line;
                u32 groups = (half + 1) >> 1;
                do {
                    setByte(at, byte(table + (lumaFirst(luma, c) & 0xffff)));
                    setByte(at + 1, byte(table + (lumaFirst(luma + 1, c) & 0xffff)));
                    setByte(at + 2, byte(table + (lumaFirst(luma + 2, c + 2) & 0xffff)));
                    setByte(at + 3, byte(table + (lumaFirst(luma + 3, c + 2) & 0xffff)));
                    c += 4;
                    luma += 4;
                    at += 4;
                } while (--groups != 0U);
            }
            at -= back;
        } while (++row < height);
        return;
    }
    const u32 skip = pitch - (2 * half);
    if (height == 0U) {
        return;
    }
    u32 row = 0;
    do {
        if ((row & 1) == 0U) {
            line += rowBytes;
        }
        if (half != 0U) {
            const u8 *c = line;
            u32 groups = (half + 1) >> 1;
            do {
                at += 4;
                setByte(at - 4, byte(table + (lumaFirst(luma, c) & 0xffff)));
                setByte(at - 3, byte(table + (lumaFirst(luma + 1, c) & 0xffff)));
                setByte(at - 2, byte(table + (lumaFirst(luma + 2, c + 2) & 0xffff)));
                setByte(at - 1, byte(table + (lumaFirst(luma + 3, c + 2) & 0xffff)));
                c += 4;
                luma += 4;
            } while (--groups != 0U);
        }
        at += skip;
    } while (++row < height);
}

/* 1338h: format 2, pitch pixels a line. */
void draw16(u8 *decoder, u8 *screen, u32 pitch, u32 flags)
{
    const u32 width = dword(decoder + WIDTH);
    const u32 height = dword(decoder + HEIGHT);
    const u8 *chroma = plane(decoder, CHROMA);
    const u8 *luma = plane(decoder, LUMA);
    const u8 *table = plane(decoder, TABLE);
    u8 *at = screen;
    const u32 half = width >> 1;
    const u32 rowBytes = 2 * half;
    const u8 *line = chroma - rowBytes;
    if ((flags & 8) != 0U) {
        if (height == 0U) {
            return;
        }
        at += 2 * ((height - 1) * pitch);
        const u32 back = pitch + (2 * half);
        u32 row = 0;
        do {
            if ((row & 1) == 0U) {
                line += rowBytes;
            }
            if (half != 0U) {
                const u8 *c = line;
                u32 groups = (half + 1) >> 1;
                do {
                    setWord(at, word(table + (2 * (lumaFirst(luma, c) & 0xffff))));
                    setWord(at + 2, word(table + (2 * (lumaFirst(luma + 1, c) & 0xffff))));
                    setWord(at + 4, word(table + (2 * (lumaFirst(luma + 2, c + 2) & 0xffff))));
                    setWord(at + 6, word(table + (2 * (lumaFirst(luma + 3, c + 2) & 0xffff))));
                    c += 4;
                    luma += 4;
                    at += 8;
                } while (--groups != 0U);
            }
            at -= 2 * back;
        } while (height > ++row);
        return;
    }
    const u32 skip = pitch - (2 * half);
    if (height == 0U) {
        return;
    }
    u32 row = 0;
    do {
        if ((row & 1) == 0U) {
            line += rowBytes;
        }
        if (half != 0U) {
            const u8 *c = line;
            u32 groups = (half + 1) >> 1;
            do {
                at += 8;
                setWord(at - 8, word(table + (2 * (lumaFirst(luma, c) & 0xffff))));
                setWord(at - 6, word(table + (2 * (lumaFirst(luma + 1, c) & 0xffff))));
                setWord(at - 4, word(table + (2 * (lumaFirst(luma + 2, c + 2) & 0xffff))));
                setWord(at - 2, word(table + (2 * (lumaFirst(luma + 3, c + 2) & 0xffff))));
                c += 4;
                luma += 4;
            } while (--groups != 0U);
        }
        at += 2 * skip;
    } while (height > ++row);
}

/* 1A4Fh: format 3 doubled. */
void draw8Double(u8 *decoder, u8 *screen, u32 pitch, u32 flags)
{
    const u32 height = dword(decoder + HEIGHT);
    const u32 width = dword(decoder + WIDTH);
    const u8 *luma = plane(decoder, LUMA);
    const u8 *table = plane(decoder, TABLE);
    const u8 *chroma = plane(decoder, CHROMA);
    u8 *at = screen;
    const u32 half = width >> 1;
    auto twice = [](u8 *to, u8 value) {
        setByte(to + 1, value);
        setByte(to, value);
    };
    if ((flags & 8) != 0U) {
        if (height == 0U) {
            return;
        }
        at += 2 * ((height - 1) * pitch);
        const u32 back = 2 * (pitch + 2 * half);
        if ((dword(decoder + FLAGS) & 1) != 0U) {
            u32 rows = height;
            do {
                if (half != 0U) {
                    u32 groups = (half + 1) >> 1;
                    do {
                        at += 8;
                        luma += 4;
                        twice(at - 8, byte(luma - 4));
                        twice(at - 6, byte(luma - 3));
                        twice(at - 4, byte(luma - 2));
                        twice(at - 2, byte(luma - 1));
                    } while (--groups != 0U);
                }
                at -= back;
            } while (--rows != 0U);
            return;
        }
        const u8 *line = chroma - (2 * half);
        u32 row = 0;
        do {
            if ((row & 1) == 0U) {
                line += 2 * half;
            }
            if (half != 0U) {
                const u8 *c = line;
                u32 groups = (half + 1) >> 1;
                do {
                    twice(at, byte(table + (lumaFirst(luma, c) & 0xffff)));
                    twice(at + 2, byte(table + (lumaFirst(luma + 1, c) & 0xffff)));
                    twice(at + 4, byte(table + (lumaFirst(luma + 2, c + 2) & 0xffff)));
                    twice(at + 6, byte(table + (lumaFirst(luma + 3, c + 2) & 0xffff)));
                    c += 4;
                    luma += 4;
                    at += 8;
                } while (--groups != 0U);
            }
            at -= back;
        } while (height > ++row);
        return;
    }
    const u32 skip = 2 * (pitch - 2 * half);
    if ((dword(decoder + FLAGS) & 1) != 0U) {
        if (height == 0U) {
            return;
        }
        u32 rows = height;
        do {
            if (half != 0U) {
                u32 groups = (half + 1) >> 1;
                do {
                    at += 8;
                    twice(at - 8, byte(luma));
                    twice(at - 6, byte(luma + 1));
                    twice(at - 4, byte(luma + 2));
                    twice(at - 2, byte(luma + 3));
                    luma += 4;
                } while (--groups != 0U);
            }
            at += skip;
        } while (--rows != 0U);
        return;
    }
    if (height == 0U) {
        return;
    }
    const u8 *line = chroma;
    u32 row = 0;
    do {
        if ((row & 1) != 0U) {
            line -= 2 * half;
        }
        if (half != 0U) {
            u32 groups = (half + 1) >> 1;
            do {
                at += 8;
                twice(at - 8, byte(table + chromaFirst(line, luma)));
                twice(at - 6, byte(table + chromaFirst(line, luma + 1)));
                twice(at - 4, byte(table + chromaFirst(line + 2, luma + 2)));
                twice(at - 2, byte(table + chromaFirst(line + 2, luma + 3)));
                line += 4;
                luma += 4;
            } while (--groups != 0U);
        }
        at += skip;
    } while (height > ++row);
}

/* 1D9Eh: format 2 doubled. */
void draw16Double(u8 *decoder, u8 *screen, u32 pitch, u32 flags)
{
    const u32 height = dword(decoder + HEIGHT);
    const u32 width = dword(decoder + WIDTH);
    const u8 *chroma = plane(decoder, CHROMA);
    const u8 *luma = plane(decoder, LUMA);
    const u8 *table = plane(decoder, TABLE);
    u8 *at = screen;
    const u32 half = width >> 1;
    auto twice = [](u8 *to, u16 value) {
        setWord(to + 2, value);
        setWord(to, value);
    };
    if ((flags & 8) != 0U) {
        const u8 *line = chroma - (2 * half);
        if (height == 0U) {
            return;
        }
        at += 4 * ((height - 1) * pitch);
        const u32 back = 2 * (pitch + 2 * half);
        u32 row = 0;
        do {
            if ((row & 1) == 0U) {
                line += 2 * half;
            }
            if (half != 0U) {
                const u8 *c = line;
                u32 groups = (half + 1) >> 1;
                do {
                    twice(at, word(table + (2 * (lumaFirst(luma, c) & 0xffff))));
                    twice(at + 4, word(table + (2 * (lumaFirst(luma + 1, c) & 0xffff))));
                    twice(at + 8, word(table + (2 * (lumaFirst(luma + 2, c + 2) & 0xffff))));
                    twice(at + 12, word(table + (2 * (lumaFirst(luma + 3, c + 2) & 0xffff))));
                    c += 4;
                    luma += 4;
                    at += 16;
                } while (--groups != 0U);
            }
            at -= 2 * back;
        } while (++row < height);
        return;
    }
    const u32 skip = 2 * (pitch - 2 * half);
    if (height == 0U) {
        return;
    }
    const u8 *line = chroma;
    u32 row = 0;
    do {
        if ((row & 1) != 0U) {
            line -= 2 * half;
        }
        if (half != 0U) {
            u32 groups = (half + 1) >> 1;
            do {
                at += 16;
                twice(at - 16, word(table + (2 * chromaFirst(line, luma))));
                twice(at - 12, word(table + (2 * chromaFirst(line, luma + 1))));
                twice(at - 8, word(table + (2 * chromaFirst(line + 2, luma + 2))));
                twice(at - 4, word(table + (2 * chromaFirst(line + 2, luma + 3))));
                line += 4;
                luma += 4;
            } while (--groups != 0U);
        }
        at += 2 * skip;
    } while (++row < height);
}

}
