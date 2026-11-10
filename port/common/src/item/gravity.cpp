/* Code10 (No Regret's 1048, No Remorse's 1038, file name unknown; the addresses
 * are No Regret's): what an item stands on, starting a fall and the falling and
 * thrown items' process, the games' differences as each game's values
 * (gravity_game.h). */
#include <cstdlib>
#include "gravity.h"
#include "animprim.h"
#include "blocking.h"
#include "collide.h"
#include "control.h"
#include "itemdata.h"
#include "itemfind.h"
#include "kernel.h"
#include "npc.h"
#include "priority.h"
#include "random.h"
#include "type.h"
#include "sounds.h"

Support::Support(u16 item, u8 close)
{
    const Npc npc(item);

    testSupport(this, itemShape[npc.id], npc.point(), close, item);
}

void testSupport(Support *support, i16 shape, const ItemPoint &point, u8 close, u16 item)
{
    i16 base;
    i16 width;
    i16 depth;
    i16 height;
    i16 otherTop;
    i16 left;
    i16 top;
    i16 right;
    i16 bottom;
    i16 midLeft;
    i16 midRight;
    i16 midTop;
    i16 midBottom;
    ShapeInfo *info;
    u16 mask;
    bool touching = false;
    Item other;
    i16 x = point.x;
    i16 y = point.y;

    info = &typeFlags.infos[shape];
    shapeBox(width, depth, height, info);
    base = point.z;
    if (base <= 0) {
        support->flags = 0x1f;
        return;
    }
    support->flags = 0;
    mask = blockMask(info);
    left = x - width + 1;
    top = y - depth + 1;
    right = x;
    bottom = y;
    midLeft = static_cast<u16>(left + right) / 2;
    midRight = static_cast<u16>(left + right + 1) / 2;
    midTop = static_cast<u16>(top + bottom) / 2;
    midBottom = static_cast<u16>(top + bottom + 1) / 2;
    {
        AreaList list(left, top, right + 0x1e0, bottom + 0x1e0);
        while (list.isValid()) {
            other = list;
            x = itemX[other.id];
            y = itemY[other.id];
            i16 z;
            if (other.id != item && x >= left && y >= top && (z = itemZ[other.id]) <= base) {
                info = &typeFlags.infos[itemShape[other.id]];
                shapeBox(width, depth, height, info);
                otherTop = z + height;
                if (static_cast<i16>(x - width + 1) <= right && static_cast<i16>(y - depth + 1) <= bottom &&
                    (otherTop == base ||
                     ((close != 0U) && otherTop <= base && static_cast<i16>(base - 9) <= otherTop))) {
                    const u16 blockers = blockMask(info) & 0x7fff;
                    /* An empty test the original leaves. */
                    if (item == 0) {
                        /* empty in the original */
                    }
                    if ((mask & blockers) != 0) {
                        if (otherTop == base) {
                            touching = true;
                        }
                        if (static_cast<i16>(x - width + 1) <= midLeft) {
                            if (static_cast<i16>(y - depth + 1) <= midTop) {
                                support->flags |= SUPPORT_NORTHWEST;
                            }
                            if (y >= midBottom) {
                                support->flags |= SUPPORT_SOUTHWEST;
                            }
                        }
                        if (x >= midRight) {
                            if (static_cast<i16>(y - depth + 1) <= midTop) {
                                support->flags |= SUPPORT_NORTHEAST;
                            }
                            if (y >= midBottom) {
                                support->flags |= SUPPORT_SOUTHEAST;
                            }
                        }
                    }
                }
            }
            list.id = foundItems[list.index];
            list.index++;
        }
    }
    if ((((support->flags & SUPPORT_NORTHWEST) != 0) && ((support->flags & SUPPORT_SOUTHEAST) != 0)) ||
        (((support->flags & SUPPORT_SOUTHWEST) != 0) && ((support->flags & SUPPORT_NORTHEAST) != 0))) {
        support->flags |= SUPPORT_STANDING;
    }
    if ((close != 0U) && !touching) {
        support->flags = 0;
    }
}

GravityProcess::GravityProcess(u16 processItem, i16 dx, i16 dy, i16 dz, i16 newGravity, i16 newLifetime,
                               ItemPoint *targetPoint)
{
    nameProcess(pid, "Gravity");
    setItem(processItem);
    setType(GRAVITY_TYPE);
    started = 0;
    speedX = dx;
    speedY = dy;
    speedZ = dz;
    this->gravity = newGravity;
    this->lifetime = newLifetime;
    this->target = targetPoint;
    sliding = 0;
    unknown3d = 0;
    Item(processItem).andFlags(~ITEM_BOUNCING);
    const Npc npc(this->item);
    if (npc.isNpc()) {
        npcs[npc.id].falling = 1;
        const u8 z = npc.z();
        npcs[npc.id].fallStart = z;
    }
}

/* A landing NPC: a dead one's animation goes on; the Silencer falling three
 * steps or more lands (3Bh) and stands or takes the combat stance. */
void GravityProcess::onKill()
{
    Npc npc(item);

    if (npc.z() < 0xfb && npc.isNpc()) {
        npcs[npc.id].falling = 0;
        if (hasFlags(PROCESS_FAILED) == 0U) {
            const u8 start = npcs[npc.id].fallStart;
            const i16 fallen = static_cast<i16>(start - npc.z()) / 8;
            if (npc.isDead() != 0U) {
                Process *anim = findItemProcess(item, FIRST_ITEM_TYPE);
                if (anim != nullptr) {
                    wakeProcess(anim);
                }
            } else {
                const u16 lastAnim = npc.lastAnimSet();
                if (npc.isSilencer() && (npc.isDead() == 0U) && lastAnim != ANIM_QUICK_JUMP &&
                    lastAnim != ANIM_COMBAT_ROLL_LEFT && lastAnim != ANIM_COMBAT_ROLL_RIGHT &&
                    lastAnim != ANIM_KNEEL_ROLL_LEFT && lastAnim != ANIM_KNEEL_ROLL_RIGHT && !npc.isRunning() &&
                    fallen >= 3) {
                    npc.doAnim(ANIM_JUMP_LANDING, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
                    if (npc.isInCombat() != 0U) {
                        npc.doAnim(ANIM_COMBAT_STAND_SMALL, DIRECTION_CURRENT, 1, 0);
                    } else {
                        npc.doAnim(ANIM_STAND, DIRECTION_CURRENT, 1, 0);
                    }
                    npc.restartSound(SOUND_HURT_MALE);
                }
            }
        }
    }
}

/* The build leaves this test empty: it is always 0. */
inline bool unknownTest()
{
    return false;
}

namespace {
/* A step's scale: the ticks the last frame took, 1 to 4. */
i16 frameScale(u32 ticks)
{
    if (ticks >= 4) {
        return 4;
    }
    if (ticks >= 3) {
        return 3;
    }
    if (ticks >= 2) {
        return 2;
    }
    return 1;
}

/* A speed a step nearer 0. */
void stepTowardZero(i16 &speed)
{
    if (speed < 0) {
        speed++;
    } else if (speed > 0) {
        speed--;
    } else {
        speed = 0;
    }
}

/* A speed after a bump: a quarter of it, rounded toward 0. */
i16 damped(i16 speed)
{
    if (speed < 0) {
        speed++;
    }
    speed >>= 2;
    return speed;
}

/* A sliding controlled NPC's speed under 16 one way gets 4 more that way;
 * a still one, by chance, 4 to the low side (the original's other chance
 * adds 4 and takes it back). */
void keepSliding(i16 &speed)
{
    if (abs(speed) >= 0x10) {
        return;
    }
    if (speed > 0) {
        speed += 4;
    } else if (speed < 0) {
        speed -= 4;
    } else if (randomBelow(2) == 0U) {
        speed -= 4;
    }
}

/* No Regret's: a fixed thing, or one without weight but a crate, does not
 * fall. */
bool fixedOrWeightless(u16 item)
{
    const Item thing(item);
    if (!thing.isValid()) {
        return false;
    }
    const ShapeInfo *info = &typeFlags.infos[itemShape[thing.id]];
    return info->fixed || ((info->weight == 0U) && !thing.isCrate());
}
}

/* A blocked low-friction thing turns by chance: in 20, 4 times its speed
 * across and 4 times down a step nearer 0, once each a step more either
 * way; the blocked sides count no more unless it bounced. Returns the
 * blocked sides. */
u16 GravityProcess::turnByChance(const Npc &moving, u16 blocked)
{
    const i16 turn = randomBelow(20);
    if (turn < 4) {
        stepTowardZero(speedX);
    } else if (turn < 8) {
        stepTowardZero(speedY);
    } else if (turn == 8) {
        speedX++;
    } else if (turn == 9) {
        speedX--;
    } else if (turn == 10) {
        speedY++;
    } else if (turn == 11) {
        speedY--;
    }
    if ((itemFlags[moving.id] & ITEM_BOUNCING) == 0) {
        blocked &= ~3;
    }
    return blocked;
}

/* The thing landed on the floor: the controlled NPC is pushed down the
 * slope it stands on (Support::only) and stands when its support holds.
 * Bouncing or sliding, it bounces back a quarter as high (a standing NPC
 * that landed on top of something stops), the controlled NPC jolting by
 * chance when it slides; else a controlled NPC slides on (keepSliding).
 * Returns whether the fall ended. */
bool GravityProcess::landOnFloor(const Npc &moving, bool onTop, bool controlled, u8 &landed)
{
    speedZ += gravity;
    landed = 0;
    if (controlled && speedZ <= 0) {
        /* (made twice, as the original) */
        Support support(item, 0);
        support = Support(item, 0);
        if (support.only(9, 6)) {
            speedY += 4;
        }
        if (support.only(3, 0xc)) {
            speedX -= 4;
        }
        if (support.only(6, 9)) {
            speedY -= 4;
        }
        if (support.only(0xc, 3)) {
            speedX += 4;
        }
        landed = support.standing();
    }
    if ((itemFlags[moving.id] & (ITEM_BOUNCING | ITEM_LOW_FRICTION)) == 0) {
        terminate(0);
        return true;
    }
    if (landed == 0U) {
        if (!controlled) {
            terminate(0);
            return true;
        }
        keepSliding(speedX);
        keepSliding(speedY);
        speedZ = 0;
        sliding = 1;
        return false;
    }
    if (onTop && moving.isNpc()) {
        terminate(0);
        return true;
    }
    if (speedZ < 0) {
        speedZ = -speedZ;
        speedZ >>= 2;
    } else {
        speedZ >>= 2;
        speedZ = -speedZ;
    }
    sliding = 0;
    if ((itemFlags[moving.id] & ITEM_LOW_FRICTION) == 0) {
        if (speedZ < gravity) {
            terminate(0);
            return true;
        }
        return false;
    }
    if (!controlled) {
        terminate(0);
        return true;
    }
    if (speedZ < gravity) {
        speedZ = 0;
    } else {
        speedX += static_cast<i16>(randomNumber(speedZ));
        speedX -= static_cast<i16>(randomNumber(speedZ));
        speedY += static_cast<i16>(randomNumber(speedZ));
        speedY -= static_cast<i16>(randomNumber(speedZ));
    }
    if (abs(speedX) < 4 && abs(speedY) < 4 && speedZ < gravity) {
        terminate(0);
        return true;
    }
    return false;
}

/* The thing was blocked (blocked: 1 across, 2 down the map, 4 up and
 * down): a thrown one stops; one not sliding slows down (a low-friction one
 * turns by chance first), its fall slows unless it landed, and it bounces
 * off the sides (an NPC stops against them); landing goes on in
 * landOnFloor. A standing thing that cannot fall stops. Returns whether
 * the fall ended. */
bool GravityProcess::bumped(const Npc &moving, u16 blocked, bool onTop, bool controlled, u8 &landed)
{
    if (target != nullptr) {
        terminate(0);
        return true;
    }
    if (sliding == 0U) {
        if ((itemFlags[moving.id] & ITEM_LOW_FRICTION) != 0) {
            blocked = turnByChance(moving, blocked);
        }
        if (((itemFlags[moving.id] & ITEM_LOW_FRICTION) == 0) || ((blocked & 3) != 0)) {
            speedX = damped(speedX);
            speedY = damped(speedY);
        }
    }
    if ((blocked & 4) == 0) {
        speedZ = damped(speedZ);
    }
    if ((blocked & 1) != 0) {
        speedX = ((sliding == 0U) && moving.isNpc()) ? 0 : static_cast<i16>(-speedX);
    }
    if ((blocked & 2) != 0) {
        speedY = ((sliding == 0U) && moving.isNpc()) ? 0 : static_cast<i16>(-speedY);
    }
    if (landed != 0U) {
        sliding = 0;
    }
    if (((blocked & 4) != 0) && landOnFloor(moving, onTop, controlled, landed)) {
        return true;
    }
    if ((landed != 0U) && cannotFall()) {
        terminate(0);
        return true;
    }
    return false;
}

/* A step of the fall or the throw, scaled by the frame's ticks: a thing out
 * of the world or in a container stops; one off the fast area waits. A
 * thing at rest lets what lies on it fall, a thing in motion takes what is
 * on it along; it moves by its speed (gravity pulls it down), and a block
 * goes to bumped. The lifetime counts down. A thrown thing (to target)
 * goes on at once until it lands near the target. */
void GravityProcess::run()
{
    Sweep sweep;
    const Npc moving(item);
    u8 landed = 0;
    const bool controlled = moving.isControlled();
    const u32 ticks = frameTicks;
    if (FALL_CHECKED_FIRST && started == 0U) {
        started = 1;
        if (fixedOrWeightless(item)) {
            terminate(0);
            return;
        }
    }
    const i16 scale = frameScale(ticks);
    if (moving.z() >= 0xfb) {
        killItemProcesses(item, ANY_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
        return;
    }
    if ((itemFlags[moving.id] & ITEM_CONTAINED) != 0) {
        terminate(0);
        return;
    }
    do {
        if (target != nullptr) {
            const i16 distance =
                abs(itemZ[moving.id] - target->z) + (abs(static_cast<i16>(itemX[moving.id] - target->x)) +
                                                     abs(static_cast<i16>(itemY[moving.id] - target->y)));
            if (distance < 50) {
                terminate(0);
                return;
            }
        }
        if (moving.inFastArea() == 0U) {
            if (target != nullptr) {
                terminate(0);
            }
            return;
        }
        if (speedZ == 0 || ((itemFlags[moving.id] & (ITEM_BOUNCING | ITEM_LOW_FRICTION)) != 0)) {
            if ((speedX != 0) || (speedY != 0)) {
                moving.grab();
            } else {
                for (SurfaceSearch above(moving.id, "$", 1, 1); above.isValid(); above.next()) {
                    fall(&above);
                }
            }
        }
        moving.andFlags(~ITEM_BOUNCING);
        const i16 x = itemX[moving.id] + (speedX * scale);
        const i16 y = itemY[moving.id] + (speedY * scale);
        const i16 z = itemZ[moving.id] + (speedZ * scale);
        speedZ -= gravity * scale;
        u16 blocked = 0;
        if (unknownTest()) {
            return;
        }
        bool onTop = false;
        if (!sweep.moveItem(item, x, y, z, MOVE_NO_FLOOR, 0)) {
            blocked = sweep.blockedAxes;
            if (blocked != 0U) {
                moving.orFlags(ITEM_BOUNCING);
            }
            if ((blocked & 4) != 0) {
                if ((itemFlags[moving.id] & ITEM_LOW_FRICTION) != 0) {
                    sweep.moveItem(item, x, y, sweep.end.z, MOVE_NO_FLOOR, 0);
                    blocked = sweep.blockedAxes | 4;
                }
                onTop = true;
            }
        }
        if (target != nullptr) {
            moving.moveTo(sweep.end);
        } else {
            sweep.apply();
            if (hasFlags(PROCESS_DEAD) != 0U) {
                return;
            }
        }
        if (sweep.outside != 0U) {
            speedX = speedY = 0;
        }
        if ((blocked != 0U) && bumped(moving, blocked, onTop, controlled, landed)) {
            return;
        }
        if ((lifetime != 0) && (--lifetime == 0)) {
            terminate(0);
            return;
        }
    } while (target != nullptr);
    setTimeout(static_cast<i16>(scale * 4));
}

bool GravityProcess::cannotFall()
{
    Sweep sweep;
    const Npc npc(item);

    return static_cast<i16>(speedZ * gravity) <= 0 &&
           (!sweep.move(item, &lvalue(ItemPoint(itemX[npc.id], itemY[npc.id], itemZ[npc.id] + speedZ)), 4));
}

u16 hurl(Item *item, i16 dx, i16 dy, i16 dz, i16 gravity)
{
    GravityProcess *process = static_cast<GravityProcess *>(firstProcess(item->id, GRAVITY_TYPE));

    if (process != nullptr) {
        process->accelerate(dx, dy, dz);
    } else {
        process = new GravityProcess(item->id, dx, dy, dz, gravity, 200, nullptr);
        if (!CONTROLLED_FALL_AT_ONCE || item->id != controlledActor) {
            process->setTimeout(20);
        }
    }
    return process->pid;
}

void fall(Item *item)
{
    ShapeInfo *info = &typeFlags.infos[item->shape()];

    /* A crate falls even when it weighs nothing (No Remorse's crates all
     * weigh 200; its own looked at the weight alone). */
    if (!info->fixed && ((info->weight != 0U) || item->isCrate())) {
        hurl(item, 0, 0, 0, 2);
    }
}

void landNow(Process *process)
{
    const Item item(process->item);
    Sweep sweep;

    sweep.moveItem(item.id, itemX[item.id], itemY[item.id], 0, MOVE_NO_FLOOR, 0);
    sweep.apply();
    if (process->hasFlags(PROCESS_DEAD) != 0U) {
        return;
    }
    process->terminate(0);
}
