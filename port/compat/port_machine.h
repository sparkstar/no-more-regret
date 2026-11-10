/* The machine-level macros of common.h for the port's modern builds; BCC
 * keeps its own, which give the original bytes. The DOS and BIOS services,
 * the flags, the registers and the stacks become calls into the system
 * layer, only declared here; the system layer writes them. The 32-bit
 * arithmetic of MISC\PFLOAT.C is
 * C++ that does what its assembly does. */
#ifndef PORT_MACHINE_H
#define PORT_MACHINE_H
#include <cstdint>
#include <cstring>

/* The asm statements of the original's sources. CLD does nothing in the
 * port. */
#define CLEAR_DIRECTION() ((void)0)

/* A temporary as an lvalue, for a call that takes its address or binds it
 * to a T &; it lives to the end of the full expression. */
template <class T> T &lvalue(T &&value)
{
    return value;
}

/* The four bytes of an operand of the 32-bit arithmetic, as MOV EAX,a
 * reads them: a long's or a PFloat's. */
template <class T> s32 portLongBytes(const T &value)
{
    static_assert(sizeof(T) == 4);
    s32 bytes;
    std::memcpy(&bytes, &value, 4);
    return bytes;
}
/* high:low=a*b, the 64-bit product of two longs; a and b are read before
 * high and low are written, as the assembly does. */
#define LONG_MULTIPLY(a, b, high, low)                                                                                 \
    do {                                                                                                               \
        const std::int64_t product_ = (std::int64_t)portLongBytes(a) * portLongBytes(b);                               \
        (high) = (s32)(product_ >> 32);                                                                                \
        (low) = (s32)product_;                                                                                         \
    } while (0)
/* whole=a/b, as IDIV of EDX:EAX=0:a (not sign-extended: a counts as
 * unsigned) by b, and fraction=the remainder divided by the high word of b
 * (DIV). Both keep their low words. b below 10000h, or a quotient past 32
 * bits, divides by zero or overflows, which faults in the original. */
#define FIXED_DIVIDE(a, b, whole, fraction)                                                                            \
    do {                                                                                                               \
        const std::int64_t dividend_ = (std::uint32_t)portLongBytes(a);                                                \
        const std::int64_t divisor_ = portLongBytes(b);                                                                \
        const std::uint32_t remainder_ = (std::uint32_t)(dividend_ % divisor_);                                        \
        (whole) = (i16)(dividend_ / divisor_);                                                                         \
        (fraction) = (u16)(remainder_ / ((std::uint32_t)portLongBytes(b) >> 16));                                      \
    } while (0)

#endif
