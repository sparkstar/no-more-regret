/* XANDRV32.DLL 0DC1h-0FD4h: the two unpackers. See xanlib.h. Neither
 * knows where its input or its output ends: the data do. */
#include "xanlib.h"

#include <cstddef>

namespace xan {

/* 0DC1h: Huffman codes to bytes. source has the count of the tree's pairs
 * and the end's code (eof), the pairs, the bits from the top of each byte.
 * The codes below eof are bytes out; above it, the tree's pairs (eof+1
 * the first), whose root is the last; the pair of node n is at source +
 * 2(n - eof). The walk keeps 2 * node + bit in EDX and puts the child in
 * DL alone: from a node of 80h or more on the higher bits stay and the
 * walk reads elsewhere. Every child is written where the next byte goes
 * (an inner one is overwritten, or stays there at the end). The bytes out. */
u32 huffman(u8 *dest, const u8 *source)
{
    const u32 pairs = byte(source);
    const u32 eof = byte(source + 1);
    const u8 *in = source + 2;
    /* The tree's pairs from in, node n's at 2(n - eof - 1) after it. */
    const u8 *tree = in;
    const std::ptrdiff_t before = 2 * static_cast<std::ptrdiff_t>(eof + 1);
    in += 2 * pairs;
    const u32 root = eof + pairs;
    u32 node = root;
    u8 *out = dest;
    for (;;) {
        u8 bits = byte(in++);
        for (int k = 0; k < 8; k++) {
            const u32 bit = bits >> 7;
            bits = static_cast<u8>(bits << 1);
            node = node * 2 + bit;
            const u8 child = byte(tree + (static_cast<std::ptrdiff_t>(node) - before));
            node = (node & ~0xffU) | child;
            if (child == static_cast<u8>(eof)) {
                return static_cast<u32>(out - dest);
            }
            setByte(out, child);
            if (child < static_cast<u8>(eof)) {
                out++;
                node = (node & ~0xffU) | (root & 0xff);
            }
        }
    }
}

/* 0EDEh: the LZ packing of the colour table and the corrections. A byte
 * below 80h: a byte b, op&3 bytes as they are, then ((op&1Ch)>>2)+3 bytes
 * from ((op&60h)<<3)+b+1 back. 80h-BFh: two bytes b0 b1, b0>>6 bytes, then
 * (op&3Fh)+4 from ((b0&3Fh)<<8)+b1+1 back. C0h-DFh: three bytes (read as a
 * dword), op&3 bytes, then b2+5+((op&0Ch)<<6) from ((op&10h)<<12)+(b0<<8)
 * +b1+1 back. E0h and up: ((op&1Fh)<<2)+4 bytes as they are, up to 70h;
 * more is the end, after op&3 bytes. The bytes out. */
u32 unpack(u8 *dest, const u8 *source)
{
    u8 *out = dest;
    const u8 *in = source;
    for (;;) {
        const u32 op = byte(in++);
        if ((op & 0x80) == 0U) {
            const u32 b = byte(in++);
            copyBytes(out, in, op & 3);
            const u8 *from = out - (((op & 0x60) << 3) + b + 1);
            copyBytes(out, from, ((op & 0x1c) >> 2) + 3);
        } else if ((op & 0x40) == 0U) {
            const u32 bytes = word(in);
            in += 2;
            const u32 b0 = bytes & 0xff;
            const u32 b1 = bytes >> 8;
            copyBytes(out, in, b0 >> 6);
            const u8 *from = out - (((b0 & 0x3f) << 8) + b1 + 1);
            copyBytes(out, from, (op & 0x3f) + 4);
        } else if ((op & 0x20) == 0U) {
            const u32 bytes = dword(in);
            in += 3;
            copyBytes(out, in, op & 3);
            const u32 b0 = bytes & 0xff;
            const u32 b1 = bytes >> 8 & 0xff;
            const u32 b2 = bytes >> 16 & 0xff;
            const u8 *from = out - ((((op & 0x10) >> 4) << 16) + (b0 << 8) + b1 + 1);
            copyBytes(out, from, b2 + 5 + ((op & 0x0c) << 6));
        } else {
            const u32 count = ((op & 0x1f) << 2) + 4;
            if (count <= 0x70) {
                copyBytes(out, in, count);
                continue;
            }
            copyBytes(out, in, op & 3);
            return static_cast<u32>(out - dest);
        }
    }
}

}
