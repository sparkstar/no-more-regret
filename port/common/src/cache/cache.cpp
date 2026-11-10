/* ..\CACHE\CACHE.C (No Regret's 1070:0000-22c1, No Remorse's 1060:0000-1f6b;
 * the addresses are No Regret's): the cache's blocks, and No Regret's
 * MemProfiler. See cache.h. What the games do differently are their values
 * (cache_game.h). */
#include "gamestate.h"
#include <conio.h>
#include <cstdio>
#include <cstdlib>
#include "cache.h"
#include "cache_game.h"
#include "cachehnd.h"
#include "cprotmem.h"
#include "crusys.h"
#include "screen.h"
#include "handler.h"
#include "kernel.h"
#include "viewport.h"
#include "files.h"
#include "host.h"
#include <tuple>

CacheBlock findFree(s32 size);
void setCacheSize(s32 size);
void releaseCache();
void allocateCacheMemory();
void initBlocks();
void findLargestFree();
void makeRoom(s32 size);
void compactCache(s32 wanted);
void mergeNext(CacheBlock *block);
void moveOverFree(CacheBlock *block);
/* UNCALLED: nothing calls it. */
void checkBlocks(std::source_location place);
/* UNCALLED: nothing calls it. */
void printCache(i16 /*unused*/);
/* UNCALLED: nothing calls it. */
void drawCache();

void *&cacheArena = gameGlobal<void *>(nullptr);
CacheBlock &cacheStart = gameGlobal<CacheBlock>(0);
CacheBlock &usedList = gameGlobal<CacheBlock>(0);
CacheBlock &freeList = gameGlobal<CacheBlock>(0);
CacheBlock &largestFree = gameGlobal<CacheBlock>(0);
s32 &totalBytes = gameGlobal<s32>(0);
s32 &freeBytes = gameGlobal<s32>(0);
/* The blocks, the two heads counted once. */
u16 &totalBlocks = gameGlobal<u16>(0);
u16 &usedBlocks = gameGlobal<u16>(0);
u16 &freeBlockCount = gameGlobal<u16>(0);
s32 &largestFreeSize = gameGlobal<s32>(0);
s32 &usedBytes = gameGlobal<s32>(0);
s32 &slackBytes = gameGlobal<s32>(0);

u16 &handlerCount = gameGlobal<u16>(0);
u8 &cacheReleased = gameGlobal<u8>(0);
/* checkBlocks checks while it is set. */
u16 &checkingCache = gameGlobal<u16>(0);
u16 &unknown10d3 = gameGlobal<u16>(1); /* no references */

/* allocateCacheMemory clears the arena in pieces that a far pointer reaches. */
constexpr int CLEAR_PIECE = 64000;

/* A tenth of the cache: what makeRoom and compactCache leave free (No
 * Regret's; 0 in No Remorse, CACHE_KEEPS_FREE). */
s32 &minimumFree = gameGlobal<s32>(0);
s32 &halfMinimumFree = gameGlobal<s32>(0);

u16 &compactions = gameGlobal<u16>(0);

CacheHandler *(&handlers)[8] = gameGlobal<CacheHandler *[8]>();

MemProfiler::MemProfiler()
{
    makeSystem();
    nameProcess(pid, "MemProfiler");
}

/* One line of the screen per value, a pixel per 2K: the free bytes, the
 * largest free block, and the memory manager's free and largest bytes. */
void MemProfiler::run()
{
    i16 x;
    u32 total;
    u32 largest;
    u16 selectors;

    if ((memoryProfiler != 0U) && (worldActive != 0U)) {
        x = freeBytes / 2048;
        if (x > SCREEN_RIGHT) {
            x = SCREEN_RIGHT;
        }
        drawViewport->line(0, 0, x, 0, 0xff);
        if (x != SCREEN_RIGHT) {
            drawViewport->line(x + 1, 0, SCREEN_RIGHT, 0, 0);
        }
        screenViewport->blit(drawViewport, 0, 0, SCREEN_RIGHT, 0, 0, 0);
        x = largestFreeSize / 2048;
        if (x > SCREEN_RIGHT) {
            x = SCREEN_RIGHT;
        }
        /* The original's bug: the white line is always the whole width. */
        drawViewport->line(0, 1, SCREEN_RIGHT, 1, 0xff);
        if (x != SCREEN_RIGHT) {
            drawViewport->line(x + 1, 1, SCREEN_RIGHT, 1, 0);
        }
        screenViewport->blit(drawViewport, 0, 1, SCREEN_RIGHT, 1, 0, 1);
        memoryStatus(&total, &largest, &selectors);
        x = total / 2048;
        if (x > SCREEN_RIGHT) {
            x = SCREEN_RIGHT;
        }
        drawViewport->line(0, 3, x, 3, 0xff);
        if (x != SCREEN_RIGHT) {
            drawViewport->line(x + 1, 3, SCREEN_RIGHT, 3, 0);
        }
        screenViewport->blit(drawViewport, 0, 3, SCREEN_RIGHT, 3, 0, 3);
        x = largest / 2048;
        if (x > SCREEN_RIGHT) {
            x = SCREEN_RIGHT;
        }
        drawViewport->line(0, 4, x, 4, 0xff);
        if (x != SCREEN_RIGHT) {
            drawViewport->line(x + 1, 4, SCREEN_RIGHT, 4, 0);
        }
        screenViewport->blit(drawViewport, 0, 4, SCREEN_RIGHT, 4, 0, 4);
    }
}

/* size is at most 8M; 0 means 8M. */
void initCache(s32 size)
{
    if (size > static_cast<s32>(0x800000L) || size == 0) {
        size = 0x800000L;
    }
    setCacheSize(size);
    if (MEMORY_PROFILER) {
        new MemProfiler;
    }
}

/* 1070:029a */
/* UNCALLED: nothing calls it. */
void cacheExit(u16 /*unused*/)
{
    releaseCache();
}

void closeCache()
{
    releaseCache();
}

/* 1070:02ac */
/* UNCALLED: nothing calls it. */
void checkCache(std::source_location place)
{
    checkBlocks(place);
}

/* 1070:02c0 */
/* UNCALLED: nothing calls it. */
void showCache()
{
    drawCache();
}

/* Text mode with 43 or 50 lines, without clearing the screen (1070:02c6). */
/* UNCALLED: nothing calls it. */
void dumpCache()
{
    /* The BIOS's mode 83h: the text, the screen kept. */
    screenSetMode(3, true);
    hostTextMode(C80);
    hostTextMode(C4350);
    printCache(0);
}

/* 1070:02eb */
void setCacheSize(s32 size)
{
    totalBytes = size;
    allocateCacheMemory();
    initBlocks();
    handlerCount = 0;
    slackBytes = 0;
    checkingCache = 1;
    cacheReleased = 0;
}

/* Frees the blocks of the handlers and the memory, once (1070:031c). */
void releaseCache()
{
    if (cacheReleased == 0U) {
        cacheReleased = 1;
        CacheBlock block = usedList;
        for (u16 i = 0; i < totalBlocks; i++) {
            const CacheBlock previous(block.previous());
            if (block.handler() <= 7) {
                freeBlock(block);
            }
            block = previous;
        }
        /* The original's farfree, which is CPROTMEM.C's releaseMemory. */
        if (cacheArena != nullptr) {
            releaseMemory(cacheArena, 0);
        }
    }
}

/* All but 4K of the largest free block of the main fragment, cleared (1070:03ab). */
void allocateCacheMemory()
{
    u8 *memory;
    u32 total;
    u32 largest;
    s32 i;
    i16 chunks;

    fragments[0].freeSpace(&total, &largest);
    totalBytes = largest - 0x1000;
    cacheArena = allocateMemory(totalBytes, 0, 0);
    if (cacheArena == nullptr) {
        halt();
    }
    if (CACHE_KEEPS_FREE) {
        minimumFree = totalBytes / 10;
        halfMinimumFree = minimumFree / 2;
    }
    /* The original keeps the arena's linear address (cacheMemoryBase),
     * which the movie driver's flat memory counts from (MOVHOST.C); the
     * port's movies take pointers. */
    cacheStart.address = 0;
    chunks = totalBytes / CLEAR_PIECE;
    for (i = 0; i < chunks; i++) {
        memory = static_cast<u8 *>(cachePointer(i * CLEAR_PIECE));
        memset(memory, 0, CLEAR_PIECE);
    }
    memory = static_cast<u8 *>(cachePointer(i * CLEAR_PIECE));
    memset(memory, 0, totalBytes % CLEAR_PIECE);
}

void registerCacheHandler(CacheHandler *handler, u16 id)
{
    if (handlerCount < 8) {
        handlers[id] = handler;
        handlerCount++;
        return;
    }
    halt();
}

void unregisterCacheHandler(u16 id)
{
    handlers[id] = nullptr;
}

/* The two heads at the start and one free block after them (1070:054a). */
void initBlocks()
{
    largestFreeSize = freeBytes = totalBytes - static_cast<ui16>(3 * sizeof(BlockHeader));
    totalBlocks = 2;
    usedBlocks = 1;
    freeBlockCount = 1;
    usedList = cacheStart;
    freeList = usedList.address + static_cast<ui16>(sizeof(BlockHeader));
    /* initLists is static; it is called through an object of its own. */
    const CacheBlock block{};
    block.initLists();
}

u32 cacheAllocate(s32 size, u16 handle, i16 handler)
{
    const CacheBlock block = findFree(size);
    const s32 available = block.size();
    u32 data;

    if (size + static_cast<ui16>(sizeof(BlockHeader)) < available) {
        freeBytes -= size + static_cast<ui16>(sizeof(BlockHeader));
        const CacheBlock rest(block.address + size + static_cast<ui16>(sizeof(BlockHeader)));
        rest.setHandler(FREE_HANDLER);
        rest.setHandle(FREE_HANDLE);
        rest.setSlack(0);
        rest.setSize(available - size - static_cast<ui16>(sizeof(BlockHeader)));
        rest.insertAfter(block);
        rest.replace(block);
    } else {
        block.unlink();
        slackBytes += block.size() - size;
        block.setSlack(block.size() - size);
        freeBytes -= size + block.slack();
        freeBlockCount--;
        totalBlocks--;
    }
    if (block.address == largestFree.address) {
        findLargestFree();
    }
    block.setHandle(handle);
    block.setHandler(handler);
    block.setSize(size);
    block.linkUsed();
    totalBlocks++;
    usedBlocks++;
    usedBytes += size;
    data = block.address + static_cast<ui16>(sizeof(BlockHeader));
    if (handle != NO_HANDLE) {
        cacheHandles[handle] = data;
    }
    return data;
}

/* The first free block of size bytes, making room when there is none (1070:0595). */
CacheBlock findFree(s32 size)
{
    if (totalBytes - static_cast<ui16>(3 * sizeof(BlockHeader)) <= size) {
        halt();
    }
    const CacheBlock largest = largestFree;
    if (size > largestFreeSize || largest.handle() != FREE_HANDLE) {
        if (freeBytes + slackBytes >= size) {
            if (freeBlockCount > 0) {
                compactCache(size + minimumFree);
            } else {
                makeRoom(size + minimumFree + static_cast<ui16>(sizeof(BlockHeader)));
            }
        } else {
            makeRoom(size + minimumFree);
        }
    }
    CacheBlock block = freeList;
    block = block.next();
    do {
        if (block.size() >= size) {
            return block;
        }
        block = block.next();
    } while (block.address != freeList.address);
    halt();
    return 0;
}

/* 1070:08f2 */
void findLargestFree()
{
    largestFreeSize = 0;
    CacheBlock block = freeList;
    u16 count = 0;
    do {
        count++;
        if (block.size() > largestFreeSize) {
            largestFree = block.address;
            largestFreeSize = block.size();
        }
        block = block.next();
    } while (block.address != freeList.address && count <= static_cast<ui16>(freeBlockCount + 1));
    if (count > static_cast<ui16>(freeBlockCount + 1)) {
        halt();
    }
}

s32 freeBlock(CacheBlock block)
{
    const u16 handle = block.handle();
    if (handlers[block.handler()] != nullptr) {
        handlers[block.handler()]->evicted(block.address + static_cast<ui16>(sizeof(BlockHeader)), handle);
    }
    if (handle < NO_HANDLE) {
        cacheHandles[handle] = 0;
    }
    block.unlink();
    block.setHandle(FREE_HANDLE);
    block.setHandler(FREE_HANDLER);
    const s32 freed = block.size() + block.slack();
    if (freed > largestFreeSize) {
        largestFreeSize = freed;
        largestFree = block.address;
    }
    usedBytes -= block.size();
    block.setSize(freed);
    freeBytes += freed;
    slackBytes -= block.slack();
    block.setSlack(0);
    usedBlocks--;
    freeBlockCount++;
    block.linkFree();
    s32 result = freed;
    const CacheBlock next(block.nextInMemory());
    if (next.handle() == FREE_HANDLE) {
        mergeNext(&block);
        result += sizeof(BlockHeader);
    }
    CacheBlock previous(block.previousInMemory());
    if (previous.handle() == FREE_HANDLE) {
        mergeNext(&previous);
        result += sizeof(BlockHeader);
        return result;
    }
    return result;
}

/* UNCALLED: nothing calls it. */
void cacheFree(u32 data)
{
    if (data != 0) {
        freeBlock(data - static_cast<ui16>(sizeof(BlockHeader)));
    }
}

/* Frees the blocks used longest ago, then compacts (1070:0be8). The original's
 * behaviour: wanted is worked out again from totalBytes instead of minimumFree,
 * and the loop frees from the end of the LRU list without stopping at its head,
 * usedList. */
void makeRoom(s32 size)
{
    s32 wanted = totalBytes / 10;
    if (size - freeBytes > wanted) {
        wanted = size - freeBytes;
    }
    s32 freed = 0;
    CacheBlock block(usedList.previous());
    while (freed < wanted) {
        const CacheBlock previous(block.previous());
        freed += freeBlock(block);
        block = previous;
    }
    compactCache(size);
}

/* Moves the used blocks down over the free ones until a free block of wanted
 * bytes forms (1070:0c9c). Walks the blocks in memory with gap, the free bytes
 * gathered so far: used blocks move down by it, free blocks and the slack of
 * used blocks add to it. What is left becomes one free block at the end. */
void compactCache(s32 wanted)
{
    if (memoryProfiler != 0U) {
        drawViewport->clear(borlandRandom(240) + 16, 10, 5, 19, 15);
        screenViewport->blit(drawViewport, 10, 5, 19, 15, 10, 5);
    }
    compactions++;
    s32 gap = 0;
    CacheBlock block(usedList.address + static_cast<ui16>(2 * sizeof(BlockHeader)));
    bool done = false;
    CacheBlock previous;
    while (block.address != usedList.address) {
        if (block.isHead()) {
            halt();
        }
        const CacheBlock next(block.nextInMemory());
        previous = block.previousInMemory();
        if (previous.slack() > 0) {
            const u16 slack = previous.slack();
            if (block.isUsed()) {
                gap += slack;
                const CacheBlock moved(block.address - gap); /* not used */
                moveBlock(&block, gap);
                slackBytes -= slack;
                freeBytes += slack;
                previous.setSlack(0);
            } else if (block.isFree()) {
                if (gap > 0) {
                    const s32 size = block.size();
                    block.slideHeader(gap);
                    if (gap + size >= wanted) {
                        done = true;
                        break;
                    }
                    block = block.address - gap;
                }
                const CacheBlock moved(block.address - slack);
                moveBlock(&block, slack);
                gap = 0;
                slackBytes -= slack;
                moved.setSize(moved.size() + slack);
                freeBytes += slack;
                previous.setSlack(0);
                if (moved.size() >= wanted) {
                    done = true;
                    break;
                }
            }
        } else if (block.isUsed()) {
            if (previous.isFree()) {
                gap += previous.size() + static_cast<ui16>(sizeof(BlockHeader));
                moveOverFree(&block);
            } else if (gap > 0) {
                moveBlock(&block, gap);
            }
        } else if (block.isFree() && gap > 0) {
            const s32 size = block.size();
            block.slideHeader(gap);
            if (gap + size >= wanted) {
                done = true;
                break;
            }
            gap = 0;
        }
        block = next;
    }
    if (!done) {
        const CacheBlock last(usedList.previousInMemory());
        if (gap > 0 || last.slack() > 0) {
            const u16 slack = last.slack();
            CacheBlock rest(last.address + last.size() + slack + static_cast<ui16>(sizeof(BlockHeader)));
            if (slack > 0) {
                rest = rest.address - slack;
                slackBytes -= slack;
                gap += slack;
                last.setSlack(0);
            }
            gap -= sizeof(BlockHeader);
            rest.setNextInMemory(usedList);
            rest.setPreviousInMemory(last);
            usedList.setPreviousInMemory(rest);
            last.setNextInMemory(rest);
            rest.setSize(gap);
            rest.setSlack(0);
            rest.setHandle(FREE_HANDLE);
            rest.setHandler(FREE_HANDLER);
            rest.linkFree();
            freeBytes = gap;
            freeBlockCount = 1;
            totalBlocks++;
        }
    }
    findLargestFree();
}

/* Joins the free block after block to it (1070:11d6). */
void mergeNext(CacheBlock *block)
{
    const CacheBlock next(block->nextInMemory());
    next.unlink();
    block->absorb(next);
    block->setSize(block->size() + next.size() + static_cast<ui16>(sizeof(BlockHeader)));
    freeBytes += sizeof(BlockHeader);
    freeBlockCount--;
    totalBlocks--;
    if (largestFree.address == next.address || block->size() > largestFreeSize) {
        largestFreeSize = block->size();
        largestFree = block->address;
    }
}

void moveBlock(CacheBlock *block, s32 distance)
{
    const u32 to = block->address - distance;
    const u16 handle = block->handle();
    if (handle < NO_HANDLE) {
        cacheHandles[handle] = to + static_cast<ui16>(sizeof(BlockHeader));
    }
    block->relink(to);
    BlockHeader *target = static_cast<BlockHeader *>(cachePointer(to));
    BlockHeader *source = static_cast<BlockHeader *>(cachePointer(block->address));
    /* The block moves down over itself: Borland's memcpy copied forward,
     * which this takes; the C library's memcpy may not (memmove). */
    memmove(target, source, block->size() + static_cast<ui16>(sizeof(BlockHeader)));
    const i16 handler = target->handler;
    if (handler < FREE_HANDLER) {
        handlers[handler]->relocated(to + static_cast<ui16>(sizeof(BlockHeader)));
    }
}

/* Moves block down over the free block before it (1070:13c1). */
void moveOverFree(CacheBlock *block)
{
    const CacheBlock free(block->previousInMemory());
    const u32 to = block->address - (free.size() + static_cast<ui16>(sizeof(BlockHeader)));
    const CacheBlock previous(free.previousInMemory());
    const u16 handle = block->handle();
    if (handle < NO_HANDLE) {
        cacheHandles[handle] = to + static_cast<ui16>(sizeof(BlockHeader));
    }
    block->slideBack(to);
    BlockHeader *target = static_cast<BlockHeader *>(cachePointer(to));
    BlockHeader *source = static_cast<BlockHeader *>(cachePointer(block->address));
    /* Down over itself, as moveBlock's. */
    memmove(target, source, block->size() + block->slack() + static_cast<ui16>(sizeof(BlockHeader)));
    const CacheBlock moved(to);
    moved.setPreviousInMemory(previous);
    freeBytes += sizeof(BlockHeader);
    freeBlockCount--;
    totalBlocks--;
    const i16 handler = target->handler;
    handlers[handler]->relocated(to + static_cast<ui16>(sizeof(BlockHeader)));
}

u16 freeOldest(i16 handler)
{
    CacheBlock block = usedList;
    u16 handle = FREE_HANDLE;
    do {
        block = block.previous();
        if (block.isFree()) {
            break;
        }
        if (block.handler() == handler) {
            handle = block.handle();
            freeBlock(block);
            break;
        }
    } while (block.address != usedList.address);
    return handle;
}

namespace {
/* Where checkBlocks found a fault; empty (1070:15ec). */
/* UNCALLED: nothing calls it. */
void cacheError(std::source_location /*unused*/) {}
}

/* Checks the counters and the lists when checkingCache is set (1070:15f1). */
/* UNCALLED: nothing calls it. */
void checkBlocks(std::source_location place)
{
    if (checkingCache == 0U) {
        return;
    }
    if (largestFree.handle() != FREE_HANDLE && freeBytes != 0) {
        cacheError(place);
    }
    if (static_cast<ui16>(usedBlocks + freeBlockCount) != totalBlocks) {
        cacheError(place);
    }
    if (totalBytes - (static_cast<ui16>(totalBlocks * sizeof(BlockHeader)) + usedBytes + slackBytes +
                      static_cast<ui16>(sizeof(BlockHeader))) !=
        freeBytes) {
        cacheError(place);
    }
    const CacheBlock last(usedList.previousInMemory());
    const s32 size = last.size();
    const u8 slack = last.slack();
    if (last.address + size + slack + static_cast<ui16>(sizeof(BlockHeader)) - usedList.address !=
        static_cast<u32>(totalBytes)) {
        cacheError(place);
    }
    s32 total = 0;
    CacheBlock forward = usedList;
    CacheBlock backward = usedList;
    u16 i;
    for (i = 0; i < totalBlocks; i++) {
        forward = forward.nextInMemory();
        backward = backward.previousInMemory();
        total += forward.size() + backward.size() + forward.slack() + backward.slack() +
                 static_cast<ui16>(2 * sizeof(BlockHeader));
        if ((forward.handle() == HEAD_HANDLE || forward.handle() == FREE_HANDLE) && forward.handler() != FREE_HANDLER) {
            cacheError(place);
        }
        const i16 handler = forward.handler();
        /* Handler 7, DEFAULT_HANDLER, is left out. */
        if (handler < 7 && handlers[handler] != nullptr && !handlers[handler]->check(forward.address)) {
            cacheError(place);
        }
    }
    if (forward.address != usedList.address) {
        cacheError(place);
    }
    if ((total >> 1) != totalBytes - static_cast<ui16>(sizeof(BlockHeader))) {
        cacheError(place);
    }
    total = 0;
    forward = backward = usedList;
    for (i = 0; i < usedBlocks; i++) {
        forward = forward.next();
        backward = backward.previous();
        total += forward.size() + backward.size();
        /* The test of the list in memory again: a free block in the LRU list
         * is not reported. */
        if ((forward.handle() == HEAD_HANDLE || forward.handle() == FREE_HANDLE) && forward.handler() != FREE_HANDLER) {
            cacheError(place);
        }
    }
    if (forward.address != usedList.address || backward.address != usedList.address) {
        cacheError(place);
    }
    if ((total >> 1) != usedBytes) {
        cacheError(place);
    }
    total = 0;
    forward = backward = freeList;
    for (i = 0; i < static_cast<ui16>(freeBlockCount + 1); i++) {
        forward = forward.next();
        backward = backward.previous();
        total += forward.size() + backward.size();
        if (forward.handle() == HEAD_HANDLE || forward.handle() == FREE_HANDLE) {
            if (forward.handler() != FREE_HANDLER) {
                cacheError(place);
            }
        } else {
            cacheError(place);
        }
    }
    if (forward.address != freeList.address || backward.address != freeList.address) {
        cacheError(place);
    }
    if ((total >> 1) != freeBytes) {
        cacheError(place);
    }
    for (i16 handle = 0; handle < CACHE_HANDLES; handle++) {
        u32 data = cacheHandles[handle];
        if (data != 0) {
            data -= sizeof(BlockHeader);
            CacheBlock block = usedList;
            u8 found = 0;
            for (i = 0; (found == 0U) && i < totalBlocks; i++) {
                block = block.nextInMemory();
                if (block.address == data) {
                    found++;
                }
            }
            if (found == 0U) {
                cacheError(place);
            }
        }
    }
}

/* 1070:1bca */
/* UNCALLED: nothing calls it. */
void printCache(i16 /*unused*/) {}

/* 1070:1bcf */
/* UNCALLED: nothing calls it. */
void unusedCache() {}

/* 1070:1bd4 */
/* UNCALLED: nothing calls it. */
void drawCache() {}

/* The used bytes in percent; no references (1070:1bd9). */
/* UNCALLED: nothing calls it. */
i16 cacheUsage()
{
    const s32 used = usedBytes;
    const s32 size = totalBytes;
    const s32 percent = used * 100 / size;
    return percent;
}

/* Saves and loads the cache's memory and state; no references (1070:1c06,
 * 1f77). The cache's memory with its lists as offsets from cacheStart, the
 * counters, the heads and the handle table. */
/* UNCALLED: nothing calls it. */
void saveCache(char *path)
{
    u8 *copy = new u8[totalBytes];
    memcpy(copy, cachePointer(cacheStart.address), totalBytes);
    FILE *file = fileStream(path, "wb");
    if (file == nullptr) {
        openFailure(path);
    }
    fileStreamWrite(file, &totalBytes, 4, 1);
    fileStreamWrite(file, &usedBytes, 4, 1);
    fileStreamWrite(file, &freeBytes, 4, 1);
    fileStreamWrite(file, &largestFreeSize, 4, 1);
    fileStreamWrite(file, &slackBytes, 4, 1);
    fileStreamWrite(file, &totalBlocks, 2, 1);
    fileStreamWrite(file, &usedBlocks, 2, 1);
    fileStreamWrite(file, &freeBlockCount, 2, 1);
    CacheBlock block = usedList;
    do {
        BlockHeader *header = static_cast<BlockHeader *>(cachePointer(block.address));
        const CacheBlock next(header->nextInMemory);
        header->nextInMemory = header->nextInMemory - cacheStart.address;
        header->previousInMemory = header->previousInMemory - cacheStart.address;
        header->next = header->next - cacheStart.address;
        header->previous = header->previous - cacheStart.address;
        block = next;
    } while (block.address != usedList.address);
    BlockHeader *header = static_cast<BlockHeader *>(cachePointer(freeList.address));
    header->nextInMemory = header->nextInMemory - cacheStart.address;
    header->previousInMemory = header->previousInMemory - cacheStart.address;
    header->next = header->next - cacheStart.address;
    header->previous = header->previous - cacheStart.address;
    fileStreamWrite(file, cachePointer(cacheStart.address), totalBytes, 1);
    u32 offset = usedList.address - cacheStart.address;
    fileStreamWrite(file, &offset, 4, 1);
    offset = freeList.address - cacheStart.address;
    fileStreamWrite(file, &offset, 4, 1);
    offset = largestFree.address - cacheStart.address;
    fileStreamWrite(file, &offset, 4, 1);
    fileStreamWrite(file, &handleCount, 2, 1);
    for (i16 handle = 0; handle < handleCount; handle++) {
        u32 data;
        if (cacheHandles[handle] != 0) {
            data = cacheHandles[handle] - cacheStart.address;
        } else {
            data = 0;
        }
        fileStreamWrite(file, &data, 4, 1);
    }
    memcpy(cachePointer(cacheStart.address), copy, totalBytes);
    delete[] copy;
    fileStreamClose(file);
}

/* UNCALLED: nothing calls it. */
void loadCache(char *path)
{
    FILE *file = fileStream(path, "rb");
    if (file == nullptr) {
        openFailure(path);
    }
    /* Nothing looks at what fread reads (the original's). */
    std::ignore = fileStreamRead(file, &totalBytes, 4, 1);
    std::ignore = fileStreamRead(file, &usedBytes, 4, 1);
    std::ignore = fileStreamRead(file, &freeBytes, 4, 1);
    std::ignore = fileStreamRead(file, &largestFreeSize, 4, 1);
    std::ignore = fileStreamRead(file, &slackBytes, 4, 1);
    std::ignore = fileStreamRead(file, &totalBlocks, 2, 1);
    std::ignore = fileStreamRead(file, &usedBlocks, 2, 1);
    std::ignore = fileStreamRead(file, &freeBlockCount, 2, 1);
    std::ignore = fileStreamRead(file, cachePointer(cacheStart.address), totalBytes, 1);
    u32 offset;
    std::ignore = fileStreamRead(file, &offset, 4, 1);
    usedList.address = cacheStart.address + offset;
    std::ignore = fileStreamRead(file, &offset, 4, 1);
    freeList.address = cacheStart.address + offset;
    std::ignore = fileStreamRead(file, &offset, 4, 1);
    largestFree.address = cacheStart.address + offset;
    std::ignore = fileStreamRead(file, &handleCount, 2, 1);
    for (i16 handle = 0; handle < handleCount; handle++) {
        u32 data;
        std::ignore = fileStreamRead(file, &data, 4, 1);
        if (data != 0) {
            cacheHandles[handle] = cacheStart.address + data;
        } else {
            cacheHandles[handle] = 0;
        }
    }
    CacheBlock block = usedList;
    do {
        BlockHeader *header = static_cast<BlockHeader *>(cachePointer(block.address));
        header->nextInMemory = cacheStart.address + header->nextInMemory;
        header->previousInMemory = cacheStart.address + header->previousInMemory;
        header->next = cacheStart.address + header->next;
        header->previous = cacheStart.address + header->previous;
        block = header->nextInMemory;
    } while (block.address != usedList.address);
    BlockHeader *header = static_cast<BlockHeader *>(cachePointer(freeList.address));
    header->nextInMemory = cacheStart.address + header->nextInMemory;
    header->previousInMemory = cacheStart.address + header->previousInMemory;
    header->next = cacheStart.address + header->next;
    header->previous = cacheStart.address + header->previous;
    fileStreamClose(file);
}
