/* TRACKER.C (Code40, 1138): the items that can be targeted, 100 at most,
 * and the hurt an explosion does around a point. */
#include "tracker.h"
#include "gamestate.h"
#include "itemdata.h"
#include "itemfind.h"
#include "distance.h"
#include "control.h"
#include "crusader.h"
#include "flex.h"
#include "shapes.h"

u16 *&targets = gameGlobal<u16 *>(nullptr);
u16 &targetSize = gameGlobal<u16>(100);
u16 &targetCount = gameGlobal<u16>(0);

void initTracker()
{
    targetSize = 100;
    /* The port's: the original leaves the last map's table. */
    delete[] targets;
    targets = new u16[targetSize];
    for (u16 i = 0; i < targetSize; i++) {
        targets[i] = 0;
    }
}

void closeTracker()
{
    if (targets != nullptr) {
        delete[] targets;
        targetSize = 0;
        targetCount = 0;
        targets = nullptr;
    }
}

void addTarget(u16 item)
{
    u16 i;
    if (targets != nullptr) {
        for (i = 0; i < targetSize; i++) {
            if ((static_cast<u8>(targets[i] != 0)) == 0U) {
                targets[i] = item;
                targetCount++;
                return;
            }
        }
    }
}

void removeTarget(u16 item)
{
    u16 i;
    if (targets != nullptr) {
        for (i = 0; i < targetSize; i++) {
            if (targets[i] == item) {
                targets[i] = 0;
                targetCount--;
                return;
            }
        }
    }
}

u16 findTarget(u16 npc, char direction, u8 sixteen)
{
    u16 i;
    const Item self(npc);
    Item target;
    i16 x1;
    i16 y1;
    i16 z1;
    i16 x2;
    i16 y2;
    i16 z2;
    u16 distance;
    u16 best = 0xffff;
    i16 bestIndex = -1;
    i16 dir;
    bool foundNpc = false;
    bool foundOccluding = false;
    u16 seen = 0;
    x1 = itemX[self.id];
    y1 = itemY[self.id];
    z1 = itemZ[self.id];
    for (i = 0; i < targetSize && seen < targetCount; i++) {
        if ((static_cast<u8>(targets[i] != 0)) != 0U) {
            seen++;
            target = targets[i];
            if (target.isNpc() || !foundNpc) {
                if ((target.isOccluding() != 0U) || !foundOccluding) {
                    if (npc != target.id) {
                        if (target.isOnScreen()) {
                            x2 = itemX[target.id];
                            y2 = itemY[target.id];
                            z2 = itemZ[target.id];
                            if (sixteen != 0U) {
                                dir = closestDirection(x1, y1, x2, y2, 16, 0, 15);
                            } else {
                                dir = closestDirection(x1, y1, x2, y2, 8, 0, 7);
                                dir <<= 1;
                            }
                            if (direction == dir) {
                                distance = mapDistance(x1, y1, z1, x2, y2, z2);
                                if (distance < best || (target.isNpc() && !foundNpc) ||
                                    ((target.isOccluding() != 0U) && !foundOccluding)) {
                                    best = distance;
                                    bestIndex = i;
                                    if (target.isNpc()) {
                                        foundNpc = true;
                                    }
                                    if (target.isOccluding() != 0U) {
                                        foundOccluding = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (bestIndex == -1) {
        return 0;
    }
    if (best > 0x400) {
        return 0;
    }
    return targets[bestIndex];
}

void hurtArea(ItemPoint *point, i16 radius, i16 damage, i16 type, u16 except, u16 attacker)
{
    const i16 reach = radius << 4;
    const i16 left = point->x - reach;
    const i16 top = point->y - reach;
    const i16 right = point->x + reach;
    const i16 bottom = point->y + reach;
    i16 distance;
    i16 hurt;
    u8 direction;
    AreaList list;
    list.init(left, top, right, bottom);
    if (damage > 250) {
        damage = 250;
    }
    while (list.isValid()) {
        if (list.z() != ETHEREAL_Z && list.shape() != SHAPE_SHOT && list.id != except &&
            (list.id != attacker || attacker == controlledActor)) {
            if (type == 3 || type == 4 || type == 0xa) {
                distance =
                    mapDistance(point->x, point->y, point->z, itemX[list.id], itemY[list.id], itemZ[list.id]) / 16;
                distance /= 3;
                if (distance == 0) {
                    distance = 1;
                }
                hurt = damage / distance;
            } else {
                hurt = damage;
            }
            direction = fineDirection(point->x, point->y, itemX[list.id], itemY[list.id]);
            takeHit(&list, 0, direction, hurt, type, point);
        }
        list.id = foundItems[list.index];
        list.index++;
    }
}

void loadTargets(Flex *flex, u16 index)
{
    flex->readObject(index, &targetSize, -1, 0);
    flex->readObject(index + 1, &targetCount, -1, 0);
    flex->readObject(index + 2, targets, -1, 0);
}

void saveTargets(Flex *flex, u16 index)
{
    flex->writeObject(index, &targetSize, 2, 0, 1);
    flex->writeObject(index + 1, &targetCount, 2, 0, 1);
    flex->writeObject(index + 2, targets, static_cast<ui16>(targetSize * sizeof(u16)), 0, 1);
}
