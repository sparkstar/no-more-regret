/* ..\\ITEM\\GLOB.C (Code22, 10a8:0000-0877): globs, the groups of items
 * kept in static\glob.flx. */
#include "gamestate.h"
#include "globs.h"
#include "item.h"
#include "itemcach.h"
#include "preload.h"
#include "cache.h"
#include "cacheblk.h"
#include "handler.h"
#include "gamefile.h"
#include "filespec.h"
#include "dlist.h"
#include "rect.h"
#include "host.h"

GlobFlex *&globFlex = gameGlobal<GlobFlex *>(nullptr); /* 1480:1380 */
GlobCacheHandler *&globHandler = gameGlobal<GlobCacheHandler *>(nullptr);
/* The first of the globs' cache slots, and the glob in each slot (FFFFh
 * for none; 1480:853c, 853e). */
u16 &firstGlobHandle = gameGlobal<u16>();
u16 (&globNumbers)[GLOB_HANDLES] = gameGlobal<u16[GLOB_HANDLES]>();

GlobFlex::GlobFlex() : Flex(makePath(nullptr, staticDirectory, gameFiles[GAME_GLOB], nullptr), FILE_READ, GLOBS) {}

GlobCacheHandler::GlobCacheHandler() : CacheHandler(GLOB_HANDLER)
{
    firstGlobHandle = allocateHandles(GLOB_HANDLES);
    registerCacheHandler(this, GLOB_HANDLER);
    memset(globNumbers, 0xff, sizeof globNumbers);
    globFlex = new GlobFlex;
}

GlobCacheHandler::~GlobCacheHandler()
{
    if (globFlex != nullptr) {
        delete globFlex;
        globFlex = nullptr;
    }
}

s32 GlobCacheHandler::globSize(u16 glob)
{
    FlexEntry entry;

    globFlex->readEntry(glob, &entry);
    return entry.size;
}

Glob *GlobCacheHandler::globData(u16 glob)
{
    i16 handle = -2;
    u32 address = 0;
    i16 i;

    for (i = 0; i < GLOB_HANDLES; i++) {
        if (globNumbers[i] == glob) {
            handle = firstGlobHandle + i;
            break;
        }
    }
    if (handle != -2) {
        address = cacheHandles[handle];
        CacheBlock(address - static_cast<ui16>(sizeof(BlockHeader))).touch();
    } else {
        s32 size;

        for (i = 0; i < GLOB_HANDLES; i++) {
            if (globNumbers[i] == 0xffff) {
                handle = firstGlobHandle + i;
                globNumbers[i] = glob;
                break;
            }
        }
        if (i >= GLOB_HANDLES) {
            handle = freeOldest(GLOB_HANDLER);
            globNumbers[static_cast<ui16>(handle - firstGlobHandle)] = glob;
        }
        size = globSize(glob);
        if (size < 2) {
            size = 2;
            address = cacheAllocate(size, handle, GLOB_HANDLER);
            *static_cast<u16 *>(cachePointer(address)) = 0;
        } else {
            address = cacheAllocate(size, handle, GLOB_HANDLER);
            globFlex->readObject(glob, cachePointer(address), -1, 0);
        }
        if (address == 0) {
            halt();
        }
    }
    return static_cast<Glob *>(cachePointer(address));
}

void GlobCacheHandler::evicted(u32 /*data*/, u16 handle)
{
    globNumbers[static_cast<ui16>(handle - firstGlobHandle)] = 0xffff;
}

i16 GlobCacheHandler::expandEgg(Item egg, u8 force)
{
    TableItem item(egg.id);
    const u16 glob = item.globQuality();

    return expandGlob(glob, item.x(), item.y(), item.z(), force);
}

i16 GlobCacheHandler::expandGlob(u16 glob, u16 x, u16 y, u8 z, u8 force)
{
    i16 count;
    i16 i;
    ItemRecord *records;
    GlobEntry *entry;
    ItemRecord *record;
    Item item;
    u16 column;
    u16 row;
    u16 left;
    u16 top;
    Glob *data;

    data = globData(glob);
    count = data->count;
    records = new ItemRecord[count];
    record = records;
    data = globData(glob);
    mapChunk(x, y, column, row);
    chunkCorner(column, row, left, top);
    left++;
    top++;
    if (count > freeItemCount) {
        halt();
    }
    entry = data->entries;
    for (i = 0; i < count; i++) {
        record->x = (entry->x << 1) + left;
        record->y = (entry->y << 1) + top;
        record->z = entry->z + z;
        record->shape = entry->shape;
        record->frame = entry->frame;
        record->flags = ITEM_DISPOSABLE | ITEM_FAST_ONLY;
        record->quality = 0;
        record->npc = 0;
        record->map = 0;
        if (record->shape == 0) {
            halt();
        }
        record++;
        entry++;
    }
    if (force == 0U) {
        i16 screenX;
        i16 screenY;
        Rect area(0, 0, 0, 0);
        Rect screen(0, 0, 0, 0);

        screenX = viewX;
        screenY = viewY;
        screen = Rect(screenX, screenY, screenX + 0x2ff, screenY + 0x27f);
        screenX = (left >> 1) - (top >> 1);
        screenY = (left >> 2) + (top >> 2);
        area = Rect(screenX - 0x178, screenY - 0xfa, screenX + 0x177, screenY + 0xff);
        if (area.touches(screen)) {
            force = 1;
        }
    }
    if (force != 0U) {
        record = records;
        for (i = 0; i < count; i++) {
            if (item.create(record->shape, record->frame)) {
                if (item.map() != 0) {
                    halt();
                }
                item.popToCoords(record->x, record->y, record->z);
                item.orFlags(ITEM_DISPOSABLE | ITEM_FAST_ONLY);
                record++;
            } else {
                /* The original stays on the record that failed. */
                debugPrintf(50, "COULD NOT CREATE GLOB ITEM!");
                hostDebugBreak();
            }
        }
    } else {
        addChunkRecords(column, row, records, count);
    }
    delete[] records;
    return count;
}

void GlobCacheHandler::markFrames(u16 glob)
{
    GlobEntry *entry;
    i16 count;
    Glob *data;

    data = globData(glob);
    count = data->count;
    if (count > GLOB_ITEMS) {
        halt();
    }
    for (i16 i = 0; i < count; i++) {
        /* The glob may move in the cache: its data is asked for again. */
        data = globData(glob);
        entry = &data->entries[i];
        markFrame(entry->shape, entry->frame);
    }
}

/* UNCALLED: nothing calls it. */
void GlobCacheHandler::discard(u16 glob)
{
    for (i16 i = 0; i < GLOB_HANDLES; i++) {
        if (globNumbers[i] == glob) {
            CacheBlock(cacheHandles[static_cast<ui16>(i + firstGlobHandle)] - static_cast<ui16>(sizeof(BlockHeader)))
                .release();
            globNumbers[i] = 0xffff;
            return;
        }
    }
}

/* UNCALLED: nothing calls it. */
u16 GlobCacheHandler::itemCount(u16 glob)
{
    Glob *data = globData(glob);

    return data->count;
}

void createGlobHandler()
{
    globHandler = new GlobCacheHandler;
}

void deleteGlobHandler()
{
    if (globHandler != nullptr) {
        delete globHandler;
        globHandler = nullptr;
    }
}
