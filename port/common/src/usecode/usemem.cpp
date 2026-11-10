/* The usecode's memory. See usemem.h. */
#include "usemem.h"
#include "gamestate.h"
#include <crash.h>

UsecodeMemory &usecodeMemory = gameGlobal<UsecodeMemory>();

namespace {

/* The code's numbers: from the first on, the lowest one free, above the
 * stacks' (whose pids are below A00h). */
const u16 FIRST_CODE = 0x8007;
const u16 LAST_STACK = 0x7fff;

[[noreturn]] void stop(const char *what, u32 value)
{
    fatalError(3, "the usecode's pointer %04X:%04X %s", value >> 16, value & 0xffff, what);
}

[[noreturn]] void outside(const char *what, const void *at)
{
    fatalError(3, "%s the usecode's memory has not, %p", what, at);
}

}

void UsecodeMemory::add(u16 number, u8 *base, u32 size, u16 origin)
{
    remove(base);
    if (const Region *old = region(number)) {
        byBase_.erase(old->base);
    }
    regions_[number] = {base, size, origin};
    byBase_[base] = number;
}

void UsecodeMemory::addStack(u16 pid, u8 *data, u32 size)
{
    if (stackOf(pid) > LAST_STACK || stackOf(pid) < stackOf(0)) {
        fatalError(3, "pid %u past the usecode's numbers of stacks", pid);
    }
    add(stackOf(pid), data, size, USECODE_DATA);
}

void UsecodeMemory::setFlags(u8 *flags, u32 size)
{
    add(FLAGS, flags, size, 0);
}

void UsecodeMemory::setYamm(u8 *heap, u32 size)
{
    add(YAMM, heap, size, 0);
}

void UsecodeMemory::addCode(u8 *code, u32 size)
{
    u16 number = FIRST_CODE;
    while (regions_.count(number) != 0U) {
        if (number > 0xffff - 8) {
            fatalError(3, "more code than the usecode's numbers");
        }
        number += 8;
    }
    add(number, code, size, 0);
}

void UsecodeMemory::remove(const u8 *base)
{
    auto at = byBase_.find(base);
    if (at == byBase_.end()) {
        return;
    }
    regions_.erase(at->second);
    byBase_.erase(at);
}

const UsecodeMemory::Region *UsecodeMemory::region(u16 number) const
{
    auto at = regions_.find(number);
    return at == regions_.end() ? nullptr : &at->second;
}

u32 UsecodeMemory::pointerTo(u16 number, const u8 *at) const
{
    const Region *r = region(number);
    if ((r == nullptr) || at < r->base || at > r->base + r->size) {
        outside("a pointer", at);
    }
    return static_cast<u32>(number) << 16 | static_cast<u16>(r->origin + (at - r->base));
}

u32 UsecodeMemory::stackPointer(u16 pid, const u8 *at) const
{
    return pointerTo(stackOf(pid), at);
}

u32 UsecodeMemory::flagsPointer(u16 offset) const
{
    const Region *r = region(FLAGS);
    return pointerTo(FLAGS, (r != nullptr) ? r->base + offset : nullptr);
}

u32 UsecodeMemory::yammPointer(u16 offset) const
{
    const Region *r = region(YAMM);
    return pointerTo(YAMM, (r != nullptr) ? r->base + offset : nullptr);
}

u32 UsecodeMemory::codePointer(const u8 *at) const
{
    const u16 number = regionOf(at);
    if (number < FIRST_CODE) {
        outside("code", at);
    }
    return pointerTo(number, at);
}

u16 UsecodeMemory::regionOf(const void *at) const
{
    const u8 *p = static_cast<const u8 *>(at);
    auto next = byBase_.upper_bound(p);
    if (next == byBase_.begin()) {
        return 0;
    }
    --next;
    const Region *r = region(next->second);
    return (r != nullptr) && p <= r->base + r->size ? next->second : 0;
}

u8 *UsecodeMemory::find(u32 pointer, u32 size) const
{
    if (pointer == 0U) {
        return nullptr;
    }
    const Region *r = region(static_cast<u16>(pointer >> 16));
    const u32 offset = pointer & 0xffff;
    if ((r == nullptr) || offset < r->origin || offset - r->origin + size > r->size) {
        return nullptr;
    }
    return r->base + (offset - r->origin);
}

u8 *UsecodeMemory::at(u32 pointer, u32 size) const
{
    u8 *memory = find(pointer, size);
    if ((pointer != 0U) && (memory == nullptr)) {
        stop((region(static_cast<u16>(pointer >> 16)) != nullptr) ? "is past its memory" : "has no memory", pointer);
    }
    return memory;
}
