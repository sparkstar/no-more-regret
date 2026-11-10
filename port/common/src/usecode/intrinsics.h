/* The usecode's intrinsics: the C functions the usecode calls by number
 * (OP_CALL_INTRINSIC). The original reached each through its far address in
 * USECODE\UNKCOFF.DAT (loadCSymbols) with the argument bytes copied onto
 * the CPU's stack, where the function found its parameters as BCC laid them
 * out: the far pointer this of a member first, then a word for each 8- or
 * 16-bit parameter and 4 bytes for a long or a far pointer. intrinsics.cpp,
 * which is generated, takes them from the usecode's
 * stack at those offsets and calls the port's function. */
#ifndef PORT_INTRINSICS_H
#define PORT_INTRINSICS_H
#include "common.h"
#include "usemem.h"

#include <cstring>

/* An intrinsic's argument bytes on the top of the usecode's stack, which
 * the case of its number reads at their offsets. */
class UsecodeArguments {
public:
    explicit UsecodeArguments(const u8 *bytes) : bytes_(bytes) {}
    [[nodiscard]] u16 wordAt(u16 offset) const
    {
        u16 value;
        std::memcpy(&value, bytes_ + offset, 2);
        return value;
    }
    [[nodiscard]] u32 longAt(u16 offset) const
    {
        u32 value;
        std::memcpy(&value, bytes_ + offset, 4);
        return value;
    }
    /* The memory a pointer of the usecode points to (usemem.h). */
    template <class T>
    [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] T *pointerAt(u16 offset) const
    {
        return usecodeMemory.at<T>(longAt(offset));
    }
    /* The number of the stack they are on (usemem.h). */
    [[nodiscard]] u16 stack() const { return usecodeMemory.regionOf(bytes_); }
    [[nodiscard]] const u8 *bytes() const { return bytes_; }

private:
    const u8 *bytes_;
};

/* The intrinsic number with its arguments: what it returns widened to 32
 * bits, 0 for a void one. */
s32 callIntrinsic(u16 number, const UsecodeArguments &args);
#endif
