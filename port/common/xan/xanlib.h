/* XANDRV32.DLL, the decoder of the Xan movies, ported from its 32-bit
 * machine code: what the game's four calls (xandrv.h) reach for the two
 * formats the game asks for, 16-bit colour (2) and 8-bit colour through a
 * table (3).
 *
 * The original runs flat, on linear addresses: a decoder is a block of the
 * caller's memory, a header of 3Ch bytes and its planes after it (the
 * luma, the chroma, a scratch buffer, the colour table); it reads the
 * chunks and the packed colour table where the caller has them and writes
 * the screen through the VGA window, whose banks the game changes. The
 * port keeps the block as the original lays it out, but for the planes'
 * places in the header, which are offsets from the block's start, and
 * reaches the memory through pointers; it draws on the screen's memory as
 * a whole, without the window's banks, which the game's movies never write
 * across. Formats 4 and 5 (32-bit pixels) and the caller's own colour
 * table (flags 2), which the game never asks for, it has not: they fail as
 * an unknown format and a missing table do.
 *
 * The files follow the object: xancreate.cpp 043Dh-0DBAh, xanunpack.cpp
 * 0DC1h-0FD4h, xandecode.cpp 0030h-043Ch and 24DBh-25F1h, xandraw.cpp
 * 0FFAh-2010h and 25F2h-2737h. */
#ifndef PORT_XANLIB_H
#define PORT_XANLIB_H
#include <cstdint>

namespace xan {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using s32 = std::int32_t;
using u32 = std::uint32_t;

/* The decoder's header. */
enum : u32 {
    MAGIC = 0x00, /* 3Ch, the header's size */
    SIZE = 0x04,  /* of the whole block */
    BITS = 0x08,  /* the movie's (0-4): at 4 the chroma's table is rewritten */
    WIDTH = 0x0c,
    HEIGHT = 0x10,
    AREA = 0x14, /* width * height */
    FORMAT = 0x18,
    RED = 0x1c, /* the masks of format 2's pixels */
    GREEN = 0x20,
    BLUE = 0x24,
    FLAGS = 0x28, /* 1 grey (no chroma) */
    LUMA = 0x2c,  /* the planes' offsets in the block, 0 for none */
    CHROMA = 0x30,
    SCRATCH = 0x34,
    TABLE = 0x38,
    HEADER_SIZE = 0x3c
};

/* The formats: 2 16-bit pixels through a table of 65,536 words, 3 bytes
 * through a table of 65,536 bytes. */
enum : u32 { FORMAT_16 = 2, FORMAT_8 = 3 };

/* The DLL's data: the last error (2Ch). */
struct Data {
    u32 error;
};
extern Data &data;

inline u8 byte(const u8 *at)
{
    return *at;
}

inline u16 word(const u8 *at)
{
    return static_cast<u16>(at[0] | at[1] << 8);
}

inline u32 dword(const u8 *at)
{
    return at[0] | at[1] << 8 | at[2] << 16 | static_cast<u32>(at[3]) << 24;
}

inline void setByte(u8 *at, u8 value)
{
    *at = value;
}

inline void setWord(u8 *at, u16 value)
{
    at[0] = static_cast<u8>(value);
    at[1] = static_cast<u8>(value >> 8);
}

inline void setDword(u8 *at, u32 value)
{
    for (int k = 0; k < 4; k++) {
        at[k] = static_cast<u8>(value >> 8 * k);
    }
}

/* REP MOVSB: count bytes forward, one at a time, so that a copy onto
 * itself repeats (the unpacker's back references). */
inline void copyBytes(u8 *&dest, const u8 *&source, u32 count)
{
    for (; count != 0U; count--) {
        *dest++ = *source++;
    }
}

/* xancreate.cpp */
bool isDecoder(const u8 *decoder); /* 043Dh */
/* The plane of the header's field (LUMA, CHROMA, SCRATCH, TABLE), nullptr
 * when it has none. */
u8 *plane(u8 *decoder, u32 field);
u32 setColors(u8 *decoder, const u8 *best, u32 red, u32 green, u32 blue, u32 range, u32 unused); /* 0C53h */
void hicolorTable(u8 *decoder, const u8 *best, u32 range);                                       /* 0985h */

/* xanunpack.cpp */
u32 huffman(u8 *dest, const u8 *source); /* 0DC1h */
u32 unpack(u8 *dest, const u8 *source);  /* 0EDEh */

/* xandecode.cpp */
void chromaTable(u8 *packed);                            /* 24DBh */
void chroma(u8 *decoder, const u8 *packed);              /* 0030h */
void lumaType0(u8 *decoder, const u8 *chunk, u32 flags); /* 00F6h */
void lumaType1(u8 *decoder, const u8 *chunk, u32 flags); /* 0249h */

/* xandraw.cpp */
void draw8(u8 *decoder, u8 *screen, u32 pitch, u32 flags);        /* 0FFAh */
void draw16(u8 *decoder, u8 *screen, u32 pitch, u32 flags);       /* 1338h */
void draw8Double(u8 *decoder, u8 *screen, u32 pitch, u32 flags);  /* 1A4Fh */
void draw16Double(u8 *decoder, u8 *screen, u32 pitch, u32 flags); /* 1D9Eh */

}
#endif
