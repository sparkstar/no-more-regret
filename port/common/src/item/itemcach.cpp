/* ..\\ITEM\\ITEMCACH.C (Code24, No Regret's 10b8:0000-2bdf, No Remorse's
 * 10a8:0000-2bd5): the map chunks' item lists, the fast area, the free
 * numbers and the ethereal stack; both games', the addresses No
 * Regret's. */
#include <cstdio>
#include "gamestate.h"
#include <cstdlib>
#include <crash.h>
#include <cstring>
#include "itemcach.h"
#include "waits.h"
#include "crusys.h"
#include "item.h"
#include "type.h"
#include "npc.h"
#include "itemutil.h"
#include "globs.h"
#include "preload.h"
#include "itemdam.h"
#include "flex.h"
#include "utility.h"
#include "kernel.h"
#include "priority.h"
#include "camera.h"
#include "control.h"
#include "tracker.h"
#include "trace.h"
#include "gravity.h"
#include "dlist.h"
#include "dlitem.h"
#include "snap.h"
#include "sprite.h"
#include "viewport.h"
#include "gumpshap.h"
#include "msgbox.h"
#include "dispatch.h"
#include "input.h"
#include "host.h"

u16 popEthereal();

/* Set while the chunk lists change; nothing reads it (1480:1452). */
u8 &itemCacheBusy = gameGlobal<u8>(0);
/* Code7 sets it; nothing reads it. */
u8 &unknown1453 = gameGlobal<u8>(0);
u16 &currentMap = gameGlobal<u16>(0xffff);
u16 (*&mapChunks)[CHUNKS] = gameGlobal<u16 (*)[CHUNKS]>(nullptr);
/* The item records each chunk was given (1480:145a). */
u16 (*&chunkRecords)[CHUNKS] = gameGlobal<u16 (*)[CHUNKS]>(nullptr);
/* static\fixed.dat through the game's paths, and the one in the current
 * directory when there is one (1480:145e, 1462). */
FixedFlex *&fixedFlex = gameGlobal<FixedFlex *>(nullptr);
Flex *&localFixedFlex = gameGlobal<Flex *>(nullptr);
/* Glob eggs expand when their chunk comes into the fast area (1480:1466). */
u8 &expandGlobEggs = gameGlobal<u8>(1);
/* Nothing reads them. */
u8 &unknown1467 = gameGlobal<u8>(1);
char (&itemCacheName)[13] = gameGlobal<char[13]>("itemcach.dat");
u8 &unknown1475 = gameGlobal<u8>(1);

FastArea &fastArea = gameGlobal<FastArea>();
/* The free item numbers from FIRST_ITEM (1480:8946, 894a, 894e) and the
 * free NPC numbers (8948, 894c, 8950): first, last and count. */
u16 &firstFreeItem = gameGlobal<u16>();
u16 &firstFreeNpc = gameGlobal<u16>();
u16 &lastFreeItem = gameGlobal<u16>();
u16 &lastFreeNpc = gameGlobal<u16>();
i16 &freeItemCount = gameGlobal<i16>();
u16 &freeNpcCount = gameGlobal<u16>();
u16 &etherealTop = gameGlobal<u16>();
i16 &etherealCount = gameGlobal<i16>();

/* Moves the ethereal stack's top to the point: the original passes x as z
 * (10b8:2373, 253d). */
inline void popToPoint(ItemPoint &point)
{
    popToCoords(point.x, point.y, point.x);
}

bool FastArea::contains(i16 column, i16 row) const
{
    if (column < 0 || column >= CHUNKS || row < 0 || row >= CHUNKS) {
        return false;
    }
    const i16 sum = column + row;
    const i16 difference = column - row;
    return sumLow <= sum && sumHigh >= sum && differenceLow <= difference && differenceHigh >= difference;
}

void FastArea::centre(u16 x, u16 y)
{
    u16 column;
    u16 row;

    mapChunk(x, y, column, row);
    const i16 sum = column + row;
    const i16 difference = column - row;
    sumLow = sum - 5;
    sumHigh = sum + 5;
    differenceLow = difference - 3;
    differenceHigh = difference + 3;
}

void addChunkRecords(u16 column, u16 row, ItemRecord *records, i16 count)
{
    u8 empty = 0;
    u16 old;
    ItemRecord *record;
    /* Set by the loop below, which runs (count > 0). */
    u16 next = 0;
    i16 i;

    if (count > 0) {
        old = mapChunks[column][row];
        mapChunks[column][row] = firstFreeItem;
        if (old == 0) {
            empty++;
        }
        freeItemCount -= count;
        chunkRecords[column][row] += count;
        record = records;
        u16 item = firstFreeItem;
        u16 last = 0;
        for (i = 0; i < count; i++) {
            if (item == 0) {
                halt();
            }
            last = item;
            next = itemNext[last];
            writeItem(item, record);
            if (itemShape[item] >= SHAPES) {
                itemShape[item] = 0;
                halt();
            }
            itemNext[last] = next;
            itemFlags[last] &= ~ITEM_DRAWN;
            item = next;
            record++;
        }
        if (empty != 0U) {
            itemNext[last] = 0;
        } else {
            itemNext[last] = old;
        }
        firstFreeItem = next;
    }
}

namespace {
/* Expands the glob eggs of the chunk (10b8:01e7). */
void expandChunkGlobs(i16 column, i16 row)
{
    if (column < 0 || column >= CHUNKS || row < 0 || row > CHUNKS) {
        return;
    }
    const FlexEntry entry;
    const Item unused;
    Item egg;

    itemCacheBusy = 1;
    if (expandGlobEggs != 0U) {
        for (u16 item = mapChunks[column][row]; item != 0U; item = itemNext[item]) {
            egg = item;
            const u16 glob = egg.globQuality();
            if (typeFlags.infos[itemShape[item]].family == FAMILY_GLOB_EGG && (glob != 0U)) {
                globHandler->expandEgg(egg, 0);
            }
        }
    }
    itemCacheBusy = 0;
}

/* The items of the chunk enter the fast area (10b8:02c7). */
void enterChunk(i16 column, i16 row)
{
    if (column < 0 || column >= CHUNKS || row < 0 || row > CHUNKS) {
        return;
    }
    Item item;
    u16 next;

    itemCacheBusy = 1;
    for (u16 id = mapChunks[column][row]; id != 0U; id = next) {
        next = itemNext[id];
        item.id = id;
        if (item.z() != FREE_Z && (!item.isNpc() || (item.inFastArea() == 0U)) &&
            ((itemFlags[item.id] & ITEM_FAST_ONLY) == 0)) {
            item.enterFastArea();
        }
    }
    itemCacheBusy = 0;
}

/* The items of the chunk leave it; the fast-only ones go (10b8:0386). */
void leaveChunk(i16 column, i16 row)
{
    if (column < 0 || column >= CHUNKS || row < 0 || row > CHUNKS) {
        return;
    }
    Item item;
    u16 id;
    u16 next;

    for (id = mapChunks[column][row]; id != 0U;) {
        item = id;
        id = itemNext[id];
        Process *process = firstProcess(item.id, GRAVITY_TYPE);
        if (process != nullptr) {
            landNow(process);
        }
    }
    for (id = mapChunks[column][row]; id != 0U; id = next) {
        item = id;
        next = itemNext[id];
        if ((itemFlags[id] & ITEM_FAST_ONLY) != 0) {
            if (typeFlags.infos[item.shape()].noisy && ((itemFlags[id] & ITEM_FAST_AREA) != 0)) {
                item.leaveFastArea();
            }
            item.destroyContents();
            item.destroy();
        } else {
            item.leaveFastArea();
        }
    }
}

/* Takes the items off every chunk when all is set (10b8:04c9). */
void clearChunks(u8 /*unused*/, u8 all)
{
    clearingChunks = 1;
    const Item unused1;
    const Item unused2;
    const FlexEntry entry;
    u16 item;
    u16 next;

    /* The original tests this and does nothing. */
    if (worldActive == 0 && (localFixedFlex != nullptr)) {
        /* empty in the original */
    }
    if (all != 0U) {
        for (i16 row = 0; row < CHUNKS; row++) {
            for (i16 column = 0; column < CHUNKS; column++) {
                item = mapChunks[column][row];
                while (item != 0U) {
                    next = itemNext[item];
                    Item(item).leaveFastArea();
                    if (Item(item).isNpc()) {
                        pushItem(item);
                        popToNowhere(0);
                        item = next;
                        continue;
                    }
                    if (Item(item).contents() != 0U) {
                        Container(item).destroyContents();
                    }
                    Item(item).destroy();
                    item = next;
                }
            }
        }
    }
    clearingChunks = 0;
}
}

/* Reads the items of map from fixed.dat; map 0 clears the NPCs too, a
 * negative map is only stored (10b8:05e3). */
/* loadMap's locals over the waits of its items' cacheIn (the port's), its
 * loops going on after them: the records', from a record, and the NPCs',
 * from an NPC. */
struct MapLoad {
    i16 map = 0;
    u16 count = 0;
    u16 remaining = 0;
    u16 bad = 0;
    u16 null = 0;
    u16 i = 0;
    i16 column = 0;
    i16 row = 0;
    ItemRecord *record = nullptr;
    u16 container = 0;
    u16 item = 0;
    u16 depth = 0;
    s32 offset = 0;
    Flex *flex = nullptr;
};
namespace {
/* The records' buffer of the map being loaded (the original's local), which
 * releaseMapLoad frees if the run ends in a wait of it. */
ItemRecord *&mapBuffer = gameGlobal<ItemRecord *>(nullptr);
/* The bad items' warning's text, which the box keeps while it is open (the
 * original's local, on its stack through the box's loop). */
char (&badItemsWarning)[80] = gameGlobal<char[80]>();

void loadMapRecords(MapLoad load, bool resumed);
void loadMapNpcs(MapLoad load, u16 from);

void loadMap(i16 map)
{
    FlexEntry entry;
    const Item unused;

    if (map < 0) {
        currentMap = map;
        return;
    }
    /* The targets of the map before go (No Remorse's original restarts the
     * tracker in Npc::teleport instead). */
    closeTracker();
    initTracker();
    itemCacheBusy = 1;
    if (map == 0) {
        if (itemZ[1] != FREE_Z) {
            Container(1).destroyContents();
        }
        for (u16 i = 2; i < FIRST_ITEM; i++) {
            if (itemZ[i] != FREE_Z) {
                Container(i).destroyContents();
                Item(i).destroy();
            }
            itemFlags[i] = 0;
            itemZ[i] = FREE_Z;
        }
    }
    ItemRecord *buffer = new ItemRecord[0xfa00 / sizeof(ItemRecord)];
    mapBuffer = buffer;
    MapLoad load;
    load.map = map;
    if (localFixedFlex != nullptr) {
        load.flex = localFixedFlex;
    } else {
        load.flex = fixedFlex;
    }
    load.flex->readEntry(map, &entry);
    load.count = load.remaining = entry.size / 16;
    if (load.count > static_cast<ui16>(freeItemCount)) {
        halt();
    }
    load.container = load.item = 0;
    load.depth = 0;
    load.offset = 0;
    load.bad = load.null = 0;
    loadMapRecords(load, false);
}

/* loadMap's records, from the start of a block, or after the record whose
 * cacheIn opened a wait (resumed). */
void loadMapRecords(MapLoad load, bool resumed)
{
    for (;;) {
        if (resumed) {
            resumed = false;
            load.record++;
            load.i++;
        } else {
            if (load.remaining == 0U) {
                break;
            }
            load.record = mapBuffer;
            load.count = load.remaining > 4000 ? 4000 : load.remaining;
            load.remaining -= load.count;
            load.flex->readObject(load.map, mapBuffer, static_cast<s32>(load.count) << 4, load.offset);
            load.offset += static_cast<ui16>(load.count << 4);
            load.i = 0;
        }
        for (; load.i < load.count; load.i++) {
            ItemRecord *record = load.record;
            if (record->shape == 0) {
                load.null++;
                load.record++;
                continue;
            }
            const i16 family = typeFlags.infos[record->shape].family;
            if (family == FAMILY_CONTAINER || family == FAMILY_MONSTER_EGG) {
                record->quality = 0;
            }
            if ((record->flags & ITEM_CONTAINED) != 0) {
                /* x is the depth in the container. */
                if (record->x > load.depth) {
                    load.container = load.item;
                    load.depth = record->x;
                }
                while (record->x < load.depth) {
                    load.container = Item(load.container).container();
                    load.depth--;
                }
                load.item = newItem();
                writeItem(load.item, record);
                popToEnd(load.container);
                if (((itemFlags[load.item] & ITEM_EQUIPPED) != 0) && (Item(load.container).isNpc())) {
                    Npc(load.container).setEquip(record->z, load.item);
                }
                itemY[load.item] = record->y;
            } else if (load.map == 0) {
                load.depth = 0;
                load.item = record->npc;
                writeItem(load.item, record);
                pushItem(load.item);
                Npc(load.item).clearEquip();
                popItem();
            } else {
                load.depth = 0;
                load.item = firstFreeItem;
                pointInFastArea(record->x, record->y, &load.column, &load.row);
                addChunkRecords(load.column, load.row, record, 1);
            }
            if (load.map != 0 && typeFlags.infos[record->shape].noisy) {
                Item(load.item).cacheIn();
                if (waitOpened()) {
                    giveRest([load]() {
                        loadMapRecords(load, true);
                        return false;
                    });
                    return;
                }
            }
            load.record++;
        }
    }
    delete[] mapBuffer;
    mapBuffer = nullptr;
    firstFreeNpc = lastFreeNpc = 0;
    freeNpcCount = 0;
    loadMapNpcs(load, 0xff);
}

/* loadMap's NPCs, from the one at from down, and its end. */
void loadMapNpcs(MapLoad load, u16 from)
{
    for (load.item = from; load.item > 0; load.item--) {
        if (itemZ[load.item] == FREE_Z && load.item >= 2) {
            itemNext[load.item] = firstFreeNpc;
            firstFreeNpc = load.item;
            if (freeNpcCount == 0) {
                lastFreeNpc = load.item;
            }
            freeNpcCount++;
        } else if (itemMap[load.item] == load.map && load.map != 0) {
            pushItem(load.item);
            popItem();
            Item(load.item).cacheIn();
            if (waitOpened()) {
                const u16 next = load.item - 1;
                giveRest([load, next]() {
                    loadMapNpcs(load, next);
                    return false;
                });
                return;
            }
        }
    }
    if (load.map != 0) {
        preloadMap(load.map);
    }
    currentMap = load.map;
    traceMapLoaded(load.map);
    rebuildScreen();
    /* Nothing counts bad items: the warning never shows. The rest after its
     * box. */
    if (load.bad != 0U) {
        gameCamera->flush();
#pragma GCC diagnostic push
        /* The original's 80 bytes are too few for the largest counts, which
         * the size cuts short; nothing counts bad items (above). GCC sees it
         * without optimizing. */
#pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(badItemsWarning, sizeof(badItemsWarning),
                 "WARNING!\n%d bad item%s and %d null item%s in map %d.\nContinue at your own risk.", load.bad,
                 load.bad == 1 ? "" : "s", load.null, load.null == 1 ? "" : "s", load.map);
#pragma GCC diagnostic pop
        openModalGump(new MessageBoxGump(NewGumpId(), nullptr, badItemsWarning, guiColors->face), static_cast<u32>(-1),
                      [](s32 /*result*/) { itemCacheBusy = 0; });
        return;
    }
    itemCacheBusy = 0;
}
}

void releaseMapLoad()
{
    if (mapBuffer != nullptr) {
        delete[] mapBuffer;
        mapBuffer = nullptr;
    }
}

/* Puts the NPCs back and the fast area again (10b8:0ac9). */
/* UNCALLED: nothing calls it. */
void reloadNpcs()
{
    const i16 map = currentMap;
    const u16 x = cameraX();
    const u16 y = cameraY();

    clearChunks(1, 1);
    currentMap = 0;
    for (u16 npc = 1; npc < FIRST_ITEM; npc++) {
        if (itemZ[npc] != FREE_Z) {
            pushItem(npc);
            popItem();
        }
    }
    clearChunks(1, 1);
    setFastArea(x, y, map);
}

/* Reads the map's items again (10b8:0b3c). */
/* UNCALLED: nothing calls it. */
void reloadMap()
{
    const i16 map = currentMap;
    const u16 x = cameraX();
    const u16 y = cameraY();

    clearChunks(1, 1);
    currentMap = 0;
    loadMap(0);
    clearChunks(1, 1);
    setFastArea(x, y, map);
}

void saveItems(Flex *flex, i16 index)
{
    s32 offset;
    FlexEntry entry;

    flex->moveObject(index, -1);
    flex->readEntry(index, &entry);
    offset = 0;
    flex->writeObject(index, itemX, static_cast<ui16>(ITEMS * sizeof(u16)), offset, 0);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->writeObject(index, itemY, static_cast<ui16>(ITEMS * sizeof(u16)), offset, 0);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->writeObject(index, itemZ, ITEMS, offset, 0);
    offset += ITEMS;
    flex->writeObject(index, itemShape, static_cast<ui16>(ITEMS * sizeof(u16)), offset, 0);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->writeObject(index, itemFrame, ITEMS, offset, 0);
    offset += ITEMS;
    flex->writeObject(index, itemFlags, static_cast<ui16>(ITEMS * sizeof(u16)), offset, 0);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->writeObject(index, itemQuality, static_cast<ui16>(ITEMS * sizeof(u16)), offset, 0);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->writeObject(index, itemNpc, ITEMS, offset, 0);
    offset += ITEMS;
    flex->writeObject(index, itemMap, ITEMS, offset, 0);
    offset += ITEMS;
    flex->writeObject(index, itemNext, static_cast<ui16>(ITEMS * sizeof(u16)), offset, 0);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->writeObject(index, foundItems, static_cast<ui16>(ITEMS * sizeof(u16)), offset, 0);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->writeObject(index, itemDamagePoints, ITEMS, offset, 0);
    /* The original counts the byte table as words: the object ends in 2800h
     * bytes of nothing. */
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    entry.size = offset;
    flex->writeEntry(index, &entry);
    index++;
    flex->moveObject(index, -1);
    flex->readEntry(index, &entry);
    offset = 0;
    flex->writeObject(index, &currentMap, 2, offset, 0);
    offset += 2;
    flex->writeObject(index, &fastArea, 8, offset, 0);
    offset += 8;
    flex->writeObject(index, mapChunks, static_cast<ui16>(CHUNKS * CHUNKS * sizeof(u16)), offset, 0);
    offset += static_cast<ui16>(CHUNKS * CHUNKS * sizeof(u16));
    flex->writeObject(index, chunkRecords, static_cast<ui16>(CHUNKS * CHUNKS * sizeof(u16)), offset, 0);
    offset += static_cast<ui16>(CHUNKS * CHUNKS * sizeof(u16));
    flex->writeObject(index, &freeItemCount, 2, offset, 0);
    offset += 2;
    flex->writeObject(index, &freeNpcCount, 2, offset, 0);
    offset += 2;
    flex->writeObject(index, &firstFreeItem, 2, offset, 0);
    offset += 2;
    flex->writeObject(index, &firstFreeNpc, 2, offset, 0);
    offset += 2;
    flex->writeObject(index, &lastFreeItem, 2, offset, 0);
    offset += 2;
    flex->writeObject(index, &lastFreeNpc, 2, offset, 0);
    offset += 2;
    flex->writeObject(index, &etherealTop, 2, offset, 0);
    offset += 2;
    flex->writeObject(index, &etherealCount, 2, offset, 0);
    offset += 2;
    entry.size = offset;
    flex->writeEntry(index, &entry);
}

bool loadItems(Flex *flex, i16 index)
{
    s32 offset;

    offset = 0;
    flex->readObject(index, itemX, static_cast<ui16>(ITEMS * sizeof(u16)), offset);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->readObject(index, itemY, static_cast<ui16>(ITEMS * sizeof(u16)), offset);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->readObject(index, itemZ, ITEMS, offset);
    offset += ITEMS;
    flex->readObject(index, itemShape, static_cast<ui16>(ITEMS * sizeof(u16)), offset);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->readObject(index, itemFrame, ITEMS, offset);
    offset += ITEMS;
    flex->readObject(index, itemFlags, static_cast<ui16>(ITEMS * sizeof(u16)), offset);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->readObject(index, itemQuality, static_cast<ui16>(ITEMS * sizeof(u16)), offset);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->readObject(index, itemNpc, ITEMS, offset);
    offset += ITEMS;
    flex->readObject(index, itemMap, ITEMS, offset);
    offset += ITEMS;
    flex->readObject(index, itemNext, static_cast<ui16>(ITEMS * sizeof(u16)), offset);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->readObject(index, foundItems, static_cast<ui16>(ITEMS * sizeof(u16)), offset);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    flex->readObject(index, itemDamagePoints, ITEMS, offset);
    offset += static_cast<ui16>(ITEMS * sizeof(u16));
    for (i16 i = 0; i < 0x2800; i++) {
        itemFlags[i] &= ~ITEM_DRAWN;
    }
    index++;
    offset = 0;
    flex->readObject(index, &currentMap, 2, offset);
    offset += 2;
    flex->readObject(index, &fastArea, 8, offset);
    offset += 8;
    flex->readObject(index, mapChunks, static_cast<ui16>(CHUNKS * CHUNKS * sizeof(u16)), offset);
    offset += static_cast<ui16>(CHUNKS * CHUNKS * sizeof(u16));
    flex->readObject(index, chunkRecords, static_cast<ui16>(CHUNKS * CHUNKS * sizeof(u16)), offset);
    offset += static_cast<ui16>(CHUNKS * CHUNKS * sizeof(u16));
    flex->readObject(index, &freeItemCount, 2, offset);
    offset += 2;
    flex->readObject(index, &freeNpcCount, 2, offset);
    offset += 2;
    flex->readObject(index, &firstFreeItem, 2, offset);
    offset += 2;
    flex->readObject(index, &firstFreeNpc, 2, offset);
    offset += 2;
    flex->readObject(index, &lastFreeItem, 2, offset);
    offset += 2;
    flex->readObject(index, &lastFreeNpc, 2, offset);
    offset += 2;
    flex->readObject(index, &etherealTop, 2, offset);
    offset += 2;
    flex->readObject(index, &etherealCount, 2, offset);
    preloadMap(0);
    return true;
}

namespace {
void fastAreaChunks(u16 x, u16 y, u8 changed);
}

void setFastArea(u16 x, u16 y, i16 map)
{
    u8 changed;

    if (map == 0) {
        map = currentMap;
    }
    changed = static_cast<u8>(currentMap != static_cast<ui16>(map));
    if (changed != 0U) {
        rebuildScreen();
        if (currentMap != 0U) {
            clearChunks(1, 1);
        }
        loadMap(map);
        if (waitOpened()) {
            /* The map's chunks after its loading's wait. */
            giveRest([x, y, changed]() {
                fastAreaChunks(x, y, changed);
                return false;
            });
            return;
        }
    }
    fastAreaChunks(x, y, changed);
}

namespace {
/* setFastArea's after the map's loading: the chunks the area leaves and
 * enters. */
void fastAreaChunks(u16 x, u16 y, u8 changed)
{
    i16 column;
    i16 row;
    i16 sum;
    i16 difference;
    const FastArea old = fastArea;
    FastArea area;
    area.centre(x, y);
    if (changed == 0U) {
        for (sum = old.sumLow; sum <= old.sumHigh; sum++) {
            for (difference = old.differenceLow; difference <= old.differenceHigh; difference++) {
                if ((static_cast<i16>((sum + difference) & 1)) == 0) {
                    column = static_cast<u16>(sum + difference) >> 1;
                    row = static_cast<u16>(sum - difference) >> 1;
                    if (!area.contains(column, row) && old.contains(column, row)) {
                        leaveChunk(column, row);
                    }
                }
            }
        }
    }
    fastArea = area;
    for (sum = fastArea.sumLow; sum <= fastArea.sumHigh; sum++) {
        for (difference = fastArea.differenceLow; difference <= fastArea.differenceHigh; difference++) {
            if ((static_cast<i16>((sum + difference) & 1)) == 0) {
                column = static_cast<u16>(sum + difference) >> 1;
                row = static_cast<u16>(sum - difference) >> 1;
                if ((!old.contains(column, row) || (changed != 0U)) && area.contains(column, row)) {
                    expandChunkGlobs(column, row);
                }
            }
        }
    }
    for (sum = fastArea.sumLow; sum <= fastArea.sumHigh; sum++) {
        for (difference = fastArea.differenceLow; difference <= fastArea.differenceHigh; difference++) {
            if ((static_cast<i16>((sum + difference) & 1)) == 0) {
                column = static_cast<u16>(sum + difference) >> 1;
                row = static_cast<u16>(sum - difference) >> 1;
                if ((!old.contains(column, row) || (changed != 0U)) && area.contains(column, row)) {
                    enterChunk(column, row);
                }
            }
        }
    }
}
}

/* Loads the current map again around the camera (10b8:161c). */
/* UNCALLED: nothing calls it. */
void resetFastArea()
{
    const i16 map = currentMap;

    currentMap = 0xffff;
    setFastArea(cameraX(), cameraY(), map);
}

void initItemCache()
{
    i16 column;
    i16 row;
    i16 item;

    allocateItems();
    fixedFlex = new FixedFlex;
    if (fileExists("static\\fixed.dat") != 0) {
        localFixedFlex = new Flex("static\\fixed.dat", 0, -1);
    } else {
        localFixedFlex = nullptr;
    }
    if (fixedFlex == nullptr) {
        allocationFailure();
    }
    mapChunks = new u16[CHUNKS][CHUNKS];
    chunkRecords = new u16[CHUNKS][CHUNKS];
    for (column = 0; column < CHUNKS; column++) {
        for (row = 0; row < CHUNKS; row++) {
            mapChunks[column][row] = 0;
            chunkRecords[column][row] = 0;
        }
    }
    for (item = 0; item < 0x2800; item++) {
        itemNext[item] = item + 1;
        itemZ[item] = FREE_Z;
    }
    itemNext[FIRST_ITEM - 1] = 0;
    itemNext[0x27ff] = 0;
    firstFreeNpc = 2;
    lastFreeNpc = FIRST_ITEM - 1;
    freeNpcCount = FIRST_ITEM - 1;
    firstFreeItem = FIRST_ITEM;
    lastFreeItem = 0x27ff;
    itemNext[0] = 0;
    freeItemCount = 0x2700;
    etherealTop = 0;
    etherealCount = 0;
    currentMap = 0xffff;
}

void resetNpcNumbers()
{
    for (i16 npc = 0; npc < FIRST_ITEM - 1; npc++) {
        itemNext[npc] = npc + 1;
        itemZ[npc] = FREE_Z;
    }
    itemNext[FIRST_ITEM - 1] = 0;
    firstFreeNpc = 2;
    lastFreeNpc = FIRST_ITEM - 1;
    freeNpcCount = FIRST_ITEM - 1;
}

void closeItemCache()
{
    if (fixedFlex != nullptr) {
        delete fixedFlex;
        fixedFlex = nullptr;
    }
    if (localFixedFlex != nullptr) {
        delete localFixedFlex;
        localFixedFlex = nullptr;
    }
    if (mapChunks != nullptr) {
        delete[] mapChunks;
        mapChunks = nullptr;
    }
    if (chunkRecords != nullptr) {
        delete[] chunkRecords;
        chunkRecords = nullptr;
    }
    freeItems();
}

namespace {
/* The item before item in its list, 0 for the first (10b8:1885). */
u16 previousItem(u16 item)
{
    u16 next;

    if (itemZ[item] == FREE_Z) {
        if (item < FIRST_ITEM) {
            next = firstFreeNpc;
        } else {
            next = firstFreeItem;
        }
    } else if ((itemFlags[item] & ITEM_ETHEREAL) != 0) {
        next = etherealTop;
    } else if ((itemFlags[item] & ITEM_CONTAINED) != 0) {
        /* A contained item's x is its container, whose quality is its
         * first item. */
        next = itemQuality[itemX[item]];
    } else {
        i16 column;
        i16 row;
        pointInFastArea(itemX[item], itemY[item], &column, &row);
        next = mapChunks[column][row];
    }
    u16 previous = 0;
    while (next != item && (next != 0U)) {
        previous = next;
        next = itemNext[next];
    }
    return previous;
}

/* Take the item out of its container, its chunk, the free lists and the
 * ethereal stack (10b8:1953, 19ac, 1a5c, 1b4d). */
void unlinkContained(u16 item)
{
    const u16 previous = previousItem(item);
    const u16 next = itemNext[item];

    if (previous != 0U) {
        itemNext[previous] = next;
    } else {
        const Item container(itemX[item]);
        container.setContents(next);
    }
}

void unlinkChunk(u16 item)
{
    const u16 previous = previousItem(item);
    const u16 next = itemNext[item];
    i16 column;
    i16 row;

    pointInFastArea(itemX[item], itemY[item], &column, &row);
    if (mapChunks[column][row] == item) {
        mapChunks[column][row] = next;
    }
    if (previous != 0U) {
        itemNext[previous] = next;
    }
    chunkRecords[column][row]--;
}

void unlinkFree(u16 item)
{
    const u16 previous = previousItem(item);

    if (item < FIRST_ITEM) {
        if (item == lastFreeNpc) {
            lastFreeNpc = 0;
        }
        if (previous != 0U) {
            itemNext[previous] = itemNext[item];
        } else {
            firstFreeNpc = itemNext[item];
        }
        freeNpcCount--;
        return;
    }
    if (item == lastFreeItem) {
        lastFreeItem = 0;
    }
    if (previous != 0U) {
        itemNext[previous] = itemNext[item];
    } else {
        firstFreeItem = itemNext[item];
    }
    freeItemCount--;
}

/* The item number becomes free (10b8:1af9). */
void freeNumber(u16 item)
{
    if (item < FIRST_ITEM) {
        itemNext[item] = firstFreeNpc;
        if (firstFreeNpc == lastFreeNpc) {
            lastFreeNpc = item;
        }
        firstFreeNpc = item;
        freeNpcCount++;
        return;
    }
    itemNext[item] = firstFreeItem;
    if (firstFreeItem == lastFreeItem) {
        lastFreeItem = item;
    }
    firstFreeItem = item;
    freeItemCount++;
}

void unlinkEthereal(u16 item)
{
    const u16 previous = previousItem(item);
    const u16 next = itemNext[item];

    if (previous != 0U) {
        itemNext[previous] = next;
    } else {
        etherealTop = next;
    }
    etherealCount--;
}
}

void removeItem(u16 item)
{
    if (item == 1) {
        return;
    }
    const Item gone(item);

    if (((gone.isOccluding() != 0U) || (gone.isTargetable() != 0U)) && (gone.isBroken() == 0U)) {
        removeTarget(item);
    }
    if ((targeting != 0U) && targetItem == item) {
        updateTarget();
    }
    if (itemZ[item] >= 0xfb || item == 0) {
        return;
    }
    if (gone.contents() != 0U) {
        Container(item).removeContents();
    }
    if ((itemFlags[item] & ITEM_ETHEREAL) != 0) {
        unlinkEthereal(item);
    } else {
        pushItem(item);
        popEthereal();
    }
    freeNumber(item);
    itemZ[item] = FREE_Z;
    clearItemBit(screenItems, item);
    clearItemBit(movedItems, item);
    killItemProcesses(item, ANY_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
}

bool pointInFastArea(u16 x, u16 y, i16 *column, i16 *row)
{
    const u16 chunkColumn = x >> CHUNK_SHIFT;
    const u16 chunkRow = y >> CHUNK_SHIFT;

    *column = chunkColumn;
    *row = chunkRow;
    return fastArea.contains(chunkColumn, chunkRow);
}

void pushItem(u16 item)
{
    if (freeItemCount <= 0) {
        halt();
    }
    if (item == 0) {
        halt();
    }
    const Item pushed(item);

    if ((itemFlags[item] & ITEM_EQUIPPED) != 0) {
        Npc owner(pushed.container());
        owner.removeEquip(item);
        itemFlags[item] &= ~ITEM_EQUIPPED;
    }
    if ((itemFlags[item] & ITEM_ETHEREAL) == 0) {
        if (itemZ[item] == FREE_Z) {
            unlinkFree(item);
        } else if ((itemFlags[item] & ITEM_CONTAINED) != 0) {
            unlinkContained(item);
        } else {
            if (clearingChunks == 0U) {
                updateItem(item);
            }
            unlinkChunk(item);
        }
        itemNext[item] = etherealTop;
        itemFlags[item] |= ITEM_ETHEREAL;
        etherealTop = item;
        etherealCount++;
    }
}

void popToCoords(u16 x, u16 y, u8 z)
{
    i16 column = x >> CHUNK_SHIFT;
    i16 row = y >> CHUNK_SHIFT;

    chunkRecords[column][row]++;
    const u16 item = popEthereal();
    if (item == 0U) {
        return;
    }
    Item placed(item);

    if (placed.isSilencer() && (silencerSprite != 0U)) {
        Sprite *sprite = static_cast<Sprite *>(findProcess(silencerSprite));
        sprite->shift(x - placed.x(), y - placed.y(), z - placed.z());
    }
    itemX[item] = x;
    itemY[item] = y;
    if (z >= 0xfb) {
        z = 0xfa;
    }
    itemZ[item] = z;
    if (!pointInFastArea(x, y, &column, &row)) {
        if ((itemFlags[item] & ITEM_FAST_AREA) != 0) {
            placed.leaveFastArea();
        }
    } else if ((itemFlags[item] & ITEM_FAST_AREA) == 0) {
        placed.enterFastArea();
    }
    if ((snapProcess->holdsCamera == 0U) && item == movedActor && followedItem == movedActor) {
        i16 screenX;
        i16 screenY;
        i16 dx;
        i16 dy;
        i16 cameraScreenX;
        i16 cameraScreenY;
        u16 mapX = 0;
        u16 mapY = 0;

        mapToScreen(x, y, &screenX, &screenY);
        screenY -= z;
        dx = 0;
        dy = 0;
        if ((screenViewport->flags() & VIEWPORT_VGA) != 0) {
            if (screenY > drawViewport->bottomBound() - 150) {
                dy = screenY - (drawViewport->top() + 100);
            }
            if (screenY < drawViewport->topBound() + 100) {
                dy = screenY - (drawViewport->bottom() - 150);
            }
            if (screenX > drawViewport->rightBound() - 160) {
                dx = screenX - (drawViewport->left() + 160);
            }
            if (screenX < drawViewport->leftBound() + 160) {
                dx = screenX - (drawViewport->right() - 160);
            }
        } else if (screenY < drawViewport->topBound() + 140 || screenY > drawViewport->bottomBound() - 70 ||
                   screenX > drawViewport->rightBound() - 100 || screenX < drawViewport->leftBound() + 100) {
            i16 edge = -1;
            if (screenY < 160) {
                if (screenX < 120) {
                    edge = 2;
                } else if (static_cast<i16>(screenViewport->width() - 120) < screenX) {
                    edge = 4;
                } else {
                    edge = 5;
                }
            } else if (static_cast<i16>(screenViewport->height() - 90) < screenY) {
                /* The bottom left corner is switched off: the camera does
                 * not follow to the left there (original behaviour). */
                /* NOLINTNEXTLINE(readability-simplify-boolean-expr): the original's disabled test */
                if ((false) && screenX < 120) {
                    edge = 2;
                } else if (static_cast<i16>(screenViewport->width() - 120) < screenX) {
                    edge = 4;
                } else {
                    edge = 5;
                }
            } else if (screenX < 120) {
                edge = 2;
            } else if (static_cast<i16>(screenViewport->width() - 120) < screenX) {
                edge = 4;
            }
            if (edge == -1) {
                halt();
            }
            switch (edge) {
            case 1:
                dx = screenX - screenViewport->width() / 2;
                dy = screenY - 160;
                break;
            case 2:
                dx = screenX - (screenViewport->right() - screenViewport->left() + 1 - 120);
                dy = screenY - (screenViewport->height() / 2 + 40);
                break;
            case 3:
                dx = screenX - screenViewport->width() / 2;
                dy = screenY - (screenViewport->bottom() - screenViewport->top() + 1 - 90);
                break;
            case 4:
                dx = screenX - 120;
                dy = screenY - (screenViewport->height() / 2 + 40);
                break;
            case 5:
                dx = screenX - screenViewport->width() / 2;
                dy = screenY - (screenViewport->height() / 2 + 40);
                break;
            }
        }
        if ((dx != 0) || (dy != 0)) {
            mapToScreen(cameraX(), cameraY(), &cameraScreenX, &cameraScreenY);
            dx += cameraScreenX;
            dy += cameraScreenY;
            screenToMap(dx, dy, &mapX, &mapY);
            moveCamera(mapX, mapY, 0, 0);
        }
    }
    itemNext[item] = mapChunks[column][row];
    mapChunks[column][row] = item;
    itemFlags[item] &= ~(ITEM_CONTAINED | ITEM_ETHEREAL);
    if (clearingChunks == 0U) {
        if ((typeFlags.infos[itemShape[item]].draw || z < cameraLevel()) && ((itemFlags[item] & ITEM_FAST_AREA) != 0)) {
            redrawItem(item);
        }
        setItemBit(movedItems, item);
    }
}

void popToContainer(u16 container)
{
    if (typeFlags.infos[itemShape[container]].family == FAMILY_CONTAINER) {
        const u16 item = popEthereal();
        if (item == 0U) {
            return;
        }
        const Item popped(item);
        if (popped.isNpc()) {
            popToPoint(lvalue(Item(container).point()));
            return;
        }
        itemNext[item] = Item(container).contents();
        Item(container).setContents(item);
        itemX[item] = container;
        itemZ[item] = 0;
        itemFlags[item] &= ~ITEM_ETHEREAL;
        itemFlags[item] |= ITEM_CONTAINED;
        return;
    }
    popToPoint(lvalue(Item(container).point()));
}

void popItem()
{
    const u16 item = etherealTop;
    const Item top(item);

    if ((itemFlags[item] & ITEM_CONTAINED) != 0) {
        const u16 y = itemY[item];
        popToContainer(top.container());
        itemY[item] = y;
        return;
    }
    popToCoords(itemX[item], itemY[item], itemZ[item]);
}

void popToEnd(u16 container)
{
    if (typeFlags.infos[itemShape[container]].family == FAMILY_CONTAINER ||
        typeFlags.infos[itemShape[container]].family == FAMILY_MONSTER_EGG) {
        const u16 item = popEthereal();
        if (item == 0U) {
            return;
        }
        u16 last = 0;
        for (u16 next = Item(container).contents(); next != 0U; next = itemNext[next]) {
            last = next;
        }
        const Item added(item);
        if (last != 0U) {
            itemNext[last] = item;
        } else {
            Item(container).setContents(item);
        }
        itemNext[item] = 0;
        itemX[item] = container;
        itemZ[item] = 0;
        itemFlags[item] &= ~ITEM_ETHEREAL;
        itemFlags[item] |= ITEM_CONTAINED;
        return;
    }
    popToPoint(lvalue(Item(container).point()));
}

void popToNowhere(u8 clear)
{
    const u16 item = popEthereal();

    if (item == 0) {
        return;
    }
    itemFlags[item] &= ~ITEM_ETHEREAL;
    if (clear != 0U) {
        itemMap[item] = 0;
        if (item >= FIRST_ITEM) {
            halt();
        }
    }
}

/* Takes the top off the ethereal stack, 0 when it is empty (10b8:26ce). */
u16 popEthereal()
{
    if (etherealCount <= 0) {
        return 0;
    }
    etherealCount--;
    const u16 item = etherealTop;
    etherealTop = itemNext[item];
    return item;
}

/* 10b8:26f8 */
u16 topEthereal()
{
    return etherealTop;
}

u16 newItem()
{
    if (freeItemCount <= 0) {
        halt();
    }
    itemFlags[firstFreeItem] = 0;
    itemQuality[firstFreeItem] = 0;
    itemNpc[firstFreeItem] = 0;
    itemMap[firstFreeItem] = 0;
    const u16 item = firstFreeItem;
    pushItem(item);
    return item;
}

u16 newNpc()
{
    if (firstFreeNpc == 0) {
        return 0;
    }
    itemFlags[firstFreeNpc] = 0;
    itemQuality[firstFreeNpc] = 0;
    itemNpc[firstFreeNpc] = 0;
    const u16 npc = firstFreeNpc;
    pushItem(npc);
    return npc;
}

/* The item numbers left when every chunk and glob of the map is in use
 * (10b8:27b0). */
/* UNCALLED: nothing calls it. */
i16 spareItems()
{
    i16 column;
    i16 row;
    i16 difference;
    u16 item;
    i16(*counts)[CHUNKS] = new i16[CHUNKS][CHUNKS];
    i16 spare = 0x2600;

    for (row = 0; row < CHUNKS; row++) {
        for (column = 0; column < CHUNKS; column++) {
            counts[column][row] = 0;
            for (item = mapChunks[column][row]; item != 0U; item = itemNext[item]) {
                if ((itemFlags[item] & ITEM_FAST_ONLY) == 0) {
                    spare -= Container(item).contentsCount() + 1;
                }
                if (typeFlags.infos[itemShape[item]].family == FAMILY_GLOB_EGG) {
                    const u16 glob = itemQuality[item];
                    if (glob != 0U) {
                        counts[column][row] = globHandler->itemCount(glob);
                    }
                }
            }
        }
    }
    i16 sum;
    i16 most = 0;
    for (row = 0; row < CHUNKS; row++) {
        for (column = 0; column < CHUNKS; column++) {
            sum = 0;
            for (difference = -3; difference <= 3; difference++) {
                for (i16 i = -5; i <= 5; i++) {
                    const i16 c = column + (static_cast<u16>(i + difference) >> 1);
                    const i16 r = row + (static_cast<u16>(i - difference) >> 1);
                    if (c >= 0 && c < CHUNKS && r >= 0 && r < CHUNKS) {
                        sum += counts[c][r];
                    }
                }
            }
            if (sum > most) {
                most = sum;
            }
        }
    }
    spare -= most;
    delete[] counts;
    return spare;
}

/* The NPC numbers in use subtracted from FEh (10b8:2973). */
/* UNCALLED: nothing calls it. */
i16 countFreeNpcs()
{
    i16 count = FREE_Z;

    for (i16 npc = 2; npc < FIRST_ITEM; npc++) {
        if (itemZ[npc] != FREE_Z) {
            count--;
        }
    }
    return count;
}

/* 10b8:299e */
/* UNCALLED: nothing calls it. */
void verifyFailed(i16 map)
{
    hostPrint("Verify Failed on %d.\n", map);
    if (inputKey() == 'x') {
        char reason[80];
        snprintf(reason, sizeof(reason), "Verify Failed on map %d", map);
        crashReport(reason);
        hostExit(-1);
    }
}

/* Prints the item lists (10b8:29c6). */
/* UNCALLED: nothing calls it. */
void dumpItems()
{
    Item item;
    const char *free;
    const char *last;
    const char *ethereal;

    hostPrint("\nfreeCount = %d\n", freeItemCount);
    hostPrint("                         Referent  Next X    Y    Z  \n");
    hostPrint("                         --------  ---- ---- ---- ---\n");
    char chunk[20];
    char unused[20];

    for (i16 id = 1; id < 0x2800; id++) {
        strcpy(chunk, "       ");
        strcpy(unused, "       ");
        for (i16 column = 0; column < CHUNKS; column++) {
            for (i16 row = 0; row < CHUNKS; row++) {
                if (mapChunks[column][row] == static_cast<ui16>(id)) {
                    snprintf(chunk, sizeof(chunk), "S %02d %02d", column, row);
                }
            }
        }
        if (static_cast<ui16>(id) == firstFreeItem) {
            free = "FF";
        } else {
            free = "  ";
        }
        if (static_cast<ui16>(id) == firstFreeNpc) {
            free = "FFn";
        } else {
            free = "  ";
        }
        if (static_cast<ui16>(id) == lastFreeItem) {
            last = "LF";
        } else {
            last = "  ";
        }
        if (static_cast<ui16>(id) == lastFreeNpc) {
            last = "LFn";
        } else {
            last = "  ";
        }
        if (static_cast<ui16>(id) == etherealTop) {
            ethereal = "ET";
        } else {
            ethereal = "  ";
        }
        item = id;
        hostPrint("%s %s %s %s %s %4d      %4d %4d %4d", chunk, unused, free, last, ethereal, id, item.next(), item.x(),
                  item.y());
        if ((item.flags() & ITEM_ETHEREAL) != 0) {
            hostPrint(" ETHEREAL\n");
        } else if (item.z() == FREE_Z) {
            hostPrint(" FREE\n");
        } else {
            hostPrint(" %3d\n", item.z());
        }
    }
}

void startItemCache()
{
    initItemCache();
}

/* 10b8:2bbe */
/* UNCALLED: nothing calls it. */
void itemCacheNothing() {}
