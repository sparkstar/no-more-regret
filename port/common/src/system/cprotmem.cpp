/* MEMORY\CPROTMEM.C (No Regret's 1298:0000-1dbf, No Remorse's 1288:0000-1c89;
 * the addresses are No Regret's) over the host's heap, the functions both
 * games have the same. The original takes all the memory the DOS extender
 * gives, shares it out in fragments and keeps each as a heap of blocks with
 * selectors of their own: fragment 0 is the most of it, which the cache takes
 * whole (CACHE.C), fragment 1 the sound cache's (SNDCACHE.C), the rest the
 * others' allocations. The port shares the machine's memory the platform
 * gives (memory.h) as the original does, so that the caches have the
 * original's sizes and the shapes are preloaded as on its machine (PRELOAD.C
 * looks at fragment 0's size), and allocates from the host's heap. The
 * sharing's values are each game's (startMemory, cprotmem_game.h). See
 * cprotmem.h. */
#include "cprotmem.h"
#include "gamestate.h"
#include "cprotmem_game.h"
#include "desc.h"
#include "memory.h"
#include "system.h"

#include <vector>

namespace {

/* The original's heap in a fragment: the free list starts with an empty
 * block and ends with the end block, 16 bytes each (MemoryBlock); a used
 * block has a header of 10 bytes and at least 16 bytes in all, and a free
 * rest of fewer than 16 bytes goes with it. */
const u32 BLOCK = 16;
const u32 USED_HEADER = 10;

/* The blocks of fragments 0 and 1, whose room the port counts: the other
 * fragments' blocks are the host heap's. */
struct Block {
    u8 *memory;
    u16 fragment;
    u32 bytes;
};
std::vector<Block> &blocks = gameGlobal<std::vector<Block>>();
u32 (&usedBytes)[MAX_FRAGMENTS] = gameGlobal<u32[MAX_FRAGMENTS]>();

/* The bytes a block of bytes takes in a fragment's heap (1298:0136): past
 * 1M whole pages, else an even count, and its header. */
u32 blockBytes(u32 bytes)
{
    if (bytes > 0x100000L && (bytes & 0xfff) != 0) {
        bytes &= 0xfffff000L;
        bytes += 0x1000;
    } else if ((bytes & 1) != 0U) {
        bytes++;
    }
    bytes += USED_HEADER;
    if (bytes < BLOCK) {
        bytes = BLOCK;
    }
    return bytes;
}

/* The fragment's free block, where the port puts its blocks one after the
 * other as the original does in an empty fragment. */
u32 room(u16 fragment)
{
    return fragments[fragment].size - (2 * BLOCK) - usedBytes[fragment];
}

}

/* DESCINIT.C's count of the pool's free descriptors (desc.h): the port's
 * pool, which had 400h, has no descriptors to give; the sound cache takes
 * one a slot. */
u16 &poolFree = gameGlobal<u16>(0x400);
u32 &systemMemory = gameGlobal<u32>(0);
Fragment (&fragments)[MAX_FRAGMENTS] = gameGlobal<Fragment[MAX_FRAGMENTS]>();
u16 &fragmentCount = gameGlobal<u16>();
/* The share of the memory fragment 0 gets (cprotmem.h). Before
 * MEMORY_STARTED: the one program's value is not a constant, and this
 * file's objects start in their order. */
u16 &firstFragmentPercent = gameGlobal<u16>(FIRST_FRAGMENT_PERCENT);
/* The original shares the memory out at its first allocation, before
 * main; the port as it starts. */
static const bool MEMORY_STARTED = (startMemory(), true);

void Fragment::freeSpace(u32 *total, u32 *largest)
{
    const u16 fragment = this - fragments;
    u32 free;

    *total = *largest = 0;
    if (size == 0) {
        return;
    }
    free = fragment == 0 || fragment == SOUND_FRAGMENT ? room(fragment) : size - (2 * BLOCK);
    if (free >= BLOCK) {
        *total = *largest = free - USED_HEADER;
    }
}

void *allocateMemory(u32 bytes, u16 /*unused*/, u16 fragment)
{
    s16 pass;
    u32 taking;

    if (bytes == 0) {
        return nullptr;
    }
    if ((bytes & 1) != 0U) {
        if (bytes > 0x0ffffff4L) {
            return nullptr;
        }
    } else if (bytes > 0x0ffffff5L) {
        return nullptr;
    }
    for (pass = 0; pass < 3; pass++) {
        if (fragment == ANY_FRAGMENT) {
            return new u8[bytes];
        }
        if (fragment == 0 || fragment == SOUND_FRAGMENT) {
            taking = blockBytes(bytes);
            if (taking <= room(fragment)) {
                if (room(fragment) - taking < BLOCK) {
                    taking = room(fragment);
                }
                u8 *memory = new u8[bytes];
                usedBytes[fragment] += taking;
                fragments[fragment].usedBlocks++;
                blocks.push_back({memory, fragment, taking});
                return memory;
            }
        } else if (fragment < fragmentCount) {
            return new u8[bytes];
        }
        if (pass < 2) {
            compactMemory(pass);
        }
    }
    return nullptr;
}

/* The original compacts the fragments from the third on after the purge;
 * the host's heap needs none. */
void compactMemory(u8 level)
{
    currentSystem->purgeMemory(level);
    currentSystem->purgeMemory(level);
}

void releaseMemory(void *pointer, u16 /*unused*/)
{
    for (auto block = blocks.begin(); block != blocks.end(); ++block) {
        if (block->memory == pointer) {
            usedBytes[block->fragment] -= block->bytes;
            fragments[block->fragment].usedBlocks--;
            blocks.erase(block);
            break;
        }
    }
    delete[] static_cast<u8 *>(pointer);
}

void fragmentFreeSpace(u16 index, u32 *total, u32 *largest)
{
    if (index >= fragmentCount) {
        halt();
    }
    fragments[index].freeSpace(total, largest);
}

void memoryStatus(u32 *total, u32 *largest, u16 *selectors)
{
    u16 index;
    u32 free;
    u32 biggest;

    *total = *largest = 0;
    for (index = 0; index < fragmentCount; index++) {
        fragments[index].freeSpace(&free, &biggest);
        if (*largest < biggest) {
            *largest = biggest;
        }
        *total += free;
    }
    *selectors = poolFree;
}

/* allocateFragments's sharing (1298:1170): fragment 0 gets a share of the
 * machine's memory, unless -x gave one; fragment 1 is the sound's. No
 * Regret's share goes by the memory's size and is counted in kilobytes; No
 * Remorse's is 57%, in bytes rounded down to a multiple of 4 and at least
 * 1.5 MB (1288:10fa). The original takes the rest in the pieces the extender
 * gives (No Regret's on its machine of 16 MB five of them, 3435520 bytes; No
 * Remorse's up to 14 MB), the port as one fragment. */
void startMemory()
{
    u32 total;
    u32 firstSize;

    systemMemory = memoryTotal();
    if (SHARE_BY_MEMORY_SIZE) {
        if (systemMemory >= 16000000L) {
            firstFragmentPercent = 78;
        } else if (systemMemory >= 14000000L) {
            firstFragmentPercent = 75;
        } else if (systemMemory >= 12000000L) {
            firstFragmentPercent = 71;
        } else if (systemMemory >= 10000000L) {
            firstFragmentPercent = 65;
        }
        total = systemMemory / 1024;
        firstSize = firstFragmentPercent * total / 100;
        firstSize <<= 10;
    } else {
        firstSize = firstFragmentPercent * systemMemory / 100;
        if (firstSize < FIRST_MINIMUM) {
            halt();
        }
        firstSize &= ~3L;
    }
    fragments[0].size = firstSize;
    fragments[SOUND_FRAGMENT].size = SOUND_FRAGMENT_SIZE;
    fragments[2].size = systemMemory - firstSize - SOUND_FRAGMENT_SIZE;
    fragmentCount = 3;
    for (u16 index = 0; index < fragmentCount; index++) {
        fragments[index].usedBlocks = 0;
        fragments[index].freeBlocks = 3;
    }
}
