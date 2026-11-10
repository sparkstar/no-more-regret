/* XANDRV32.DLL 043Dh-0DBAh: the decoder's making and its colours. See
 * xanlib.h. */
#include "xandrv.h"
#include "gamestate.h"
#include "xanlib.h"

namespace xan {

Data &data = gameGlobal<Data>({0});

namespace {

/* 066Fh: the shift of a mask (its zeros below, up to 32) and its width
 * (the ones after them). */
void maskBits(u32 mask, u32 &width, u32 &shift)
{
    u32 n = 0;
    while (((mask & 1) == 0U) && n < 32) {
        mask >>= 1;
        n++;
    }
    shift = n;
    while (((mask & 1) != 0U) && n < 32) {
        mask >>= 1;
        n++;
    }
    width = n - shift;
}

s32 clampColor(s32 value)
{
    if (value <= 0) {
        return 0;
    }
    return value < 0xff ? value : 0xff;
}

}

/* 043Dh: whether decoder is one (its header's size first); error 4 when
 * not. */
bool isDecoder(const u8 *decoder)
{
    if ((decoder != nullptr) && dword(decoder + MAGIC) == HEADER_SIZE) {
        return true;
    }
    data.error = 4;
    return false;
}

u8 *plane(u8 *decoder, u32 field)
{
    const u32 offset = dword(decoder + field);
    return (offset != 0U) ? decoder + offset : nullptr;
}

/* 04AAh. The planes: the luma (width x height), the chroma (a word for 2x2
 * pixels, (width/2) x ((height+1)/2), none grey), the scratch buffer the
 * unpacking goes to (width x height + 4) and the colour table (format 2
 * 20000h, format 3 10000h). */
u8 *create(u32 width, u32 height, u32 format, const u8 *best, u32 flags, u32 red, u32 green, u32 blue, u32 range,
           u32 bits, u8 *memory)
{
    const u32 chromaRows = static_cast<u32>(static_cast<s32>(height + 1) / 2);
    const u32 chromaColumns = static_cast<u32>(static_cast<s32>(width) / 2);
    u32 chromaSize = 2 * (chromaRows * chromaColumns);
    const u32 area = height * width;
    const u32 scratchSize = area + 4;
    u32 tableSize = 0x10000;
    if (bits > 4) {
        data.error = 7;
        return nullptr;
    }
    /* The caller's own table (flags 2): as without one. */
    if ((flags & 2) != 0U) {
        data.error = 2;
        return nullptr;
    }
    switch (format) {
    case FORMAT_16:
        tableSize += tableSize;
        break;
    case FORMAT_8:
        if ((flags & 1) != 0U) {
            tableSize = 0;
            chromaSize = 0;
        }
        break;
    default:
        /* 4 and 5 (32-bit pixels) too. */
        data.error = 1;
        return nullptr;
    }
    const u32 size = scratchSize + tableSize + area + chromaSize + HEADER_SIZE;
    if (memory == nullptr) {
        data.error = 3;
        return nullptr;
    }
    for (u32 k = 0; k < size; k++) {
        setByte(memory + k, 0);
    }
    setDword(memory + MAGIC, HEADER_SIZE);
    setDword(memory + SIZE, size);
    setDword(memory + BITS, bits);
    setDword(memory + WIDTH, width);
    setDword(memory + HEIGHT, height);
    setDword(memory + AREA, area);
    setDword(memory + FORMAT, format);
    setDword(memory + FLAGS, flags);
    setDword(memory + RED, red);
    setDword(memory + GREEN, green);
    setDword(memory + BLUE, blue);
    u32 at = HEADER_SIZE;
    setDword(memory + LUMA, at);
    at += area;
    setDword(memory + CHROMA, at);
    at += chromaSize;
    setDword(memory + SCRATCH, at);
    at += scratchSize;
    setDword(memory + TABLE, at);
    if (chromaSize == 0U) {
        setDword(memory + CHROMA, 0);
    }
    if (tableSize == 0U) {
        setDword(memory + TABLE, 0);
    }
    setColors(memory, best, red, green, blue, range, 0);
    return memory;
}

/* 047Ah */
u8 *image(u8 *decoder)
{
    if (!isDecoder(decoder)) {
        return nullptr;
    }
    if ((dword(decoder + FLAGS) & 2) != 0U) {
        data.error = 8;
        return nullptr;
    }
    return plane(decoder, TABLE);
}

/* 0C53h: the masks, which must not overlap (format 2 their low words), and
 * the colour table: format 2's made from them, format 3's unpacked from
 * best. */
u32 setColors(u8 *decoder, const u8 *best, u32 red, u32 green, u32 blue, u32 range, u32 unused)
{
    if (!isDecoder(decoder)) {
        return 0;
    }
    if (unused != 0U) {
        data.error = 6;
        return 0;
    }
    const u32 format = dword(decoder + FORMAT);
    if (format != FORMAT_8) {
        if (format == FORMAT_16) {
            red &= 0xffff;
            green &= 0xffff;
            blue &= 0xffff;
        }
        if ((((red | blue) & green) | (red & blue)) != 0) {
            data.error = 5;
            return 0;
        }
    }
    setDword(decoder + RED, red);
    setDword(decoder + GREEN, green);
    const u32 flags = dword(decoder + FLAGS);
    setDword(decoder + BLUE, blue);
    if ((flags & 2) != 0U) {
        data.error = 2;
        return 0;
    }
    switch (format) {
    case FORMAT_16:
        hicolorTable(decoder, best, range);
        return 1;
    case FORMAT_8:
        if ((flags & 1) != 0U) {
            return 1;
        }
        if (best == nullptr) {
            data.error = 2;
            return 0;
        }
        unpack(plane(decoder, TABLE), best);
        return 1;
    default:
        data.error = 1;
        return 0;
    }
}

/* 0985h: format 2's table, 32 x 32 x 64 pixels: two chromas from -128 to
 * 120 in steps of 8 (range+1 scales them), a luma (best's 256 bytes in
 * steps of 4, or the step itself), the colours of their sum clamped to a
 * byte, each cut to its mask's width and moved to its place. Grey
 * decoders have no chroma. Divisions by 256 go towards 0. */
void hicolorTable(u8 *decoder, const u8 *best, u32 range)
{
    const u32 n = (range & 0xff) + 1;
    u32 drop[3];
    u32 shift[3];
    s32 round[3] = {0, 0, 0};
    for (int k = 0; k < 3; k++) {
        u32 width;
        maskBits(dword(decoder + RED + (4 * k)), width, shift[k]);
        const s32 d = 8 - static_cast<s32>(width);
        drop[k] = static_cast<u32>(d);
        if (d < 0) {
            drop[k] = 0;
            shift[k] -= static_cast<u32>(d);
        }
    }
    for (int k = 0; k < 3; k++) {
        if (static_cast<s32>(drop[k]) > 0) {
            round[k] = static_cast<s32>(1U << ((drop[k] - 1) & 31));
        }
    }
    s32 u = 0;
    s32 v = 0;
    s32 g = 0;
    u32 at = 0;
    const s32 step = static_cast<s32>(n * 8);
    s32 outer = -static_cast<s32>(n << 7);
    for (int i = 0; i < 32; i++) {
        s32 middle = -static_cast<s32>(n << 7);
        for (int j = 0; j < 32; j++) {
            if ((dword(decoder + FLAGS) & 1) == 0U) {
                u = outer / 256;
                v = middle / 256;
                g = -(130 * u) / 256 - 50 * v / 256;
            }
            for (u32 d = 0; d < 0x100; d += 4) {
                const s32 y = (best != nullptr) ? byte(best + d) : static_cast<s32>(d);
                const s32 r = clampColor(round[0] + u + y);
                const s32 gr = clampColor(round[1] + g + y);
                const s32 b = clampColor(round[2] + v + y);
                u16 pixel = static_cast<u16>(static_cast<u32>(b >> (drop[2] & 31)) << (shift[2] & 31));
                pixel |= static_cast<u16>(static_cast<u32>(gr >> (drop[1] & 31)) << (shift[1] & 31));
                pixel |= static_cast<u16>(static_cast<u32>(r >> (drop[0] & 31)) << (shift[0] & 31));
                setWord(plane(decoder, TABLE) + at, pixel);
                at += 2;
            }
            middle += step;
        }
        outer += step;
    }
}

u32 lastError()
{
    return data.error;
}

}
