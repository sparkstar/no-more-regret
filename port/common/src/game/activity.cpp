/* Code36 (No Regret's 1118, No Remorse's 1100, file name unknown; the
 * addresses are No Regret's): the processes of the NPC activities that
 * Npc::setActivity starts: standing, pacing, guarding, loitering, the
 * surrender and No Regret's rolling thunder, the games' differences as each
 * game's values (activity_game.h). */
#include <cstdlib>
#include "activity.h"
#include "animcach.h"
#include "animprim.h"
#include "anchor.h"
#include "collide.h"
#include "control.h"
#include "crusader.h"
#include "distance.h"
#include "explode.h"
#include "itemdata.h"
#include "itemfind.h"
#include "kernel.h"
#include "npc.h"
#include "pathfind.h"
#include "random.h"
#include "scrit.h"
#include "systimer.h"
#include "utility.h"
#include "shapes.h"

/* No Regret's rolling thunder's rolls (RollingThunder::run). */
bool canRoll(u16 npc, u16 anim, char *direction);
u16 rollDistance(u16 npc, u16 target, char direction, i16 dx, i16 dy, i16 dz, i16 x, i16 y, i16 z);

namespace {
/* What No Remorse's surrendering NPC says, by its shape (Surrender::run). */
i16 surrenderSoundsOfficeWorker[6] = {0xe9, 0xe0, 0xeb, 0xe2, 0xe1, 0xea};
i16 surrenderSoundsChemsuitWorker[4] = {0xb4, 0xc5, 0xc6, 0xe8};
i16 surrenderSoundsScientist[4] = {0xe3, 0xe4, 0xec, 0xf6};
i16 surrenderSoundsMaintenanceMan[4] = {0xde, 0xdf, 0x8a, 0x8b};
i16 surrenderSoundsSeatedLady[3] = {0xd6, 0xff, 0xd7};
}

bool seesControlledNpc(u16 npc, u8 mode)
{
    char ahead;
    char left;
    char farLeft;
    char right;
    char farRight;
    Npc self(npc);
    const Npc target(controlledActor);
    u8 facing = self.direction();
    bool aside;
    const i16 anim = self.lastAnimSet();
    if ((controlledActor == 0U) || !self.isMostlyOnScreen()) {
        return false;
    }
    if ((mode != 0U) && (anim == ANIM_LOOK_LEFT || anim == ANIM_LOOK_RIGHT) && npcs[self.id].animFrame >= 2) {
        if (anim == ANIM_LOOK_LEFT) {
            aside = false;
            if (facing != DIRECTION_SOUTH && facing != DIRECTION_SOUTHWEST && facing != DIRECTION_WEST) {
                aside = true;
            }
        } else {
            aside = true;
            if (facing != DIRECTION_SOUTH && facing != DIRECTION_SOUTHWEST && facing != DIRECTION_WEST) {
                aside = false;
            }
        }
        if (aside) {
            facing = (facing - 4) & 0xf;
        } else {
            facing = (facing + 4) & 0xf;
        }
    }
    ahead = direction(self.x(), self.y(), target.x(), target.y());
    farLeft = (ahead - 2) & 0xf;
    left = (ahead - 1) & 0xf;
    right = (ahead + 1) & 0xf;
    farRight = (ahead + 2) & 0xf;
    if (facing == ahead || facing == farLeft || facing == left || facing == right || facing == farRight) {
        if (fireDistance(&self, controlledActor, ahead, 0, 0, 0) != 0U) {
            return true;
        }
        if ((firingFrameFound == 0U) && (rangeIfVisible(&self, controlledActor) != 0U)) {
            return true;
        }
    }
    return false;
}

Activity::Activity(u16 ownerNpc)
{
    setType(ACTIVITY_TYPE);
    setItem(ownerNpc);
    this->npc = ownerNpc;
    if (IDLE_TIMING) {
        if (oneIn(3)) {
            lastTime = 0;
        } else {
            lastTime = systemTimer->ticks;
        }
        interval = borlandRandom(15) + 10;
    }
    const Npc self(ownerNpc);
    if (hasAnim(self.shape(), ANIM_STAND)) {
        setFirstFrame(self.id, ANIM_STAND);
        if (STANDING_PUTS_WEAPON_AWAY) {
            npcs[self.id].weaponDrawn = 0;
        }
    }
    eventMask |= EVENT_RELATED_DIED | EVENT_TIMEOUT;
}

bool Activity::checkAlert() const
{
    Npc self(npc);
    if (self.isOnScreen()) {
        const u16 activity = self.defaultActivity1();
        if (activity != 0U) {
            if (!self.isActivity(ACTIVITY_DEFAULT1)) {
                if (seesControlledNpc(npc, 0)) {
                    self.setActivity(ACTIVITY_DEFAULT1);
                    return true;
                }
            }
        }
    }
    return false;
}

bool Activity::intervalPassed()
{
    const u32 now = systemTimer->ticks;
    if (lastTime != 0U) {
        if (lastTime + static_cast<i16>(interval * 60) <= now) {
            lastTime = now;
            return true;
        }
        return false;
    }
    lastTime = now;
    return true;
}

Stand::Stand(u16 ownerNpc) : Activity(ownerNpc)
{
    nameProcess(pid, "Stand");
    setType(STAND_TYPE);
}

void Stand::run()
{
    if (!checkAlert()) {
        setTimeout(60);
    }
}

Surrender::Surrender(u16 ownerNpc) : Activity(ownerNpc)
{
    nameProcess(pid, "Surrendering");
    setType(SURRENDER_TYPE);
    soundPlayed = 0;
    unknown48 = 0;
    Npc self(this->npc);
    self.doAnim(ANIM_SURRENDER, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
    npcs[self.id].activityLocked = 1;
}

void Surrender::run()
{
    Npc self(item);
    u8 direction;
    if (!self.isBusy()) {
        if (controlledActor != 0U) {
            if (self.isMostlyOnScreen()) {
                direction = self.directionToItem(controlledActor);
                if (self.direction() == direction) {
                    if (SURRENDER_SPEAKS_ONCE) {
                        /* It speaks once, when it sees the controlled NPC. */
                        if ((soundPlayed == 0U) && (rangeIfVisible(&self, controlledActor) != 0U)) {
                            i16 sound = -1;
                            soundPlayed = 1;
                            switch (self.shape()) {
                            case SHAPE_OFFICE_WORKER:
                                sound = surrenderSoundsOfficeWorker[borlandRandom(6)];
                                break;
                            case SHAPE_CHEMSUIT_WORKER:
                                sound = surrenderSoundsChemsuitWorker[borlandRandom(4)];
                                break;
                            case SHAPE_SCIENTIST:
                                sound = surrenderSoundsScientist[borlandRandom(4)];
                                break;
                            case SHAPE_MAINTENANCE_MAN:
                                sound = surrenderSoundsMaintenanceMan[borlandRandom(4)];
                                break;
                            case SHAPE_SEATED_LADY:
                                sound = surrenderSoundsSeatedLady[borlandRandom(3)];
                            }
                            if (sound != -1) {
                                self.playSound(sound);
                            }
                        }
                    } else if (intervalPassed() && (rangeIfVisible(&self, controlledActor) != 0U)) {
                        self.playHurtSound();
                    }
                } else {
                    self.turnToward(direction, 1);
                }
            }
        }
    }
    setTimeout(60);
}

void Surrender::onKill()
{
    const Npc self(item);
    npcs[self.id].activityLocked = 0;
}

Pace::Pace(u16 ownerNpc) : Activity(ownerNpc)
{
    nameProcess(pid, "Pace");
    setType(PACE_TYPE);
    failures = 0;
}

void Pace::run()
{
    Npc self(npc);
    Process *turn;
    AnimPrimitive *walk;
    u16 walkPid;
    u16 turnPid;
    ItemPoint point = self.point();
    if (checkAlert()) {
        return;
    }
    if (self.isBusy()) {
        return;
    }
    walk = new WalkAnim(item, self.direction(), &point, 0);
    walk->listenTo(pid);
    walk->run();
    if (hasFlags(PROCESS_CHILD_FAILED) != 0U) {
        failures++;
        if (failures >= 2) {
            const u16 shape = self.shape();
            if (shape == SHAPE_MAINTENANCE_MAN || shape == SHAPE_OFFICE_WORKER || shape == SHAPE_CHEMSUIT_WORKER ||
                shape == SHAPE_SCIENTIST || shape == SHAPE_SEATED_LADY) {
                self.setActivity(ACTIVITY_SURRENDER);
                return;
            }
            self.setActivity(ACTIVITY_ATTACK);
            return;
        }
        self.doAnim(ANIM_STAND, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
        turnPid = turnNpc(self.id, (self.direction() + 8) & 0xf, 0, 0);
        if (turnPid != 0U) {
            turn = findProcess(turnPid);
            turn->setTimeout(60);
            turn->block(this);
        }
    } else {
        failures = 0;
        walkPid = self.doAnim(ANIM_WALK, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
        if (walkPid != 0U) {
            findProcess(walkPid)->block(this);
        }
    }
}

namespace {
/* npc plays an idle animation (1118:08c1). */
void fidget(u16 npc, Activity * /*unused*/)
{
    Npc self(npc);
    if (self.isBusy()) {
        return;
    }
    self.doAnim(oneIn(2) ? 0x1e : 0x1f, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
    self.doAnim(ANIM_STAND, DIRECTION_CURRENT, 1, 0);
}
}

Guard::Guard(u16 ownerNpc) : Activity(ownerNpc)
{
    nameProcess(pid, "Guard");
    setItem(ownerNpc);
    setType(GUARD_TYPE);
    eventMask |= EVENT_RELATED_DIED;
}

void Guard::run()
{
    bool canFidget;
    Npc self(item);
    if (self.isBusy()) {
        setTimeout(60);
        return;
    }
    if (seesControlledNpc(item, 0)) {
        self.setActivity(ACTIVITY_ATTACK);
        return;
    }
    if (oneIn(2)) {
        setTimeout(static_cast<i16>((borlandRandom(3) * 60) + 60));
        return;
    }
    canFidget = hasAnim(self.shape(), ANIM_LOOK_LEFT);
    if (canFidget) {
        fidget(item, this);
    }
}

Loiter::Loiter(u16 ownerNpc, i16 loiterCount) : Activity(ownerNpc)
{
    nameProcess(pid, "Loiter");
    setItem(ownerNpc);
    setType(LOITER_TYPE);
    setRate(1);
    startTimer(1);
    eventMask |= EVENT_RELATED_DIED | EVENT_TIMER;
    this->count = loiterCount;
    const Npc self(this->npc);
    homeX = self.x();
    homeY = self.y();
    homeZ = self.z();
}

void Loiter::run()
{
    Npc self(item);
    ItemPoint target;
    PathFinder *finder;
    bool canFidget;
    if (self.isBusy()) {
        return;
    }
    if (count == -1 || ((count--) != 0)) {
        canFidget = hasAnim(self.shape(), ANIM_LOOK_LEFT);
        if (oneIn(2) && canFidget) {
            fidget(item, this);
            return;
        }
        target.x = homeX + randomBetween(-160, 160);
        target.y = homeY + randomBetween(-160, 160);
        target.z = homeZ;
        finder = new PathFinder(item, 0, target, 12, 0, 0x40, 0);
        finder->block(this);
        finder->listenTo(pid);
    } else {
        terminate(0);
    }
}

/* A timer event checks for the controlled NPC; when the walk it listens to
 * ends, the NPC waves or stretches twice, or waits 2-4 seconds. */
void Loiter::event(u32 event, EventData /*data*/)
{
    Npc self(item);
    u16 first;
    u16 second = 0;
    bool canWave;
    bool canStretch;
    const u16 shape = self.shape();
    if (!self.inVoid() && (self.isDead() == 0U)) {
        if (killingAll != 0U) {
            return;
        }
        switch (event) {
        case EVENT_TIMER:
            if (checkAlert()) {
                return;
            }
            break;
        case EVENT_RELATED_DIED:
            canWave = hasAnim(shape, ANIM_KNEELING_RETREAT);
            canStretch = hasAnim(shape, ANIM_KNEELING_ADVANCE);
            if (oneIn(8) || (!canWave && !canStretch)) {
                setTimeout(static_cast<i16>(randomBetween(2, 4) * 60));
                return;
            }
            if (canWave || canStretch) {
                if ((canWave && oneIn(6)) || !canStretch) {
                    first = self.doAnim(ANIM_KNEELING_RETREAT, DIRECTION_CURRENT, 5, 0);
                    if (first != 0U) {
                        second = self.doAnim(ANIM_KNEELING_RETREAT, DIRECTION_CURRENT, 5, 0);
                    }
                } else {
                    first = self.doAnim(ANIM_KNEELING_ADVANCE, DIRECTION_CURRENT, 5, 0);
                    if (first != 0U) {
                        second = self.doAnim(ANIM_KNEELING_ADVANCE, DIRECTION_CURRENT, 5, 0);
                    }
                }
                if (first != 0U) {
                    findProcess(first)->block(this);
                    if (second != 0U) {
                        findProcess(second)->block(this);
                    }
                }
            }
        }
    }
}

RollingThunder::RollingThunder(u16 ownerNpc) : Activity(ownerNpc)
{
    nameProcess(pid, "RollingThunder");
    setItem(ownerNpc);
    setType(ROLLING_THUNDER_TYPE);
    eventMask |= EVENT_RELATED_DIED;
    lastSeek = 0;
}

bool RollingThunder::seekDue()
{
    const u32 now = systemTimer->ticks;
    if (lastSeek + 90 <= now) {
        lastSeek = now;
        return true;
    }
    return false;
}

/* The second roll blocks on the last shot's animation, not on the roll back:
 * pid keeps the shot's number. */
void RollingThunder::run()
{
    Npc self(item);
    i16 anim;
    Npc target(npcs[self.id].attackTarget);
    char direction;
    char facing;
    /* Set by the shots, of which there is at least one. */
    u16 startedPid = 0;
    AnimPrimitive *shot;
    Process *waited;
    i16 i;
    i16 shots;
    bool onScreen;
    u8 hit;
    u8 left;
    if (self.isBusy()) {
        setTimeout(60);
        return;
    }
    if (target.id != controlledActor &&
        (!target.isValid() || target.inVoid() || itemShape[target.id] != SHAPE_CRAWLING_SPIDER)) {
        if (controlledActor == 0U) {
            self.setAttackTarget(1);
        } else {
            self.setAttackTarget(controlledActor);
        }
    }
    target = Npc(npcs[self.id].attackTarget);
    if ((itemShape[target.id] != SHAPE_CRAWLING_SPIDER && (target.isDead() != 0U)) || !target.isValid() ||
        target.z() == ETHEREAL_Z) {
        if (target.id != controlledActor) {
            self.setAttackTarget(controlledActor);
        } else {
            setTimeout(60);
            return;
        }
        if ((target.isDead() != 0U) || !target.isValid() || target.z() == ETHEREAL_Z) {
            setTimeout(60);
            return;
        }
    }
    onScreen = self.isMostlyOnScreen();
    if (onScreen) {
        left = borlandRandom(2);
        if (left != 0U) {
            anim = ANIM_COMBAT_ROLL_LEFT;
        } else {
            anim = ANIM_COMBAT_ROLL_RIGHT;
        }
        facing = self.direction();
        if (canRoll(item, anim, &direction)) {
            self.doAnim(anim, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
            if (direction != facing) {
                turnNpc(item, direction, 1, 0);
            }
            shots = borlandRandom(3) + 1;
            for (i = 0; i < shots; i++) {
                startedPid = self.doAnim(ANIM_FIRE_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
                shot = static_cast<AnimPrimitive *>(findProcess(startedPid));
                shot->setTarget(controlledActor);
            }
            startedPid = self.doAnim(anim == ANIM_COMBAT_ROLL_LEFT ? 0x3e : 0x3d, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
            waited = findProcess(startedPid);
            waited->block(this);
        } else {
            if (left != 0U) {
                anim = ANIM_COMBAT_ROLL_RIGHT;
            } else {
                anim = ANIM_COMBAT_ROLL_LEFT;
            }
            if (canRoll(item, anim, &direction)) {
                self.doAnim(anim, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
                if (direction != facing) {
                    turnNpc(item, direction, 1, 0);
                }
                shots = borlandRandom(3) + 1;
                for (i = 0; i < shots; i++) {
                    startedPid = self.doAnim(ANIM_FIRE_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
                    shot = static_cast<AnimPrimitive *>(findProcess(startedPid));
                    shot->setTarget(controlledActor);
                }
                self.doAnim(anim == ANIM_COMBAT_ROLL_LEFT ? 0x3e : 0x3d, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
                waited = findProcess(startedPid);
                waited->block(this);
            } else {
                char toward;
                const char heading = self.direction();
                toward =
                    closestDirection(itemX[self.id], itemY[self.id], itemX[target.id], itemY[target.id], 16, 0, 15);
                if (toward == heading) {
                    if ((hit = static_cast<u8>(self.recentlyHit())) == 0U) {
                        hit = fireDistance(&self, npcs[self.id].attackTarget, toward, 0, 0, 0);
                    }
                    if (hit != 0U) {
                        self.doAnim(ANIM_FIRE_SMALL, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
                        return;
                    }
                    if (seek()) {
                        return;
                    }
                } else {
                    startedPid = turnNpc(item, toward, 1, static_cast<i16>(self.isWeaponDrawn() == 0U));
                    if (startedPid != 0U) {
                        waited = findProcess(startedPid);
                        waited->block(this);
                        return;
                    }
                }
            }
        }
    }
    setTimeout(60);
}

bool RollingThunder::seek()
{
    Npc self(item);
    const Npc target(npcs[self.id].attackTarget);
    if (itemShape[target.id] != SHAPE_CRAWLING_SPIDER && seekDue()) {
        AreaSearch search;
        search.init(itemX[self.id] - 400, itemY[self.id] - 400, itemX[self.id] + 400, itemY[self.id] + 400,
                    LoopScript(SCRIPT_SHAPE, SCRIPT_INT, SHAPE_CRAWLING_SPIDER, SCRIPT_EQUAL, SCRIPT_END), -1);
        if (search.isValid()) {
            const char direction =
                closestDirection(itemX[self.id], itemY[self.id], itemX[search.id], itemY[search.id], 16, 0, 15);
            if (fireDistance(&self, search.id, direction, 0, 0, 0) != 0U) {
                self.setAttackTarget(search.id);
                return true;
            }
        }
    }
    return false;
}

/* 1 when npc can roll with anim and shoot its target from where it lands;
 * direction gets the direction to the target (1118:16be). The tests of the
 * directions beside the target compare first and mask the truth value: facing
 * == toward-1 & 0xf is (facing == toward-1) & 0xf. The shape is read and not
 * used. */
bool canRoll(u16 npc, u16 anim, char *direction)
{
    Npc self(npc);
    [[maybe_unused]] const u16 shape = self.shape();
    const char facing = self.direction();
    char toward;
    const Npc target(npcs[self.id].attackTarget);
    AnimStatus result;
    ItemPoint from;
    AnimPrimitive *roll;
    if (self.isMostlyOnScreen()) {
        from = self.point();
        if (anim == ANIM_COMBAT_ROLL_LEFT) {
            roll = new CombatRollLeftAnim(npc, facing, &from, 0);
        } else {
            roll = new CombatRollRightAnim(npc, facing, &from, 0);
        }
        roll->block(anchorProcess);
        roll->run();
        result = anchorProcess->result;
        if (!result.bits.ended) {
            return false;
        }
        if (!isVisible(from.x, from.y, from.z, itemShape[self.id], self.frame())) {
            return false;
        }
        toward = closestDirection(from.x, from.y, itemX[target.id], itemY[target.id], 16, 0, 15);
        *direction = toward;
        if (facing == toward || ((static_cast<int>(facing == static_cast<i16>(toward - 1)) & 0xf) != 0) ||
            ((static_cast<int>(facing == static_cast<i16>(toward + 1)) & 0xf) != 0) ||
            ((static_cast<int>(facing == static_cast<i16>(toward - 2)) & 0xf) != 0) ||
            ((static_cast<int>(facing == static_cast<i16>(toward + 2)) & 0xf) != 0)) {
            if (rollDistance(npc, controlledActor, toward, 0, 0, 0, from.x, from.y, from.z) != 0U) {
                return true;
            }
            if (rollDistance(npc, controlledActor, (toward - 1) & 0xf, 0, 0, 0, from.x, from.y, from.z) != 0U) {
                return true;
            }
            if (rollDistance(npc, controlledActor, (toward + 1) & 0xf, 0, 0, 0, from.x, from.y, from.z) != 0U) {
                return true;
            }
            if (rollDistance(npc, controlledActor, (toward - 2) & 0xf, 0, 0, 0, from.x, from.y, from.z) != 0U) {
                return true;
            }
            if (rollDistance(npc, controlledActor, (toward + 2) & 0xf, 0, 0, 0, from.x, from.y, from.z) != 0U) {
                return true;
            }
        }
    }
    return false;
}

/* The distance a shot of npc from x,y,z in direction flies to target, 0 when it
 * does not get there (1118:1a2f). When the shape does not fit and the item in
 * the way is the target, the distance is taken to aim, which only a fitting
 * step sets. */
u16 rollDistance(u16 npc, u16 target, char direction, i16 dx, i16 dy, i16 dz, i16 x, i16 y, i16 z)
{
    Sweep sweep;
    ItemPoint to;
    ItemPoint aim;
    const Item goal(target);
    Item hit;
    u16 distance;
    u16 blocker;
    i16 anim;
    i16 i;
    /* Set with steps = 2, and read only then. */
    i16 nextX = 0;
    i16 nextY = 0;
    i16 nextZ = 0;
    i16 steps = 1;
    AnimFrame frame;
    firingFrameFound = 0;
    AnimInfo info;
    i16 frames;
    bool moved = false;
    const Npc self(npc);
    const u16 shape = self.shape();
    anim = ANIM_FIRE_SMALL;
    info = animInfo(shape, anim);
    frames = info.frames;
    for (i = 0; i < frames; i++) {
        getAnimFrame(shape, anim, direction, i, &frame);
        if (frame.moves()) {
            if (moved) {
                nextX = frame.dx << 4;
                nextY = frame.dy << 4;
                nextZ = frame.dz << 3;
                steps = 2;
                break;
            }
            dx = frame.dx << 4;
            dy = frame.dy << 4;
            dz = frame.dz << 3;
            moved = true;
        }
    }
    if (!moved) {
        return 0;
    }
    for (i = 0; i < steps; i++) {
        if (i == 0) {
            to.set(x + dx, y + dy, z + dz);
        } else {
            to.set(x + nextX, y + nextY, z + nextZ);
        }
        if (shapeFits(SHAPE_SHOT, to.x, to.y, to.z, MOVE_NO_FLOOR, 0, 0, &blocker)) {
            aimPoint(npc, target, &aim);
            sweep.test(SHAPE_SHOT, to, aim.x, aim.y, aim.z, MOVE_NO_FLOOR, 0, 0, 0);
            hit = sweep.hitItem;
            if (!hit.isValid() || hit.id == target) {
                distance = mapDistance(to.x, to.y, aim.x, aim.y);
                return distance >> 4;
            }
        } else if (blocker == target) {
            distance = mapDistance(to.x, to.y, aim.x, aim.y);
            return distance >> 4;
        }
    }
    return 0;
}
