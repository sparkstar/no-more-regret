/* ..\CACHE\CACHE.C (Code15): the cache. One block of linear memory
 * (cacheArena) is cut into blocks (cacheblk.h) that the handlers own: a used
 * block keeps its data's address in its slot of the handle table
 * (HANDLER.C), and the handler hears when the block moves or is freed. When
 * there is no room the cache frees the blocks used longest ago or compacts
 * the memory. No Regret's MemProfiler draws the free memory. */
#ifndef PORT_CACHE_H
#define PORT_CACHE_H
#include "common.h"
#include "cacheblk.h"
#include "desc.h"
#include "kprocess.h"

class CacheHandler;

void registerCacheHandler(CacheHandler *handler, u16 id);
void unregisterCacheHandler(u16 id);

/* The process that draws the free memory in the top lines of the screen
 * while memoryProfiler is set (No Regret only). */
class MemProfiler : public Process {
public:
    /* UNCALLED: nothing calls it. */
    explicit MemProfiler(Loading loading) : Process(loading) {}
    MemProfiler();
    void run() override;
};
ASSERT_SIZE(MemProfiler, 0x32);

/* Half of the free space makeRoom and compactCache leave (a tenth of the
 * cache; No Regret only). */
extern s32 &halfMinimumFree;
/* The memory of the cache. The selector of the far pointer covers all of it;
 * the blocks' addresses are offsets from it. */
extern void *&cacheArena;
/* The start of the memory (0), the two heads and the largest free block. */
extern CacheBlock &cacheStart;
extern CacheBlock &usedList;
extern CacheBlock &freeList;
extern CacheBlock &largestFree;
extern s32 &freeBytes;
/* The size of the cache's arena (No Remorse's PLAYER.C checks the room for
 * its buffer against it). */
extern s32 &totalBytes;
extern CacheHandler *(&handlers)[8];

/* A pointer to offset in the cache's memory. The port reaches it from
 * cacheArena, as CacheBlock's members do; the original through the
 * linear address. */
inline void *cachePointer(u32 offset)
{
    return static_cast<u8 *>(cacheArena) + offset;
}

void initCache(s32 size);
void closeCache();
/* A used block of size bytes for handler; its data's address goes in slot
 * handle of the handle table unless handle is NO_HANDLE. The result is the
 * data's address. */
u32 cacheAllocate(s32 size, u16 handle, i16 handler);
/* Frees a used block; the result is the bytes freed. */
s32 freeBlock(CacheBlock block);
inline void CacheBlock::release()
{
    freeBlock(*this);
}
/* UNCALLED: nothing calls it. */
void cacheFree(u32 data);
/* Moves block distance bytes down. */
void moveBlock(CacheBlock *block, s32 distance);
/* Frees the block of handler used longest ago; the result is its handle,
 * FREE_HANDLE when there is none. */
u16 freeOldest(i16 handler);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void cacheExit(u16 /*unused*/);
/* UNCALLED: nothing calls it. */
void checkCache(std::source_location place = CALL_PLACE);
/* UNCALLED: nothing calls it. */
void showCache();
/* UNCALLED: nothing calls it. */
void dumpCache();
/* UNCALLED: nothing calls it. */
void unusedCache();
/* UNCALLED: nothing calls it. */
i16 cacheUsage();
/* UNCALLED: nothing calls it. */
void saveCache(char *path);
/* UNCALLED: nothing calls it. */
void loadCache(char *path);
#endif
