/* The usecode's memory (port/common/src/usecode/usemem.h): the regions'
 * numbers, the pointers into them and back, the ones outside them, a code
 * block's number given again; and a data stack's top, which moves by a
 * signed word as the original's far pointer wraps. */
#include "inter.h"
#include "usemem.h"

#include <cstdio>
#include <new>
#include <string>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH usemem: %s\n", what.c_str());
    }
}

}

int main()
{
    /* A thread's data at 3Ah as in the original's UsecodeProcess, the flags,
     * the YAMM heap and two blocks of code from 0. */
    static u8 stack[200], flags[100], heap[100], code[50], more[50];
    UsecodeMemory &m = usecodeMemory;
    m.addStack(70, stack, sizeof(stack));
    m.setFlags(flags, sizeof(flags));
    m.setYamm(heap, sizeof(heap));
    m.addCode(code, sizeof(code));
    m.addCode(more, sizeof(more));
    u16 thread = m.regionOf(stack);
    check(thread == 0x3007 + 8 * 70, "pid 70's stack, as the saves number it");
    for (u16 number : {thread, m.regionOf(flags), m.regionOf(heap), m.regionOf(code), m.regionOf(more)}) {
        check((number & 7) == 7 && number >> 8 != 0, "a selector's form, the high byte not 0");
    }
    check(m.regionOf(code) == 0x8007 && m.regionOf(more) == 0x800f, "the code blocks in turn");
    u32 pointer = m.stackPointer(70, stack + 0xc6);
    check(pointer == ((u32)thread << 16 | 0x100), "the arguments' pointer as the original's: offset 100h");
    check(m.at(pointer, 2) == stack + 0xc6, "and back");
    check(m.stackPointer(70, stack + 200) == ((u32)thread << 16 | 0x102), "a stack's end, its empty top");
    check(m.flagsPointer(0x1e) == ((u32)UsecodeMemory::FLAGS << 16 | 0x1e), "a global's address");
    check(m.yammPointer(0x20) == ((u32)UsecodeMemory::YAMM << 16 | 0x20), "a string's");
    check(m.codePointer(more + 4) == (0x800fu << 16 | 4), "a place in the code");
    check(m.at(0, 4) == nullptr, "0 is none");
    check(!m.find((u32)thread << 16 | 0x39, 1), "below the data");
    check(!m.find((u32)thread << 16 | 0x101, 2), "past the data");
    check(m.find((u32)thread << 16 | 0x100, 2) != nullptr, "its last two bytes");
    check(!m.find((u32)0x05ff << 16, 1) && !m.find(0x12340000, 1), "no memory behind the number");
    /* A code block taken away, its number given again. */
    m.remove(code);
    m.remove(code);
    check(!m.find(0x80070001, 1), "taken away");
    static u8 other[10];
    m.addCode(other, sizeof(other));
    check(m.regionOf(other) == 0x8007, "the lowest free number, given again");
    m.remove(stack);
    check(!m.find(pointer, 2), "a stack taken away");

    /* The data stack's top moves by a signed word: MOVE_TOP -4 is
     * reserve(0xFFFC), 4 bytes up, as the original's 16-bit offset wraps. */
    alignas(DataStack) unsigned char raw[sizeof(DataStack)];
    DataStack *s = reinterpret_cast<DataStack *>(raw);
    s->top = s->data + 100;
    s->reserve((u16)-4);
    check(s->top == s->data + 104, "reserve(-4) drops 4 bytes");
    s->reserve(6);
    check(s->top == s->data + 98, "reserve(6) makes room for 6");

    std::printf("usemem: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
