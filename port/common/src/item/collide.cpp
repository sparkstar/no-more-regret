/* Code31 (10f0:0000-190a, file name unknown): moving a shape's box through
 * the map. */
#include <cstdlib>
#include "collide.h"
#include "item.h"
#include "type.h"
#include "itemcach.h"
#include "itemfind.h"
#include "refstore.h"
#include "blocking.h"

Sweep::Sweep()
{
    hits = nullptr;
    item = 0;
    targetFirst = targetHit = 0;
}

Sweep::~Sweep()
{
    clearHits();
}

void Sweep::reset()
{
    clearHits();
    targetFirst = 0;
    targetHit = 0;
}

void Sweep::clearHits()
{
    while (hits != nullptr) {
        Hit *hit = hits;
        hits = hits->next;
        delete hit;
    }
}

bool Sweep::fits(u16 shape, ItemPoint *point, u8 turned)
{
    ShapeInfo *info = &typeFlags.infos[shape];

    shapeBox(width, depth, height, info);
    if (turned != 0U) {
        const i16 swap = width;
        width = depth;
        depth = swap;
    }
    mask = blockMask(info);
    u16 blockers;
    u8 z;
    i16 top = (z = point->z) + height - 1;
    if (top >= 0x100) {
        top = 0xff;
    }
    blockers = blockersAt(point->x - width + 1, point->y - depth + 1, point->x, point->y, z, top, &blocker);
    return (mask & blockers) == 0;
}

void Sweep::addHit(u16 metItem, i16 x, i16 y, i16 z, u8 blocking)
{
    if (blocking != 0U) {
        if (abs(x) < abs(moveX) || abs(y) < abs(moveY) || abs(z) < abs(moveZ)) {
            if (target == metItem) {
                targetHit = 1;
                if (hitItem == 0) {
                    targetFirst = 1;
                }
            } else {
                targetFirst = 0;
            }
            hitItem = metItem;
            moveX = x;
            moveY = y;
            moveZ = z;
        }
    }
    Hit *hit = new Hit;
    if (hit != nullptr) {
        hit->next = hits;
        hits = hit;
        hit->item = metItem;
        hit->x = x;
        hit->y = y;
        hit->z = z;
    }
}

void Sweep::collide(u16 other, Move *move)
{
    i16 otherWidth;
    i16 otherDepth;
    i16 otherHeight;
    s32 product;
    /* The other item's box from the start, in map units. */
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
    s32 low;
    s32 high;
    Item otherItem;
    bool touches;
    const i16 x = move->x;
    const i16 y = move->y;
    const i16 z = move->z;

    otherItem = ItemRef(other);
    ShapeInfo *info = &typeFlags.infos[itemShape[otherItem.id]];

    shapeBox(otherWidth, otherDepth, otherHeight, info);
    if ((itemFlags[otherItem.id] & ITEM_FLIPPED) != 0) {
        const i16 swap = otherWidth;
        otherWidth = otherDepth;
        otherDepth = swap;
    }
    if (move->fall != 0U) {
        const i16 gap = move->end.z - (itemZ[otherItem.id] + otherHeight);
        if (gap >= 0 && drop >= gap) {
            left = static_cast<i16>(itemX[otherItem.id] - otherWidth + 1);
            top = static_cast<i16>(itemY[otherItem.id] - otherDepth + 1);
            right = static_cast<i16>(itemX[otherItem.id]);
            bottom = static_cast<i16>(itemY[otherItem.id]);
            if (static_cast<ui16>(move->end.x - width + 1) <= right &&
                static_cast<ui16>(move->end.y - depth + 1) <= bottom && move->end.x >= left && move->end.y >= top) {
                /* A variable: the original keeps the bit in CX. */
                const u16 floorFails = SWEEP_FLOOR_FAILS;
                if ((options & floorFails) != 0) {
                    move->blockedBelow = 1;
                }
                /* The second test of the bit always passes here. */
                else if (info->solid && ((~options & floorFails) != 0)) {
                    move->supported = 1;
                    drop = gap;
                    if (gap == 0) {
                        addHit(otherItem.id, 0, 0, 0, 0);
                    }
                }
            }
        }
    }
    const u8 blocks = blockMask(info) & mask;
    touches = info->damaging || (move->damaging != 0U);
    if (blocks == 0 && !touches) {
        return;
    }
    left = static_cast<i16>(itemX[otherItem.id] - otherWidth + 1 - move->start.x);
    top = static_cast<i16>(itemY[otherItem.id] - otherDepth + 1 - move->start.y);
    right = static_cast<i16>(itemX[otherItem.id] + width - 1 - move->start.x);
    bottom = static_cast<i16>(itemY[otherItem.id] + depth - 1 - move->start.y);
    high = static_cast<i16>(itemZ[otherItem.id] + otherHeight - 1 - move->start.z);
    low = static_cast<i16>(itemZ[otherItem.id] - height + 1 - move->start.z);
    if ((x != 0) && x >= left && left > 0) {
        product = y * left;
        if (bottom * x >= product && top * x <= product) {
            product = z * left;
            if (high * x >= product && low * x <= product) {
                if (blocks != 0U) {
                    move->clear = 0;
                    blockedAxes |= 1;
                }
                left--;
                addHit(otherItem.id, left, y * left / x, z * left / x, blocks);
                return;
            }
        }
    }
    if ((y != 0) && y >= top && top > 0) {
        product = x * top;
        if (right * y >= product && left * y <= product) {
            product = z * top;
            if (high * y >= product && low * y <= product) {
                if (blocks != 0U) {
                    move->clear = 0;
                    blockedAxes |= 2;
                }
                top--;
                addHit(otherItem.id, x * top / y, top, z * top / y, blocks);
                return;
            }
        }
    }
    if ((z != 0) && z >= low && low > 0) {
        product = x * low;
        if (right * z >= product && left * z <= product) {
            product = y * low;
            if (bottom * z >= product && top * z <= product) {
                if (blocks != 0U) {
                    move->clear = 0;
                    blockedAxes |= 4;
                }
                low--;
                addHit(otherItem.id, x * low / z, y * low / z, low, blocks);
                return;
            }
        }
    }
    if ((x != 0) && x <= right && right < 0) {
        product = y * right;
        if (bottom * x <= product && top * x >= product) {
            product = z * right;
            if (high * x <= product && low * x >= product) {
                if (blocks != 0U) {
                    move->clear = 0;
                    blockedAxes |= 1;
                }
                right++;
                addHit(otherItem.id, right, y * right / x, z * right / x, blocks);
                return;
            }
        }
    }
    if ((y != 0) && y <= bottom && bottom < 0) {
        product = x * bottom;
        if (right * y <= product && left * y >= product) {
            product = z * bottom;
            if (high * y <= product && low * y >= product) {
                if (blocks != 0U) {
                    move->clear = 0;
                    blockedAxes |= 2;
                }
                bottom++;
                addHit(otherItem.id, x * bottom / y, bottom, z * bottom / y, blocks);
                return;
            }
        }
    }
    if ((z != 0) && z <= high && high < 0) {
        product = x * high;
        if (right * z <= product && left * z >= product) {
            product = y * high;
            if (bottom * z <= product && top * z >= product) {
                if (blocks != 0U) {
                    move->clear = 0;
                    blockedAxes |= 4;
                }
                high++;
                addHit(otherItem.id, x * high / z, y * high / z, high, blocks);
            }
        }
    }
}

bool Sweep::test(u16 shape, ItemPoint &start, i16 x, i16 y, i16 z, u8 flags, u16 movingItem, u16 testMask,
                 u16 aimedItem)
{
    i16 left;
    i16 top;
    i16 right;
    i16 bottom;
    ShapeInfo info;
    Move move;

    move.clear = 1;
    move.fall = static_cast<u8>((flags & MOVE_NO_FLOOR) == 0);
    move.blockedBelow = 0;
    move.supported = 0;
    move.damaging = 0;
    info = typeFlags.infos[shape];
    if (shape == 0) {
        info.x = info.y = info.z = 0;
        info.fixed = 0;
        info.solid = 1;
        info.damaging = 0;
    }
    this->item = movingItem;
    this->target = aimedItem;
    outside = 0;
    drop = 0;
    info.box(width, depth, height);
    if (movingItem != 0U) {
        if ((ItemRef(movingItem).flags() & ITEM_FLIPPED) != 0) {
            const i16 swap = width;
            width = depth;
            depth = swap;
        }
    }
    if (testMask != 0U) {
        this->mask = testMask;
    } else {
        this->mask = blockMask(&info) & 0x7fff;
    }
    if (info.damaging) {
        move.damaging = 1;
    }
    options = 2 | SWEEP_FLOOR_FAILS;
    blockedAxes = 0;
    if ((flags & MOVE_STANDS) != 0) {
        options &= ~SWEEP_FLOOR_FAILS;
    }
    if ((flags & MOVE_STEP_DOWN) != 0) {
        drop = 9;
    }
    end = ItemPoint(x, y, z);
    fullX = moveX = move.x = x - start.x;
    fullY = moveY = move.y = y - start.y;
    fullZ = moveZ = move.z = z - start.z;
    hitItem = 0;
    if (start.z > 0xfa) {
        return false;
    }
    if (x < 0) {
        end.x = move.x < 0 ? 0 : 0x7fff;
        blockedAxes |= 1;
        outside = 1;
        move.clear = 0;
    }
    if (y < 0) {
        end.y = move.y < 0 ? 0 : 0x7fff;
        blockedAxes |= 2;
        outside = 1;
        move.clear = 0;
    }
    if (z < 0 || z > 0xfa) {
        end.z = z < 0 ? 0 : 0xfa;
        blockedAxes |= 4;
        move.clear = 0;
    }
    i16 column;
    i16 row;
    if (!pointInFastArea(start.x - width + 1, start.y - depth + 1, &column, &row) ||
        (!pointInFastArea(start.x, start.y - depth + 1, &column, &row)) ||
        (!pointInFastArea(start.x - width + 1, start.y, &column, &row)) ||
        (!pointInFastArea(start.x, start.y, &column, &row))) {
        end = start;
        return false;
    }
    if (!pointInFastArea(x - width + 1, y - depth + 1, &column, &row) ||
        (!pointInFastArea(x, y - depth + 1, &column, &row)) || !pointInFastArea(x - width + 1, y, &column, &row) ||
        (!pointInFastArea(x, y, &column, &row))) {
        end = start;
        blockedAxes |= 3;
        outside = 1;
        return false;
    }
    left = start.x - width;
    top = start.y - depth;
    right = start.x + 0x100;
    bottom = start.y + 0x100;
    i16 edge = x - width;
    if (edge < left) {
        left = edge;
    }
    edge = y - depth;
    if (edge < top) {
        top = edge;
    }
    /* The far edges compare as map places, unsigned. */
    edge = x + 0x100;
    if (static_cast<u16>(edge) > static_cast<ui16>(right)) {
        right = edge;
    }
    edge = y + 0x100;
    if (static_cast<u16>(edge) > static_cast<ui16>(bottom)) {
        bottom = edge;
    }
    if (left < 0) {
        left = 0;
    }
    if (top < 0) {
        top = 0;
    }
    if (right < 0) {
        right = 0x7fff;
    }
    if (bottom < 0) {
        bottom = 0x7fff;
    }
    move.end.x = x;
    move.end.y = y;
    move.end.z = z;
    move.start = start;
    for (AreaList list(left, top, right, bottom); list.isValid(); list.next()) {
        if (list.id != movingItem) {
            collide(list.id, &move);
        }
    }
    if (hitItem != 0U) {
        end.x = moveX + start.x;
        end.y = moveY + start.y;
        end.z = moveZ + start.z;
        if (end.z > 0xfa) {
            end.z = static_cast<i16>(moveZ + start.z) < 0 ? 0 : 0xfa;
        }
    }
    return (move.clear != 0U) && ((move.fall != 0U) ? ((move.supported != 0U) && (move.blockedBelow == 0U)) : true);
}

bool Sweep::moveItem(u16 movingItem, u16 x, u16 y, u16 z, u8 flags, u16 aimedItem)
{
    ItemRef moving(movingItem);

    this->item = movingItem;
    this->target = aimedItem;
    if (flags == 0) {
        flags = MOVE_STANDS;
    }
    return test(moving.shape(), lvalue(moving.point()), x, y, z, flags, movingItem, 0, this->target);
}

bool Sweep::moveShape(u16 shape, ItemPoint &from, ItemPoint &to, u8 flags, u16 movingItem, u16 aimedItem)
{
    this->item = movingItem;
    this->target = aimedItem;
    if (flags == 0) {
        flags = MOVE_STANDS;
    }
    return test(shape, from, to, flags, movingItem, this->target);
}

void Sweep::apply()
{
    /* Set and read under the same hitItem. */
    i16 force = 0;
    Hit *hit;

    if (item != 0U) {
        if (hitItem != 0U) {
            const i16 x = abs(fullX >> 2);
            const i16 y = abs(fullY >> 2);
            const i16 z = abs(fullZ);
            i16 most = x > y ? x : y;
            most = most > z ? most : z;
            force = static_cast<i16>(x + y + z + most) >> 1;
        }
        Item(item).move(end.x, end.y, end.z);
        i16 count = 0;
        if (hitItem != 0U) {
            count += 2;
        }
        for (hit = hits; hit != nullptr; hit = hit->next) {
            if (hit->item != hitItem &&
                (abs(hit->x) <= abs(moveX) || abs(hit->y) <= abs(moveY) || abs(hit->z) <= abs(moveZ))) {
                count += 2;
            }
        }
        RefStore store(count);
        if (hitItem != 0U) {
            const i16 family = typeFlags.infos[ItemRef(item).shape()].family;
            if ((family == FAMILY_QUANTITY || family == FAMILY_REAGENT) &&
                ItemRef(hitItem).shape() == ItemRef(item).shape()) {
                Item(item).absorb(hitItem);
            } else {
                store.add(new StorageData(&store, STORE_HIT, item, hitItem, force));
                store.add(new StorageData(&store, STORE_GOT_HIT, hitItem, item, force));
            }
        }
        for (hit = hits; hit != nullptr; hit = hit->next) {
            if (hit->item != hitItem &&
                (abs(hit->x) <= abs(moveX) || abs(hit->y) <= abs(moveY) || abs(hit->z) <= abs(moveZ))) {
                store.add(new StorageData(&store, STORE_HIT, item, hit->item, 0));
                store.add(new StorageData(&store, STORE_GOT_HIT, hit->item, item, 0));
            }
        }
        item = 0;
        if (count != 0) {
            store.flush();
        }
    }
}

bool HitSweep::collideMove(u16 movingItem, u16 x, u16 y, u16 z, u8 flags, u16 aimedItem)
{
    ItemRef moving(movingItem);

    return step(moving.shape(), lvalue(moving.point()), lvalue(ItemPoint(x, y, z)), flags, movingItem, aimedItem);
}

bool HitSweep::step(u16 shape, ItemPoint &from, ItemPoint &to, u8 flags, u16 movingItem, u16 aimedItem)
{
    climb = 0;
    this->item = movingItem;
    this->target = aimedItem;
    if (flags == 0) {
        flags = MOVE_STANDS;
    }
    if (to.z == from.z) {
        if (moveShape(shape, from, to, flags | MOVE_STEP_DOWN, movingItem, this->target)) {
            climb = -drop;
            if ((drop != 0) && test(shape, to, to.x, to.y, to.z + climb, flags, movingItem, 0, 0)) {
                end.z = to.z + climb;
            }
            return true;
        }
        if ((flags & MOVE_CLIMB) != 0) {
            ItemPoint up;
            if ((this->target != 0U) && (targetFirst != 0U)) {
                return false;
            }
            up.x = from.x;
            up.y = from.y;
            up.z = from.z + 8;
            if (moveShape(shape, from, up, MOVE_NO_FLOOR, movingItem, this->target)) {
                if (test(shape, up, to.x, to.y, to.z + 8, flags | MOVE_STEP_DOWN, movingItem, 0, 0)) {
                    climb = 8 - drop;
                    end.z = to.z + climb;
                    return true;
                }
                climb = 0;
            }
            up.z = from.z + 9;
            if (moveShape(shape, from, up, MOVE_NO_FLOOR, movingItem, this->target)) {
                if (test(shape, up, to.x, to.y, to.z + 9, flags | MOVE_STEP_DOWN, movingItem, 0, 0)) {
                    climb = 9;
                    end.z = to.z + climb;
                    return true;
                }
                climb = 0;
                return false;
            }
        }
    } else {
        return moveShape(shape, from, to, flags, movingItem, this->target);
    }
    return false;
}

bool shapeFits(u16 shape, u16 x, u16 y, u16 z, u8 /*unused*/, u16 /*unused*/, u16 /*unused*/, u16 *blocker)
{
    Sweep sweep;
    ItemPoint where;
    ItemPoint point;

    where.set(x, y, z);
    /* The original checks a copy. */
    point = where;

    if (sweep.fits(shape, &point, 0)) {
        return true;
    }
    if (blocker != nullptr) {
        *blocker = sweep.blocker;
    }
    return false;
}
