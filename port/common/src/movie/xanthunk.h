/* No Regret's Code144 (1478:0000-0560, xanthunk.asm): the decoders of the
 * 'Axan' sound (xanthunk.cpp). Its calls into XANDRV32.DLL, the 32-bit Xan
 * movie driver (xanCallback, openXanDriver, callThunk and xanCall1-8), the
 * port has not: the game calls its decoder (port/common/xan/xandrv.h). */
#ifndef PORT_XANTHUNK_H
#define PORT_XANTHUNK_H
#include "common.h"

extern "C" {
/* The decoders: count bytes of codes at source become samples at dest; they
 * return the bytes made. Code137 picks one for the formats. */
u16 xanMono16(void *dest, const void *source, u16 count);
u16 xanStereo16(void *dest, const void *source, u16 count);
u16 xanStereo8(void *dest, const void *source, u16 count);
u16 xanMono8(void *dest, const void *source, u16 count);
u16 xanStereoToMono16(void *dest, const void *source, u16 count);
u16 xanStereoToMono8(void *dest, const void *source, u16 count);
u16 xanMonoToStereo16(void *dest, const void *source, u16 count);
u16 xanMonoToStereo8(void *dest, const void *source, u16 count);
}
#endif
