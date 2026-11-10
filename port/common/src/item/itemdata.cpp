/* ..\\ITEM\\ITEMDATA.C (Code25, 10c0:0000-05d5): the item tables. */
#include "gamestate.h"
#include "itemdata.h"
#include "item.h"
#include "itemdam.h"

/* Nothing reads or writes it (1480:8956). */
u8 &unknown8956 = gameGlobal<u8>();
u16 *&itemX = gameGlobal<u16 *>();
u16 *&itemY = gameGlobal<u16 *>();
u8 *&itemZ = gameGlobal<u8 *>();
u16 *&itemShape = gameGlobal<u16 *>();
u8 *&itemFrame = gameGlobal<u8 *>();
u16 *&itemFlags = gameGlobal<u16 *>();
u16 *&itemQuality = gameGlobal<u16 *>();
u8 *&itemNpc = gameGlobal<u8 *>();
u8 *&itemMap = gameGlobal<u8 *>();
u16 *&itemNext = gameGlobal<u16 *>();
u16 *&foundItems = gameGlobal<u16 *>();
u8 *&itemDamagePoints = gameGlobal<u8 *>();

void allocateItems()
{
    itemX = new u16[ITEMS];
    itemY = new u16[ITEMS];
    itemZ = new u8[ITEMS];
    itemShape = new u16[ITEMS];
    itemFrame = new u8[ITEMS];
    itemFlags = new u16[ITEMS];
    itemQuality = new u16[ITEMS];
    itemNpc = new u8[ITEMS];
    itemMap = new u8[ITEMS];
    itemNext = new u16[ITEMS];
    foundItems = new u16[ITEMS];
    itemDamagePoints = new u8[ITEMS];
    memset(itemX, 0, ITEMS * sizeof(u16));
    memset(itemY, 0, ITEMS * sizeof(u16));
    memset(itemZ, 0, ITEMS);
    memset(itemShape, 0, ITEMS * sizeof(u16));
    memset(itemFrame, 0, ITEMS);
    memset(itemFlags, 0, ITEMS * sizeof(u16));
    memset(itemQuality, 0, ITEMS * sizeof(u16));
    memset(itemNpc, 0, ITEMS);
    memset(itemMap, 0, ITEMS);
    memset(itemNext, 0, ITEMS * sizeof(u16));
    memset(foundItems, 0, ITEMS * sizeof(u16));
    memset(itemDamagePoints, 0, ITEMS);
}

void freeItems()
{
    if (itemX != nullptr) {
        delete[] itemX;
        itemX = nullptr;
    }
    if (itemY != nullptr) {
        delete[] itemY;
        itemY = nullptr;
    }
    if (itemZ != nullptr) {
        delete[] itemZ;
        itemZ = nullptr;
    }
    if (itemShape != nullptr) {
        delete[] itemShape;
        itemShape = nullptr;
    }
    if (itemFrame != nullptr) {
        delete[] itemFrame;
        itemFrame = nullptr;
    }
    if (itemFlags != nullptr) {
        delete[] itemFlags;
        itemFlags = nullptr;
    }
    if (itemQuality != nullptr) {
        delete[] itemQuality;
        itemQuality = nullptr;
    }
    if (itemNpc != nullptr) {
        delete[] itemNpc;
        itemNpc = nullptr;
    }
    if (itemMap != nullptr) {
        delete[] itemMap;
        itemMap = nullptr;
    }
    if (itemNext != nullptr) {
        delete[] itemNext;
        itemNext = nullptr;
    }
    if (foundItems != nullptr) {
        delete[] foundItems;
        foundItems = nullptr;
    }
    if (itemDamagePoints != nullptr) {
        delete[] itemDamagePoints;
        itemDamagePoints = nullptr;
    }
}

void readItem(u16 item, ItemRecord *record)
{
    record->x = itemX[item];
    record->y = itemY[item];
    record->z = itemZ[item];
    record->shape = itemShape[item];
    record->frame = itemFrame[item];
    record->flags = itemFlags[item];
    record->quality = itemQuality[item];
    record->npc = itemNpc[item];
    record->map = itemMap[item];
    record->next = itemNext[item];
}

void writeItem(u16 item, ItemRecord *record)
{
    itemX[item] = record->x;
    itemY[item] = record->y;
    itemZ[item] = record->z;
    itemShape[item] = record->shape;
    itemFrame[item] = record->frame;
    itemFlags[item] = record->flags;
    itemQuality[item] = record->quality;
    itemNpc[item] = record->npc;
    itemMap[item] = record->map;
    itemNext[item] = record->next;
    if (damageTable != nullptr) {
        itemDamagePoints[item] = damageTable->entries[record->shape].damagePoints;
    }
}

void resetItem(u16 item, ItemRecord *record)
{
    itemX[item] = record->x;
    itemY[item] = record->y;
    itemZ[item] = record->z;
    itemShape[item] = record->shape;
    itemFrame[item] = record->frame;
    itemFlags[item] = 0;
    itemQuality[item] = 0;
    itemNpc[item] = record->npc;
}

u16 createItem(ItemRecord *record)
{
    Item item(0);

    item.create(record->shape, record->frame);
    itemQuality[item.id] = record->quality;
    itemNpc[item.id] = record->npc;
    itemMap[item.id] = record->map;
    return item.id;
}
