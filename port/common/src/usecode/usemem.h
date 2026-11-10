/* The usecode's memory: what a pointer of the usecode points into. The
 * usecode's code (EUSECODE.FLX) keeps a pointer in 4 bytes, as the original's
 * far pointers are: the offset in the low word, a number in the high word.
 * The numbers are the regions of the usecode's memory: each usecode process's
 * data stack, the flags, the YAMM heap and the code of each loaded function.
 * UsecodeMemory gives the regions their numbers, makes the pointers into them
 * and finds the memory a pointer points to.
 *
 * The numbers keep the form of the original's selectors (an index in the
 * local table at privilege 3), so that the oracles give the original's
 * memory the same values, and their high byte is never 0: the usecode sees a
 * stack's (intrinsics.cpp's fineDirection) where the original's are over
 * 100h. A process's stack has the number of its pid, which the saves have
 * too (saveimage.cpp); a function's code one the table gives while it is
 * loaded. A pointer with no memory behind it stops the game (the original
 * reads what its selector has then). */
#ifndef PORT_USEMEM_H
#define PORT_USEMEM_H
#include "common.h"

#include <map>

/* Where the original's UsecodeProcess has its thread's data stack: the
 * offsets in a stack start there, so that a pointer into it is the
 * original's. */
const u16 USECODE_DATA = 0x3a;

class UsecodeMemory {
public:
    /* The flags' and the YAMM heap's numbers. */
    static const u16 FLAGS = 0x0807;
    static const u16 YAMM = 0x080f;
    /* The number of the stack of process pid. */
    static u16 stackOf(u16 pid) { return static_cast<u16>(0x3007 + (8 * pid)); }

    /* The regions: the data of pid's stack (200 bytes from USECODE_DATA), the
     * flags and the YAMM heap (from 0), a function's code (from 0). */
    void addStack(u16 pid, u8 *data, u32 size);
    void setFlags(u8 *flags, u32 size);
    void setYamm(u8 *heap, u32 size);
    void addCode(u8 *code, u32 size);
    /* size bytes at base as the region number, whose offsets start at
     * origin (the tests' own regions). */
    void add(u16 number, u8 *base, u32 size, u16 origin);
    /* Takes away the region of the memory at base, if it has one. */
    void remove(const u8 *base);

    /* The usecode's pointer to at in pid's stack, to offset in the flags or
     * the YAMM heap, to at in a function's code (its end too: an empty
     * stack's top). */
    u32 stackPointer(u16 pid, const u8 *at) const;
    [[nodiscard]] u32 flagsPointer(u16 offset) const;
    [[nodiscard]] u32 yammPointer(u16 offset) const;
    u32 codePointer(const u8 *at) const;
    /* The number of the region that holds at, 0 for none. */
    u16 regionOf(const void *at) const;

    /* The memory of pointer, size bytes; nullptr for 0, and where it is not
     * the usecode's (find) or a stop (at). */
    [[nodiscard]] u8 *find(u32 pointer, u32 size) const;
    [[nodiscard]] u8 *at(u32 pointer, u32 size) const;
    template <class T>
    [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] T *at(u32 pointer) const
    {
        return reinterpret_cast<T *>(at(pointer, sizeof(T)));
    }

private:
    struct Region {
        u8 *base;
        u32 size;
        u16 origin;
    };
    [[nodiscard]] const Region *region(u16 number) const;
    u32 pointerTo(u16 number, const u8 *at) const;
    std::map<u16, Region> regions_;
    /* The numbers by their memory, for regionOf and remove. */
    std::map<const u8 *, u16> byBase_;
};

extern UsecodeMemory &usecodeMemory;
#endif
