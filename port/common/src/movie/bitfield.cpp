/* bitfield.asm (1460:0000-002a) in C++. See bitfield.h. */
#include "bitfield.h"

/* The original swaps the bytes (XCHG, ROR 16, XCHG), shifts left by start
 * and right by 32 - count, both in CL, which the 386 takes mod 32. */
u32 bitField(u32 value, u8 start, u8 count)
{
    const u32 bigEndian = (value >> 24) | ((value >> 8) & 0xff00) | ((value << 8) & 0xff0000) | (value << 24);
    const u8 right = static_cast<u8>(32 - count);
    return (bigEndian << (start & 31)) >> (right & 31);
}
