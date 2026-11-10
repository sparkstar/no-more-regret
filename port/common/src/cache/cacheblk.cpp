/* CACHE\cacheblk.asm (1088:0000-0676) in C++: the members of CacheBlock (see
 * cacheblk.h). Each reads and writes the headers in the original's order,
 * which decides the outcome where the links of a block point at itself or
 * at each other. */
#include "cacheblk.h"
#include "cache.h"
#include "handler.h"

namespace {

/* The cache's memory (CACHE.C); a block is the offset of its header. */
u8 *arena()
{
    return static_cast<u8 *>(cacheArena);
}

const u32 HANDLER = 0;
const u32 HANDLE = 2;
const u32 SIZE = 4;
const u32 SLACK = 8;
const u32 UNKNOWN09 = 9;
const u32 NEXT_IN_MEMORY = 0x0d;
const u32 PREVIOUS_IN_MEMORY = 0x11;
const u32 NEXT = 0x15;
const u32 PREVIOUS = 0x19;
const u32 HEADER = 0x1d;

u32 get(u32 at)
{
    const u8 *p = arena() + at;
    return p[0] | p[1] << 8 | p[2] << 16 | static_cast<u32>(p[3]) << 24;
}

u16 getWord(u32 at)
{
    return static_cast<u16>(arena()[at] | arena()[at + 1] << 8);
}

void put(u32 at, u32 value)
{
    u8 *p = arena() + at;
    p[0] = static_cast<u8>(value);
    p[1] = static_cast<u8>(value >> 8);
    p[2] = static_cast<u8>(value >> 16);
    p[3] = static_cast<u8>(value >> 24);
}

void putWord(u32 at, u16 value)
{
    arena()[at] = static_cast<u8>(value);
    arena()[at + 1] = static_cast<u8>(value >> 8);
}

}

void CacheBlock::linkUsed() const
{
    const u32 head = usedList.address;
    put(address + PREVIOUS, head);
    put(address + NEXT, get(head + NEXT));
    put(head + NEXT, address);
    put(get(address + NEXT) + PREVIOUS, address);
}

void CacheBlock::touch() const
{
    unlink();
    const u32 head = usedList.address;
    put(address + NEXT, get(head + NEXT));
    put(address + PREVIOUS, head);
    put(head + NEXT, address);
    put(get(address + NEXT) + PREVIOUS, address);
}

void CacheBlock::unlink() const
{
    put(get(address + PREVIOUS) + NEXT, get(address + NEXT));
    const u32 previous = get(address + PREVIOUS);
    put(get(address + NEXT) + PREVIOUS, previous);
}

void CacheBlock::unlinkMemory() const
{
    put(get(address + PREVIOUS_IN_MEMORY) + NEXT_IN_MEMORY, get(address + NEXT_IN_MEMORY));
    const u32 previous = get(address + PREVIOUS_IN_MEMORY);
    put(get(address + NEXT_IN_MEMORY) + PREVIOUS_IN_MEMORY, previous);
}

void CacheBlock::insertAfter(CacheBlock previous) const
{
    const u32 next = get(previous.address + NEXT_IN_MEMORY);
    put(address + NEXT_IN_MEMORY, next);
    put(address + PREVIOUS_IN_MEMORY, previous.address);
    put(next + PREVIOUS_IN_MEMORY, address);
    put(previous.address + NEXT_IN_MEMORY, address);
}

void CacheBlock::replace(CacheBlock old) const
{
    put(address + NEXT, get(old.address + NEXT));
    put(address + PREVIOUS, get(old.address + PREVIOUS));
    put(get(address + NEXT) + PREVIOUS, address);
    put(get(address + PREVIOUS) + NEXT, address);
}

void CacheBlock::linkFree() const
{
    const u32 head = freeList.address;
    put(address + NEXT, get(head + NEXT));
    put(address + PREVIOUS, head);
    put(get(head + NEXT) + PREVIOUS, address);
    put(head + NEXT, address);
}

void CacheBlock::absorb(CacheBlock next) const
{
    put(address + NEXT_IN_MEMORY, get(next.address + NEXT_IN_MEMORY));
    put(get(address + NEXT_IN_MEMORY) + PREVIOUS_IN_MEMORY, address);
}

void CacheBlock::relink(CacheBlock moved) const
{
    put(get(address + PREVIOUS_IN_MEMORY) + NEXT_IN_MEMORY, moved.address);
    put(get(address + NEXT_IN_MEMORY) + PREVIOUS_IN_MEMORY, moved.address);
    put(get(address + NEXT) + PREVIOUS, moved.address);
    put(get(address + PREVIOUS) + NEXT, moved.address);
}

/* The free block before this one leaves the list in memory and the free
 * list; the neighbours of both point at moved. */
void CacheBlock::slideBack(CacheBlock moved) const
{
    put(get(get(address + PREVIOUS_IN_MEMORY) + PREVIOUS_IN_MEMORY) + NEXT_IN_MEMORY, moved.address);
    put(get(address + NEXT_IN_MEMORY) + PREVIOUS_IN_MEMORY, moved.address);
    const u32 free = get(address + PREVIOUS_IN_MEMORY);
    const u32 before = get(free + PREVIOUS);
    put(get(free + NEXT) + PREVIOUS, before);
    const u32 after = get(free + NEXT);
    put(get(free + PREVIOUS) + NEXT, after);
    put(get(address + NEXT) + PREVIOUS, moved.address);
    put(get(address + PREVIOUS) + NEXT, moved.address);
}

/* The two heads and the first free block, at cacheStart+3Ah with all of
 * freeBytes, and the lists that join them. */
void CacheBlock::initLists()
{
    const u32 first = cacheStart.address + 0x3a;
    const u32 used = usedList.address;
    put(used + NEXT_IN_MEMORY, first);
    put(used + PREVIOUS_IN_MEMORY, first);
    put(used + NEXT, used);
    put(used + PREVIOUS, used);
    putWord(used + HANDLER, 0xff);
    putWord(used + HANDLE, 0xffff);
    put(used + SIZE, 0);
    arena()[used + SLACK] = 0;
    put(first + NEXT_IN_MEMORY, usedList.address);
    put(first + PREVIOUS_IN_MEMORY, usedList.address);
    put(first + NEXT, freeList.address);
    put(first + PREVIOUS, freeList.address);
    putWord(first + HANDLER, 0xff);
    putWord(first + HANDLE, 0xfffe);
    put(first + SIZE, static_cast<u32>(freeBytes));
    arena()[first + SLACK] = 0;
    const u32 free = freeList.address;
    put(free + NEXT_IN_MEMORY, free);
    put(free + PREVIOUS_IN_MEMORY, free);
    put(free + NEXT, first);
    put(free + PREVIOUS, first);
    putWord(free + HANDLER, 0xff);
    putWord(free + HANDLE, 0xffff);
    put(free + SIZE, 0);
    arena()[free + SLACK] = 0;
    largestFree.address = first;
}

u32 CacheBlock::unknown09() const
{
    return get(address + UNKNOWN09);
}

s32 CacheBlock::size() const
{
    return static_cast<s32>(get(address + SIZE));
}

u32 CacheBlock::next() const
{
    return get(address + NEXT);
}

u32 CacheBlock::previous() const
{
    return get(address + PREVIOUS);
}

u32 CacheBlock::nextInMemory() const
{
    return get(address + NEXT_IN_MEMORY);
}

u32 CacheBlock::previousInMemory() const
{
    return get(address + PREVIOUS_IN_MEMORY);
}

u16 CacheBlock::handle() const
{
    return getWord(address + HANDLE);
}

u16 CacheBlock::slack() const
{
    return arena()[address + SLACK];
}

s16 CacheBlock::handler() const
{
    return static_cast<s16>(getWord(address + HANDLER));
}

void CacheBlock::setSize(s32 size) const
{
    put(address + SIZE, static_cast<u32>(size));
}

void CacheBlock::setUnknown09(u32 value) const
{
    put(address + UNKNOWN09, value);
}

void CacheBlock::setNextInMemory(CacheBlock block) const
{
    put(address + NEXT_IN_MEMORY, block.address);
}

void CacheBlock::setPreviousInMemory(CacheBlock block) const
{
    put(address + PREVIOUS_IN_MEMORY, block.address);
}

void CacheBlock::setHandler(s16 handler) const
{
    putWord(address + HANDLER, static_cast<u16>(handler));
}

void CacheBlock::setSlack(u8 slack) const
{
    arena()[address + SLACK] = slack;
}

void CacheBlock::setHandle(u16 handle) const
{
    putWord(address + HANDLE, handle);
}

/* Moves the block distance bytes down: its slot gets the new address of the
 * data, its neighbours point at the new place, and size bytes from the
 * header on are copied there; the handler at the new place. Nothing calls
 * it. The original writes the slot at the variable cacheHandles itself and
 * the words after it in DGROUP, not in the table it points to (CACHE.C's
 * moveBlock does that right), and copies size bytes from the header, which
 * leaves out the data's last 1Dh bytes. This writes the table and keeps
 * the short copy. */
s16 CacheBlock::moveDown(s32 distance) const
{
    const u32 moved = address - static_cast<u32>(distance);
    const u16 slot = getWord(address + HANDLE);
    if (slot < 0xfffd) {
        cacheHandles[slot] = moved + HEADER;
    }
    relink(moved);
    const u32 bytes = get(address + SIZE);
    for (u32 k = 0; k < bytes; k++) {
        arena()[moved + k] = arena()[address + k];
    }
    return static_cast<s16>(getWord(moved + HANDLER));
}

/* Moves only the header distance bytes down, so the block grows by distance
 * at its front. */
void CacheBlock::slideHeader(s32 distance) const
{
    const u32 moved = address - static_cast<u32>(distance);
    relink(moved);
    const u32 size = get(address + SIZE) + static_cast<u32>(distance);
    for (u32 k = 0; k < HEADER; k++) {
        arena()[moved + k] = arena()[address + k];
    }
    put(moved + SIZE, size);
}
