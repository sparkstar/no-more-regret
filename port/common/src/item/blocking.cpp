/* Code4 (1018, file name unknown): what a shape is blocked by and what blocks
 * a box of the map. */
#include "blocking.h"
#include "item.h"
#include "itemdata.h"
#include "itemfind.h"
#include "npc.h"

u16 blockersInCells(i16 left, i16 top, i16 right, i16 bottom, i16 low, i16 high);

u16 blockMask(ShapeInfo *info)
{
    u16 mask = 0x8000;

    if (info->solid) {
        mask |= 1;
    }
    if (info->damaging) {
        mask |= 0x20;
    }
    return mask;
}

/* blockersInCells for the one cell of map point x,y at level z (1018:0032).
 * Nothing calls it. */
u16 blockersAtPoint(u16 x, u16 y, u8 z)
{
    x >>= 4;
    y >>= 4;
    z >>= 3;
    return blockersInCells(x, y, x, y, z, z);
}

/* blockersInCells for one cell (1018:0061). Nothing calls it. */
u16 blockersInCell(i16 column, i16 row, i16 level)
{
    return blockersInCells(column, row, column, row, level, level);
}

/* blockersAt for the cells from left,top to right,bottom (16 map units
 * each) and the levels from low to high (8 units each); FFFFh when a bound
 * is negative (1018:007d). */
u16 blockersInCells(i16 left, i16 top, i16 right, i16 bottom, i16 low, i16 high)
{
    i16 x1;
    i16 y1;
    i16 x2;
    i16 y2;
    u8 z1;
    u8 z2;

    if (left < 0 || top < 0 || right < 0 || bottom < 0) {
        return 0xffff;
    }
    x1 = left << 4;
    y1 = top << 4;
    x2 = (right << 4) + 15;
    y2 = (bottom << 4) + 15;
    z1 = low << 3;
    /* Original behaviour: shifting the byte by 10 leaves 0, so only the items
     * at level 0 count. */
    z2 = high << 10;
    return blockersAt(x1, y1, x2, y2, z1, z2, nullptr);
}

u16 blockersAt(i16 left, i16 top, i16 right, i16 bottom, u8 low, u8 high, u16 *first)
{
    i16 width;
    i16 depth;
    i16 height;
    u16 blockers = 0;

    if (left < 0 || top < 0 || right < 0 || bottom < 0) {
        return 0xffff;
    }
    if (low == 0 && high == 0xff) {
        return 0;
    }
    Item item;
    ShapeInfo *info;
    AreaList list(left, top, right + 0x100, bottom + 0x100);
    if (first != nullptr) {
        *first = 0;
    }
    while (list.isValid()) {
        item = list;
        if (itemX[item.id] >= static_cast<ui16>(left) && itemY[item.id] >= static_cast<ui16>(top) &&
            itemZ[item.id] <= high) {
            info = &typeFlags.infos[itemShape[item.id]];
            shapeBox(width, depth, height, info);
            if (static_cast<ui16>(itemX[item.id] - width + 1) <= static_cast<ui16>(right) &&
                static_cast<ui16>(itemY[item.id] - depth + 1) <= static_cast<ui16>(bottom) &&
                static_cast<i16>(itemZ[item.id] + height - 1) >= low) {
                blockers |= blockMask(info) & 0x7fff;
                if ((first != nullptr) && ((blockers & 1) != 0)) {
                    *first = item.id;
                    first = nullptr;
                }
            }
        }
        list.id = foundItems[list.index];
        list.index++;
    }
    return blockers;
}

/* 1 when item stands on the floor, is contained, or stands on an item that
 * blocks as it does (1018:02de). Nothing calls it. */
bool restsOnBlocker(u16 id)
{
    Npc item(id);
    const u8 z = itemZ[item.id];
    i16 x;
    i16 y;
    ShapeInfo *info;
    u16 mask;
    i16 width;
    i16 depth;
    i16 height;
    u16 blockers;

    if (z == 0 || ((itemFlags[item.id] & ITEM_CONTAINED) != 0)) {
        return true;
    }
    blockers = 0;
    x = itemX[item.id];
    y = itemY[item.id];
    info = &typeFlags.infos[itemShape[item.id]];
    mask = blockMask(info);
    shapeBox(width, depth, height, info);
    const i16 left = x - width + 1;
    const i16 top = y - depth + 1;
    AreaList list(left, top, x + 0x100, y + 0x100);
    while (list.isValid()) {
        item = list;
        if (item.id != id && itemX[item.id] >= static_cast<ui16>(left) && itemY[item.id] >= static_cast<ui16>(top) &&
            itemZ[item.id] <= z) {
            info = &typeFlags.infos[itemShape[item.id]];
            shapeBox(width, depth, height, info);
            if (static_cast<ui16>(itemX[item.id] - width + 1) <= static_cast<ui16>(x) &&
                static_cast<ui16>(itemY[item.id] - depth + 1) <= static_cast<ui16>(y) &&
                static_cast<i16>(itemZ[item.id] + height) == z) {
                blockers |= blockMask(info) & 0x7fff;
            }
        }
        list.id = foundItems[list.index];
        list.index++;
    }
    return (mask & blockers) != 0;
}
