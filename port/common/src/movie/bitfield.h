/* No Remorse's bitfield.asm (1460:0000-002a, file name unknown) in C++. The
 * original's declaration is in PLAYER.C, which reads the run lengths of its
 * RRV slices with it, as rrv.cpp does. */
#ifndef PORT_BITFIELD_H
#define PORT_BITFIELD_H
#include "common.h"

/* count bits of value from bit start, value's bytes in memory order (the
 * four bytes at the code, read as a dword) and each byte's highest bit
 * first. The shifts are the 386's, by their count mod 32: a count of 0
 * gives all the bits from start on. The callers' fourth argument, which it
 * ignores, is left out. */
u32 bitField(u32 value, u8 start, u8 count);
#endif
