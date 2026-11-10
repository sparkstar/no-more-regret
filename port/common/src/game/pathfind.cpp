/* Code38 (No Regret's 1128, No Remorse's 1110; file name unknown; No Regret's,
 * without No Remorse's debugging breaks, which nothing set): the path finder.
 * Each step it tries a walk from where the NPC stands, as a test animation that
 * runs at once and reports whether it is blocked; when the way to the target is
 * blocked it goes around, turning to one side. */
#include <cstdlib>
#include "pathfind.h"
#include "animprim.h"
#include "attack.h"
#include "crusader.h"
#include "distance.h"
#include "explode.h"
#include "itemdata.h"
#include "kernel.h"
#include "npc.h"
#include "type.h"

namespace {
/* The turns from the direction to the target, tried in order: the first
 * table when randomSide is set. */
char plusFirst[8] = {0, 2, -2, 4, -4, 6, -6, -8};
char minusFirst[8] = {0, -2, 2, -4, 4, -6, 6, -8};
}

PathFinder::PathFinder(u16 npc, u16 goalItem, ItemPoint point, u16 newMaxSteps, u8 newProbe, u16 newStopDistance,
                       u8 newTurnAtEnd)
{
    nameProcess(pid, "PathFinder");
    Npc self(npc);
    const Item goal(goalItem);
    ShapeInfo info;
    if (self.unknown77Bit5() != 0U) {
        killItemProcesses(npc, PATH_FINDER_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    }
    self.setUnknown77Bit5();
    setItem(npc);
    setType(PATH_FINDER_TYPE);
    target.x = point.x;
    target.y = point.y;
    target.z = point.z;
    this->probe = newProbe;
    heading = 0x10;
    lastHeading = 0x10;
    detour = 0;
    this->maxSteps = newMaxSteps;
    step = 0;
    targetItem = goalItem;
    arrived = 0;
    solid = 0;
    this->stopDistance = newStopDistance;
    this->turnAtEnd = newTurnAtEnd;
    standAtEnd = 1;
    randomSide = borlandRandom(2);
    distance = mapDistance(itemX[self.id], itemY[self.id], target.x, target.y);
    if (goal.isValid()) {
        info = typeFlags.info(goal.shape());
        if ((info.z != 0U) && info.solid) {
            solid = 1;
        }
    }
    holsterWeapon();
}

PathFinder::PathFinder(u16 npc, u16 goalItem, u16 newMaxSteps, u8 newProbe, u16 newStopDistance, u8 newTurnAtEnd)
{
    nameProcess(pid, "PathFinder");
    Npc self(npc);
    const Item goal(goalItem);
    ShapeInfo info;
    if (self.unknown77Bit5() != 0U) {
        killItemProcesses(npc, PATH_FINDER_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    }
    self.setUnknown77Bit5();
    setItem(npc);
    setType(PATH_FINDER_TYPE);
    target = goal.point();
    this->probe = newProbe;
    heading = 0x10;
    lastHeading = 0x10;
    detour = 0;
    this->maxSteps = newMaxSteps;
    step = 0;
    targetItem = goalItem;
    arrived = 0;
    solid = 0;
    this->stopDistance = newStopDistance;
    this->turnAtEnd = newTurnAtEnd;
    standAtEnd = 1;
    randomSide = borlandRandom(2);
    distance = mapDistance(itemX[self.id], itemY[self.id], target.x, target.y);
    if (goal.isValid()) {
        info = typeFlags.info(goal.shape());
        if ((info.z != 0U) && info.solid) {
            solid = 1;
        }
    }
    holsterWeapon();
}

void PathFinder::holsterWeapon()
{
    Npc self(item);
    if ((self.isInCombat() != 0U) && (self.isWeaponDrawn() != 0U)) {
        self.doAnim(ANIM_UNREADY_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
    }
}

/* A probe runs every step at once and ends with 1 when it gets there. */
void PathFinder::run()
{
    ItemPoint here;
    Npc self(item);
    AnimPrimitive *walk;
    char facing;
    const Item goal(targetItem);
    if (self.inFastArea() == 0U) {
        return;
    }
    if (arrived != 0U) {
        terminate(1);
        return;
    }
    if (goal.isValid() && (solid != 0U)) {
        target = goal.point();
    }
    here.set(itemX[self.id], itemY[self.id], self.z());
    do {
        if (target.x == here.x && target.y == here.y) {
            if (target.z == here.z) {
                terminate(1);
                return;
            }
            terminate(0);
            return;
        }
        const u8 last = heading;
        heading = nextDirection(&here);
        lastHeading = last;
        if (heading == 0x10) {
            terminate(0);
            return;
        }
        if (heading == 0xff) {
            if (probe != 0U) {
                terminate(1);
                return;
            }
            arrived = 1;
        } else if (step == maxSteps) {
            terminate(0);
            return;
        }
        if (probe == 0U) {
            if (arrived != 0U) {
                facing = tryHeading;
            } else {
                facing = heading;
            }
            turnNpc(item, facing, (arrived != 0U) ? 1 : 0, 1);
            if (self.isInCombat() != 0U) {
                walk = new AdvanceSmallAnim(item, facing);
            } else {
                walk = new WalkAnim(item, facing);
            }
            walk->block(this);
        }
        step++;
        /* A disabled test, as in nextDirection. */
        /* NOLINTNEXTLINE(readability-simplify-boolean-expr): the original's disabled test */
        if ((false) && (probe != 0U)) {
            /* empty in the original */
        }
    } while (probe != 0U);
}

void PathFinder::finish()
{
    Npc self(item);
    self.clearUnknown77Bit5();
    if ((probe == 0U) && (killingAll == 0U) && (hasFlags(PROCESS_FAILED) == 0U) && !self.inVoid() &&
        (self.isDead() == 0U)) {
        u8 turned = 0;
        if (turnAtEnd != 0U) {
            if (targetItem != 0U) {
                turned = turnNpc(item, self.directionToItem(targetItem), 1, 1);
            } else {
                turned = turnNpc(item, self.directionTo(target.x, target.y), 1, 1);
            }
        }
        if ((turned == 0U) && (standAtEnd != 0U)) {
            if ((self.isInCombat() != 0U) && (self.isWeaponDrawn() != 0U)) {
                self.doAnim(ANIM_COMBAT_STAND_SMALL, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
                return;
            }
            self.doAnim(ANIM_STAND, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
        }
    }
}

void PathFinder::terminate(u32 endResult)
{
    finish();
    Process::terminate(endResult);
}

void PathFinder::fail(u32 failResult)
{
    finish();
    Process::fail(failResult);
}

/* Returns 10h when every step is blocked and FFh when it has arrived: the
 * test step touched the solid target or ended within stopDistance, or the NPC
 * stopped to shoot at its attack target. */
u8 PathFinder::nextDirection(ItemPoint *from)
{
    i16 i;
    i16 dist;
    i16 start;
    Npc self(item);
    u8 toward;
    ItemPoint position;
    u8 around[8];
    u8 turns[8];
    /* Set by the loop below, which runs from start (0 or 2). */
    i16 half = 0;
    AnimPrimitive *walk;
    AnimStatus animStatus;
    toward = direction(from->x, from->y, target.x, target.y);
    dist = mapDistance(from->x, from->y, target.x, target.y);
    if (distance > dist) {
        distance = dist;
        position = *from;
        if (self.isInCombat() != 0U) {
            walk = new AdvanceSmallAnim(item, toward, &position, targetItem);
        } else {
            walk = new WalkAnim(item, toward, &position, targetItem);
        }
        walk->listenTo(pid);
        walk->run();
        if (hasFlags(PROCESS_CHILD_FAILED) == 0U) {
            detour = 0;
        }
    }
    start = 0;
    if (detour != 0U) {
        if (randomSide != 0U) {
            around[0] = (heading + ((detourSide != 0U) ? 2 : -2) + 8) & 0xf;
        } else {
            around[0] = (heading + ((detourSide != 0U) ? -2 : 2) + 8) & 0xf;
        }
        for (i = 2; i < 16; i += 2) {
            half = i >> 1;
            if (randomSide != 0U) {
                around[half] = (around[static_cast<i16>(half - 1)] + ((detourSide != 0U) ? 2 : -2)) & 0xf;
            } else {
                around[half] = (around[static_cast<i16>(half - 1)] + ((detourSide != 0U) ? -2 : 2)) & 0xf;
            }
        }
        start = 2;
    } else {
        for (i = 0; i < 8; i++) {
            if (randomSide != 0U) {
                turns[i] = (toward + plusFirst[i]) & 0xf;
            } else {
                turns[i] = (toward + minusFirst[i]) & 0xf;
            }
        }
    }
    for (i = start; i < 16; i += 2) {
        half = i >> 1;
        if (detour != 0U) {
            if (half == 2) {
                continue;
            }
            if (i == 14) {
                break;
            }
            tryHeading = around[half];
        } else {
            tryHeading = turns[half];
        }
        position = *from;
        if (self.isInCombat() != 0U) {
            walk = new AdvanceSmallAnim(item, tryHeading, &position, targetItem);
        } else {
            walk = new WalkAnim(item, tryHeading, &position, targetItem);
        }
        walk->listenTo(pid);
        walk->run();
        animStatus = result;
        if ((solid != 0U) && animStatus.bits.touched) {
            turnAtEnd = 1;
            return 0xff;
        }
        if ((stopDistance != 0) && mapDistance(target.x, target.y, position.x, position.y) <= stopDistance) {
            turnAtEnd = 1;
            return 0xff;
        }
        if (hasFlags(PROCESS_CHILD_FAILED) == 0U) {
            break;
        }
    }
    if (hasFlags(PROCESS_CHILD_FAILED) != 0U) {
        return 0x10;
    }
    if (tryHeading != toward) {
        /* A disabled test: only its label is left, where the original loads
         * this again. */
        /* NOLINTNEXTLINE(readability-simplify-boolean-expr): the original's disabled test */
        if ((false) && (detour != 0U)) {
            ;
        } else if (detour == 0U) {
            detour = 1;
            if ((half & 1) != 0) {
                detourSide = 1;
            } else {
                detourSide = 0;
            }
        }
    }
    from->x = position.x;
    from->y = position.y;
    from->z = position.z;
    if ((self.isInCombat() != 0U) && !self.isControlled()) {
        Attack *attack = static_cast<Attack *>(firstProcess(item, ATTACK_TYPE));
        const Item enemy(npcs[self.id].attackTarget);
        if (enemy.isValid() && self.isOnScreen() && (fireDistance(&self, enemy.id, toward, 0, 0, 0) != 0U)) {
            self.doAnim(ANIM_STAND, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
            attack->setStoppedToFire();
            standAtEnd = 0;
            turnAtEnd = 1;
            return 0xff;
        }
    }
    return tryHeading;
}
