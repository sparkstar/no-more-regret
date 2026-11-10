/* Borland's phapi.h for the port: the Phar Lap 286|DOS-Extender API
 * (PHAPI.H of the SDK) as far as the sources name it: the types of the
 * interrupt vectors that SPANKER.C and BIOSTIMR.C keep. The port calls none
 * of the extender's functions. */
#ifndef PORT_PHAPI_H
#define PORT_PHAPI_H
#include <cstdint>

/* A real-mode segment:offset, as one 32-bit value. */
using REALPTR = std::uint32_t;

/* The registers of a Borland interrupt function. */
struct REGS_BINT {
    std::uint16_t bp, di, si, ds, es, dx, cx, bx, ax, ip, cs, flags;
};
using PIHANDLER = void (*)(REGS_BINT registers);

#endif
