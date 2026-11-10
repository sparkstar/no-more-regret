/* ..\CACHE\HANDLER.C (Code16, 1078:0319-0525): the handle table, whose
 * slots hold the addresses of the blocks' data (0 when a block is not in the
 * cache), and DefaultCacheHandler, the owner of blocks that must not move. */
#ifndef PORT_HANDLER_H
#define PORT_HANDLER_H
#include "common.h"
#include "cache.h"
#include "cachehnd.h"

/* The slots of the handle table. */
constexpr int CACHE_HANDLES = 5000;
/* DefaultCacheHandler's number. */
constexpr int DEFAULT_HANDLER = 7;

/* Owns one slot; its block may be freed but not moved. */
class DefaultCacheHandler : public CacheHandler {
public:
    /* BCC 3.1 made the implicit destructor of a class with virtual functions
     * virtual when it has work to do, and a delete through this class called the
     * object's own. */
    ~DefaultCacheHandler() override = default;
    DefaultCacheHandler(); /* 1078:03ff */
    /* A block of size bytes in the handler's slot; no references
     * (1078:0452). */
    /* UNCALLED: nothing calls it. */
    u32 allocate(s32 size);
    /* Frees the block; no references (1078:049b). */
    /* UNCALLED: nothing calls it. */
    void release();
    /* Halts (1078:04d9). */
    void relocated(u32 data) override;
    void evicted(u32 data, u16 handle) override; /* 1078:04ec */
};
ASSERT_SIZE(DefaultCacheHandler, 4);

extern u32 *&cacheHandles;
/* The slots handed out so far. */
extern i16 &handleCount;

/* The first of count new slots (1078:0376). */
i16 allocateHandles(i16 count);
void initCacheHandlers();  /* 1078:03a8 */
void closeCacheHandlers(); /* 1078:03d5 */
#endif
