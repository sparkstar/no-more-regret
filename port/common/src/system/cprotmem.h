/* MEMORY\CPROTMEM.C: the protected-mode memory manager. The original splits
 * the memory the DOS extender gives into fragments, each a heap of blocks
 * with a selector of its own, and replaces the Borland heap with it. The
 * port keeps how it shares out the machine's memory, which sizes the caches
 * and decides the preloading of shapes, and allocates from the host's heap. */
#ifndef PORT_CPROTMEM_H
#define PORT_CPROTMEM_H
#include "common.h"

/* allocateMemory's fragment argument: any fragment from the third on. */
constexpr int ANY_FRAGMENT = 0xff;
/* The fragment of the sound cache (SNDCACHE.C). */
constexpr int SOUND_FRAGMENT = 1;

/* A fragment: one piece of the machine's memory, which the original keeps
 * as a heap; the port keeps its size and the room its blocks take. */
class Fragment {
public:
    /* The original's linear address of the fragment; the port has none. */
    u32 base;
    u32 size;
    u16 usedBlocks;
    /* The blocks in the free list, the first and the end block included. */
    u16 freeBlocks;

    /* The free bytes, as the blocks the original could give. */
    void freeSpace(u32 *total, u32 *largest);
};
ASSERT_SIZE(Fragment, 12);
/* The size of the fragment table. */
constexpr int MAX_FRAGMENTS = 10;

/* The share of the memory fragment 0 gets; -x sets 100 less it. */
extern u16 &firstFragmentPercent;
/* The memory the machine gives the game. */
extern u32 &systemMemory;
/* The game's switch "Memory profiler activated.": compactions
 * and the cache's use are drawn on the screen (CPROTMEM.C, CACHE.C,
 * SHAPHAND.C; No Regret only). */
extern u8 &memoryProfiler;
/* Fragment 0 is the main heap, 1 the sound fragment. */
extern Fragment (&fragments)[MAX_FRAGMENTS];
extern u16 &fragmentCount;
/* Shares the machine's memory out in the fragments as the original's
 * allocateFragments does, as the port starts (each game's: No Regret's by the
 * memory's size, No Remorse's 57% and at least 1.5 MB). */
void startMemory();

/* bytes from fragment (or ANY_FRAGMENT), purging the caches when it has no
 * room; 0 when there is none. */
void *allocateMemory(u32 bytes, u16 owner, u16 fragment);
/* Asks the system to purge twice. */
void compactMemory(u8 level);
void releaseMemory(void *pointer, u16 owner);
void fragmentFreeSpace(u16 index, u32 *total, u32 *largest);
void memoryStatus(u32 *total, u32 *largest, u16 *selectors);
#endif
