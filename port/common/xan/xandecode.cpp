/* XANDRV32.DLL 0030h-043Ch and 24DBh-25F1h: a frame's decoding. See
 * xanlib.h.
 *
 * A chunk: the frame's type (0 a whole frame, 1 a change of the one
 * before), where its chroma is (from chunk+4, 0 for none), then the
 * luma's corrections (one from the luma's data, type 1 two) and its
 * Huffman codes. The luma is of 6 bits: the codes are steps of the pixels
 * of the even columns, the odd ones lie between them, and the corrections
 * move the odd ones. The chroma is a word for 2x2 pixels, the index of the
 * colour table's high bits (the luma is the low 6). */
#include "xandrv.h"
#include "xanlib.h"

namespace xan {

namespace {

/* The corrections of the odd pixels from at on: the bytes at in, doubled
 * as dwords (a byte's top bit carries into the next), added to every other
 * pixel. count counts them in twos, a zero dword skips four. */
void correct(u8 *at, const u8 *in, s32 count)
{
    for (;;) {
        u32 x = dword(in) * 2;
        if (x == 0U) {
            count -= 2;
            if (count <= 0) {
                return;
            }
            x = dword(in + 4) * 2;
            if (x == 0U) {
                in += 8;
                at += 16;
                count -= 2;
                if (count > 0) {
                    continue;
                }
                return;
            }
        } else {
            setByte(at, static_cast<u8>((byte(at) + static_cast<u8>(x)) & 0x3f));
            setByte(at + 2, static_cast<u8>((byte(at + 2) + static_cast<u8>(x >> 8)) & 0x3f));
            if (--count <= 0) {
                return;
            }
            x >>= 16;
            setByte(at + 4, static_cast<u8>((byte(at + 4) + static_cast<u8>(x)) & 0x3f));
            setByte(at + 6, static_cast<u8>((byte(at + 6) + static_cast<u8>(x >> 8)) & 0x3f));
            if (--count <= 0) {
                return;
            }
            x = dword(in + 4) * 2;
            if (x == 0U) {
                in += 8;
                at += 16;
                count -= 2;
                if (count > 0) {
                    continue;
                }
                return;
            }
        }
        setByte(at + 8, static_cast<u8>((byte(at + 8) + static_cast<u8>(x)) & 0x3f));
        setByte(at + 10, static_cast<u8>((byte(at + 10) + static_cast<u8>(x >> 8)) & 0x3f));
        if (--count <= 0) {
            return;
        }
        x >>= 16;
        setByte(at + 12, static_cast<u8>((byte(at + 12) + static_cast<u8>(x)) & 0x3f));
        setByte(at + 14, static_cast<u8>((byte(at + 14) + static_cast<u8>(x >> 8)) & 0x3f));
        in += 8;
        at += 16;
        if (--count <= 0) {
            return;
        }
    }
}

/* Type 1's two streams of corrections, the second second bytes after the
 * first: their dwords' sum, tested for 0 before it is doubled. */
void correctTwo(u8 *at, const u8 *in, u32 second, s32 count)
{
    for (;;) {
        u32 x = dword(in);
        x += dword(in + second);
        if (x == 0U) {
            count -= 2;
            if (count <= 0) {
                return;
            }
            x = dword(in + 4);
            x += dword(in + second + 4);
            if (x == 0U) {
                in += 8;
                at += 16;
                count -= 2;
                if (count > 0) {
                    continue;
                }
                return;
            }
        } else {
            x *= 2;
            setByte(at, static_cast<u8>((byte(at) + static_cast<u8>(x)) & 0x3f));
            setByte(at + 2, static_cast<u8>((byte(at + 2) + static_cast<u8>(x >> 8)) & 0x3f));
            if (--count <= 0) {
                return;
            }
            x >>= 16;
            setByte(at + 4, static_cast<u8>((byte(at + 4) + static_cast<u8>(x)) & 0x3f));
            setByte(at + 6, static_cast<u8>((byte(at + 6) + static_cast<u8>(x >> 8)) & 0x3f));
            if (--count <= 0) {
                return;
            }
            x = dword(in + 4);
            x += dword(in + second + 4);
            if (x == 0U) {
                in += 8;
                at += 16;
                count -= 2;
                if (count > 0) {
                    continue;
                }
                return;
            }
        }
        x *= 2;
        setByte(at + 8, static_cast<u8>((byte(at + 8) + static_cast<u8>(x)) & 0x3f));
        setByte(at + 10, static_cast<u8>((byte(at + 10) + static_cast<u8>(x >> 8)) & 0x3f));
        if (--count <= 0) {
            return;
        }
        x >>= 16;
        setByte(at + 12, static_cast<u8>((byte(at + 12) + static_cast<u8>(x)) & 0x3f));
        setByte(at + 14, static_cast<u8>((byte(at + 14) + static_cast<u8>(x >> 8)) & 0x3f));
        in += 8;
        at += 16;
        if (--count <= 0) {
            return;
        }
    }
}

/* A chroma value of the table: below 80h less 2, at least 0. */
u8 chromaValue(u8 value)
{
    if (value < 0x80) {
        value = static_cast<u8>(value - 2);
        if (value > 0x80) {
            value = 0;
        }
    }
    return value;
}

}

/* 256Bh */
u32 decode(u8 *decoder, u8 *chunk, u32 flags)
{
    if (!isDecoder(decoder)) {
        return 0;
    }
    if ((dword(decoder + FLAGS) & 1) == 0U) {
        const u32 at = dword(chunk + 4);
        if (at != 0U) {
            if (static_cast<s32>(dword(decoder + BITS)) >= 4) {
                chromaTable(chunk + 4 + at);
            }
            /* Read again: the table's rewriting may have moved it. */
            chroma(decoder, chunk + 4 + dword(chunk + 4));
        }
    }
    if (dword(chunk) == 0U) {
        lumaType0(decoder, chunk + 8, flags);
    } else {
        lumaType1(decoder, chunk + 8, flags);
    }
    return 1;
}

/* 24DBh: the chroma's table of the movies of 4 bits and more, in place:
 * each pair of chromas (a byte each, from +4, as many as the byte at +2
 * says) becomes the index of their high 5 bits, bits 15-11 and 10-6. */
void chromaTable(u8 *packed)
{
    u32 count = byte(packed + 2);
    u8 *at = packed + 4;
    if (count == 0U) {
        return;
    }
    do {
        const u8 first = chromaValue(byte(at));
        const u8 second = chromaValue(byte(at + 1));
        setWord(at, static_cast<u16>((((second & 0xf8) << 8) | static_cast<u16>(first << 3)) & 0xffc0));
        at += 2;
    } while (--count != 0U);
}

/* 0030h: the chroma. packed: the mode, the count of the table's words, the
 * table (read from packed+2, so that its word 0 is the count), the bytes
 * of indexes LZ-packed. Mode 0: an index for each 4x4 pixels (2x2 words),
 * 0 keeping them, then the last rows again to the end; otherwise one for
 * each word, from the last. */
void chroma(u8 *decoder, const u8 *packed)
{
    u8 *scratch = plane(decoder, SCRATCH);
    const u32 head = dword(packed);
    const u8 *table = packed + 2;
    const u32 count = unpack(scratch, table + (2 * (head >> 16)) + 2);
    u8 *words = plane(decoder, CHROMA);
    if ((head & 0xffff) != 0U) {
        s32 at = static_cast<s32>(count) - 2;
        if (at < 0) {
            return;
        }
        do {
            const u16 indexes = word(scratch + static_cast<u32>(at));
            const u32 high = word(table + (2 * (indexes >> 8)));
            const u32 low = word(table + (2 * (indexes & 0xff)));
            setDword(words + (2 * static_cast<u32>(at)), high << 16 | low);
            at -= 2;
        } while (at >= 0);
        return;
    }
    u8 rows = static_cast<u8>(dword(decoder + HEIGHT) >> 2);
    const u32 width = dword(decoder + WIDTH);
    u8 *at = words;
    const u8 *in = scratch;
    do {
        u32 columns = width >> 2;
        do {
            const u8 index = byte(in);
            if (index != 0U) {
                u32 value = word(table + (2 * index));
                value |= value << 16;
                setDword(at, value);
                setDword(at + width, value);
            }
            at += 4;
            in++;
        } while (--columns != 0U);
        at += width;
    } while (--rows != 0U);
    const u32 height = dword(decoder + HEIGHT);
    const u32 extra = (((height + 1) & ~1U) - (height & ~3U)) >> 1;
    if (extra == 0U) {
        return;
    }
    const u8 *row = at - width;
    for (s32 left = static_cast<s32>(extra); left > 0; left--) {
        const u8 *from = row;
        for (u32 k = width >> 2; k != 0U; k--) {
            setDword(at, dword(from));
            at += 4;
            from += 4;
        }
    }
}

/* 00F6h: a whole frame's luma. The first row: steps from the one before,
 * the second's pixels from the first's; the others from the row above. */
void lumaType0(u8 *decoder, const u8 *chunk, u32 flags)
{
    u8 *scratch = plane(decoder, SCRATCH);
    huffman(scratch, chunk + 4);
    u8 *out = plane(decoder, LUMA);
    const u8 *in = scratch;
    u8 low = byte(in++);
    u8 high = low;
    low = static_cast<u8>(low + low);
    setByte(out++, low);
    const u32 width = dword(decoder + WIDTH);
    s32 count = static_cast<s32>((width - 2) >> 1);
    do {
        low = high;
        high = static_cast<u8>((high + byte(in++)) & 0x1f);
        low = static_cast<u8>(low + high);
        setByte(out++, low);
        low = high;
        low = static_cast<u8>(low + low);
        setByte(out++, low);
    } while (--count > 0);
    setByte(out++, low);
    s32 rows = static_cast<s32>(dword(decoder + HEIGHT)) - 1;
    const u32 back = 0U - width;
    do {
        low = static_cast<u8>(byte(out - width) >> 1);
        low = static_cast<u8>((low + byte(in++)) & 0x1f);
        high = static_cast<u8>(low + low);
        setByte(out++, high);
        high = low;
        s32 column = static_cast<s32>(back + 2) >> 1;
        do {
            low = static_cast<u8>(byte(out - width + 1) >> 1);
            low = static_cast<u8>((low + byte(in++)) & 0x1f);
            high = static_cast<u8>(high + low);
            setByte(out++, high);
            high = low;
            low = static_cast<u8>(low + low);
            setByte(out++, low);
        } while (++column < 0);
        setByte(out++, low);
    } while (--rows > 0);
    if ((flags & 1) != 0U) {
        return;
    }
    const u32 offset = dword(chunk);
    if (offset == 0U) {
        return;
    }
    scratch = plane(decoder, SCRATCH);
    const u32 count2 = unpack(scratch, chunk + offset);
    correct(plane(decoder, LUMA) + 1, scratch, static_cast<s32>(count2 >> 1));
}

/* 0249h: a change of the luma: each code doubled onto the pixel of an even
 * column, the odd ones halfway between. */
void lumaType1(u8 *decoder, const u8 *chunk, u32 flags)
{
    u8 *scratch = plane(decoder, SCRATCH);
    huffman(scratch, chunk + 8);
    u8 *out = plane(decoder, LUMA);
    s32 rows = static_cast<s32>(dword(decoder + HEIGHT));
    const u32 pairs = (dword(decoder + WIDTH) - 2) >> 1;
    const u8 *in = scratch;
    do {
        u8 low = byte(in++);
        low = static_cast<u8>(low + low);
        low = static_cast<u8>((low + byte(out)) & 0x3f);
        setByte(out, low);
        out++;
        s32 count = static_cast<s32>(pairs);
        do {
            u8 high = byte(in++);
            high = static_cast<u8>(high + high);
            high = static_cast<u8>((high + byte(out + 1)) & 0x3f);
            setByte(out + 1, high);
            low = static_cast<u8>(static_cast<u8>(low + high) >> 1);
            setByte(out, low);
            out += 2;
            low = high;
        } while (--count > 0);
        setByte(out, low);
        out++;
    } while (--rows > 0);
    if ((flags & 1) != 0U) {
        return;
    }
    const u32 offset = dword(chunk);
    if (offset == 0U) {
        return;
    }
    scratch = plane(decoder, SCRATCH);
    u32 count = unpack(scratch, chunk + offset);
    const u32 second = dword(chunk + 4);
    if (second == 0U) {
        correct(plane(decoder, LUMA) + 1, scratch, static_cast<s32>(count >> 1));
        return;
    }
    count = unpack(scratch + count, chunk + second);
    correctTwo(plane(decoder, LUMA) + 1, scratch, count, static_cast<s32>(count >> 1));
}

}
