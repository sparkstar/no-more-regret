/* ..\CACHE\HANDLER.C (1078:02d1-0525): CacheHandler's members, the handle
 * table and DefaultCacheHandler. See cachehnd.h and handler.h. */
#include "gamestate.h"
#include "handler.h"

void CacheHandler::relocated(u32 /*unused*/) {}

void CacheHandler::evicted(u32 /*unused*/, u16 /*unused*/) {}

bool CacheHandler::check(u32 /*unused*/)
{
    return true;
}

CacheHandler::~CacheHandler()
{
    unregisterCacheHandler(id);
}

u32 *&cacheHandles = gameGlobal<u32 *>(nullptr);
DefaultCacheHandler *&defaultHandler = gameGlobal<DefaultCacheHandler *>(nullptr);
/* The slot of defaultHandler. */
u16 &defaultHandle = gameGlobal<u16>(0);
i16 &handleCount = gameGlobal<i16>();

namespace {
/* 1078:0319 */
void initHandles()
{
    cacheHandles = new u32[CACHE_HANDLES];
    memset(cacheHandles, 0, CACHE_HANDLES * sizeof(u32));
    handleCount = 0;
}

/* 1078:0357 */
void closeHandles()
{
    if (cacheHandles != nullptr) {
        delete[] cacheHandles;
        cacheHandles = nullptr;
    }
}
}

i16 allocateHandles(i16 count)
{
    if (static_cast<i16>(count + handleCount) < CACHE_HANDLES) {
        const i16 first = handleCount;
        handleCount += count;
        return first;
    }
    halt();
    return 0;
}

void initCacheHandlers()
{
    initHandles();
    defaultHandler = new DefaultCacheHandler;
}

void closeCacheHandlers()
{
    closeHandles();
    if (defaultHandler != nullptr) {
        delete defaultHandler;
        defaultHandler = nullptr;
    }
}

DefaultCacheHandler::DefaultCacheHandler() : CacheHandler(DEFAULT_HANDLER)
{
    defaultHandle = allocateHandles(1);
    registerCacheHandler(this, DEFAULT_HANDLER);
}

/* UNCALLED: nothing calls it. */
u32 DefaultCacheHandler::allocate(s32 size)
{
    const u32 data = cacheAllocate(size, defaultHandle, DEFAULT_HANDLER);
    CacheBlock(data - static_cast<ui16>(sizeof(BlockHeader))).touch();
    return data;
}

/* UNCALLED: nothing calls it. */
void DefaultCacheHandler::release()
{
    cacheFree(cacheHandles[defaultHandle]);
    if (cacheHandles[defaultHandle] != 0) {
        halt();
    }
}

void DefaultCacheHandler::relocated(u32 /*data*/)
{
    halt();
}

void DefaultCacheHandler::evicted(u32 /*data*/, u16 /*handle*/) {}
