/* CRUSADER.C (Code41, No Regret's 1140, No Remorse's 1128; the addresses
 * are No Regret's): hits and shots, most of them usecode intrinsics, the
 * Silencer's death, and the processes that heal the controlled NPC and
 * charge its battery, the games' differences as each game's values
 * (crusader_game.h). */
#include <cstdlib>
#include "gamestate.h"
#include "crusader.h"
#include "crusader_game.h"
#include "animcach.h"
#include "animfrm.h"
#include "animprim.h"
#include "attack.h"
#include "boboboom.h"
#include "collide.h"
#include "control.h"
#include "crusys.h"
#include "cycle.h"
#include "distance.h"
#include "dsfxman.h"
#include "dtable.h"
#include "explode.h"
#include "firetype.h"
#include "gravity.h"
#include "itemdata.h"
#include "itemfind.h"
#include "kernel.h"
#include "pathfind.h"
#include "priority.h"
#include "random.h"
#include "scrit.h"
#include "sprite.h"
#include "status.h"
#include "system.h"
#include "systimer.h"
#include "tracker.h"
#include "type.h"
#include "utility.h"
#include "world.h"
#include "wpncache.h"
#include "sounds.h"
#include "shapes.h"

namespace {
/* Nothing reads it. */
ItemPoint unknown2d78(0xffff, 0xffff, 0xff);
}
u8 &mouseFiring = gameGlobal<u8>(0);

u8 &firingFrameFound = gameGlobal<u8>(0);

DeathSilence::DeathSilence()
{
    nameProcess(pid, "DeathSilence");
    setType(DEATH_SILENCE_TYPE);
    ticks = 0;
    setRate(1);
    startTimer(1);
    eventMask |= EVENT_TIMER;
    flags |= PROCESS_ACTIVE;
}

void DeathSilence::run()
{
    playEffect(DEATH_SOUND);
    if (DEATH_CLEARS_STASIS) {
        clearStasis();
    }
    terminate(0);
}

void DeathSilence::event(u32 /*event*/, EventData /*data*/)
{
    ticks++;
    if (ticks == 3) {
        flags &= ~PROCESS_ACTIVE;
    }
}

void aimPoint(u16 shooter, u16 target, ItemPoint *point)
{
    i16 z;
    const Item aim(target);
    const Item from(shooter);
    const ShapeInfo info = typeFlags.info(aim.shape());
    i16 fromZ;
    i16 aimZ;
    i16 dz;
    point->x = aim.centreX();
    point->y = aim.centreY();
    z = aim.z();
    z += info.z * 8;
    if (info.z >= 3) {
        z -= 16;
        fromZ = from.z();
        aimZ = aim.z();
        dz = fromZ - aimZ;
        if (dz <= -0x30) {
            z += 8;
        } else if (dz >= 0x30) {
            if (info.z >= 7) {
                z -= 0x18;
            } else if (info.z >= 6) {
                z -= 0x10;
            }
        }
    } else if (info.z != 0U) {
        z -= 8;
    }
    point->z = z;
}

/* Intrinsic 55h (1140:04cb): takeHit without a point. */
void receiveHitIntrinsic(Item *item, u16 hitter, u8 direction, i16 damage, u16 type)
{
    takeHit(item, hitter, direction, damage, type, nullptr);
}

/* The push is never given: the function returns first (original). The test
 * before the push goes with the rest; a call there splits the code, so the
 * common subexpression of item's index outlives it (1140:1973 reads [BP-6],
 * which nothing writes). */
void receiveHit(Item *item, u16 hitter, i16 force)
{
    return;
#pragma GCC diagnostic push
    /* The original's code after the return. */
#pragma GCC diagnostic ignored "-Wunreachable-code"
    Item const other(hitter);
    if ((item->isFixed() == 0U) && (item->weight() != 0)) {
        s32 dx = static_cast<s32>(itemX[item->id]) - itemX[other.id];
        if (dx > 0) {
            dx = force;
        } else if (dx < 0) {
            dx = static_cast<i16>(force * -1);
        }
        s32 dy = static_cast<s32>(itemY[item->id]) - itemY[other.id];
        if (dy > 0) {
            dy = force;
        } else if (dy < 0) {
            dy = static_cast<i16>(force * -1);
        }
        i16 dz = itemZ[item->id] - itemZ[other.id];
        if (dz > 0) {
            dz = force;
        } else if (dz < 0) {
            dz = force * -1;
        }
        hurl(item, dx, dy, dz, 2);
    }
#pragma GCC diagnostic pop
}

/* ROBOT_SHAPES are each game's robots. */
bool isOrganic(Item *item)
{
    const u16 shape = item->shape();
    return !isOneOf(shape, ROBOT_SHAPES);
}

bool dodgeOver(Item * /*unused*/)
{
    const u32 now = systemTimer->time();
    const u32 start = dodgeStart;
    const u32 length = dodgeLength;
    return (start == 0U) || start + length <= now;
}

/* The distance in cells from the firing frame's place to the target's aim
 * point when a shot from there meets the target first; 0 when it meets
 * something else or the NPC has no firing frame (its callers ask only
 * whether it is 0). Where the target itself stands in the shot's place, the
 * shot meets it at once: 1. The original took the distance there to a point
 * aimPoint had not set in that call, its stack's leftovers, so whether an
 * NPC at the target's muzzle fired or walked away was the stack's chance
 * (and the port's, other on each compiler and run). */
u16 fireDistance(Item *item, u16 target, u8 direction, i16 dx, i16 dy, i16 dz)
{
    Sweep sweep;
    ItemPoint from;
    ItemPoint to;
    const Item other(target);
    Item blocker;
    u16 distance;
    u16 blocked;
    i16 anim;
    i16 i;
    /* Set with count = 2, and read only then. */
    i16 dx2 = 0;
    i16 dy2 = 0;
    i16 dz2 = 0;
    i16 count = 1;
    firingFrameFound = 0;
    if (item->isNpc()) {
        AnimFrame frame;
        AnimInfo info;
        i16 frames;
        Npc npc(item->id);
        bool found = false;
        if (npc.isKneeling() != 0U) {
            if (npc.usesSmallAnims()) {
                anim = ANIM_KNEEL_FIRE_SMALL;
            } else {
                anim = ANIM_KNEEL_FIRE_LARGE;
            }
        } else if (npc.isSilencer()) {
            if (npc.usesSmallAnims()) {
                anim = ANIM_FIRE_SMALL;
            } else {
                anim = ANIM_FIRE_LARGE;
            }
        } else {
            anim = ANIM_FIRE_SMALL;
        }
        info = animInfo(item->shape(), anim);
        frames = info.frames;
        for (i = 0; i < frames; i++) {
            getAnimFrame(item->shape(), anim, direction, i, &frame);
            if (frame.moves()) {
                if (found) {
                    dx2 = frame.dx * 16;
                    dy2 = frame.dy * 16;
                    dz2 = frame.dz * 8;
                    count = 2;
                    break;
                }
                dx = frame.dx * 16;
                dy = frame.dy * 16;
                dz = frame.dz * 8;
                found = true;
            }
        }
        if (!found) {
            return 0;
        }
    }
    firingFrameFound = 1;
    for (i = 0; i < count; i++) {
        if (i == 0) {
            from.set(itemX[item->id] + dx, itemY[item->id] + dy, itemZ[item->id] + dz);
        } else {
            from.set(itemX[item->id] + dx2, itemY[item->id] + dy2, itemZ[item->id] + dz2);
        }
        if (shapeFits(SHAPE_SHOT, from.x, from.y, from.z, MOVE_NO_FLOOR, 0, 0, &blocked)) {
            aimPoint(item->id, target, &to);
            sweep.test(SHAPE_SHOT, from, to, MOVE_NO_FLOOR, 0, 0);
            blocker = sweep.hitItem;
            if (!blocker.isValid() || blocker.id == target) {
                distance = mapDistance(from.x, from.y, to.x, to.y);
                return distance / 16;
            }
        } else if (blocked == target) {
            return 1;
        }
    }
    return 0;
}

/* rangeIfVisible when other is within range cells (1140:247f). Nothing calls
 * it. The distance is divided by 16 before it is rounded up (original). */
/* UNCALLED: nothing calls it. */
u16 rangeWithin(Item *item, u16 other, u16 range)
{
    u16 distance;
    ItemPoint from;
    ItemPoint to;
    const Item target(other);
    const ShapeInfo info = typeFlags.info(target.shape());
    i16 z;
    if (!target.isValid()) {
        return 0;
    }
    from = item->point();
    to = target.point();
    z = to.z;
    if (info.z != 1) {
        z += info.z * 8;
    }
    to.z = z;
    distance = mapDistance(from.x, from.y, from.z, to.x, to.y, to.z);
    distance /= 16;
    if ((distance & 0xf) != 0) {
        distance++;
    }
    if (distance <= range) {
        distance = rangeIfVisible(item, other);
        if (distance <= range) {
            return distance;
        }
    }
    return 0;
}

/* The distance is divided by 16 before it is rounded up (original). */
u16 rangeIfVisible(Item *item, u16 other)
{
    Sweep sweep;
    ItemPoint from;
    ItemPoint to;
    const Item target(other);
    i16 z;
    u16 distance;
    const ShapeInfo info = typeFlags.info(target.shape());
    if (!target.isValid()) {
        return 0;
    }
    from = item->point();
    to = target.point();
    z = to.z;
    if (info.z != 0 && info.z != 1) {
        z += (info.z - 1) * 8;
    }
    to.z = z;
    sweep.test(item->shape(), from, to, MOVE_NO_FLOOR, item->id, other);
    if ((sweep.targetHit != 0U) && (sweep.targetFirst != 0U)) {
        distance = mapDistance(from.x, from.y, from.z, to.x, to.y, to.z);
        distance /= 16;
        if ((distance & 0xf) != 0) {
            distance++;
        }
        if (distance <= 0x30) {
            return distance;
        }
    }
    return 0;
}

CruHealer::CruHealer()
{
    nameProcess(pid, "CruHealer");
    setType(HEALER_TYPE);
    setItem(controlledNpc.id);
    controlledNpc.held = 1;
    maxHp = controlledNpc.maxHp();
    controlledNpc.playLoop(SOUND_HEALING);
}

void CruHealer::onKill()
{
    controlledNpc.cancelSound(SOUND_HEALING);
    controlledNpc.held = 0;
}

void CruHealer::run()
{
    i16 hp = controlledNpc.hp();
    if (maxHp != hp) {
        if (controlledNpc.isSoundPlaying(SOUND_HEAL_POINT) == 0U) {
            controlledNpc.playSound(SOUND_HEAL_POINT);
        }
        hp++;
        if (maxHp < hp) {
            hp = maxHp;
        }
        controlledNpc.setHp(hp);
        updateStatus(STATUS_HEALTH);
        setTimeout(1);
        return;
    }
    terminate(0);
}

BatteryCharger::BatteryCharger()
{
    nameProcess(pid, "BatteryCharger");
    setType(HEALER_TYPE);
    setItem(controlledNpc.id);
    controlledNpc.held = 1;
    maxEnergy = controlledNpc.maxEnergy();
    controlledNpc.playLoop(SOUND_CHARGING);
}

void BatteryCharger::onKill()
{
    controlledNpc.cancelSound(SOUND_CHARGING);
    controlledNpc.held = 0;
}

void BatteryCharger::run()
{
    u16 energy = controlledNpc.mana();
    if (maxEnergy != energy) {
        if (controlledNpc.isSoundPlaying(SOUND_CHARGE_POINT) == 0U) {
            controlledNpc.playSound(SOUND_CHARGE_POINT);
        }
        energy += 25;
        if (maxEnergy < energy) {
            energy = maxEnergy;
        }
        controlledNpc.setMana(energy);
        updateStatus(STATUS_ENERGY);
        setTimeout(1);
        return;
    }
    terminate(0);
}

u16 createBatteryCharger()
{
    BatteryCharger *charger = new BatteryCharger;
    return charger->pid;
}

u16 createHealer()
{
    CruHealer *healer = new CruHealer;
    return healer->pid;
}

namespace {
/* npcAnim's pairs of an animation and the one played instead, and the pairs
 * meant for map 25; the animations the Silencer's enemies keep are each
 * game's (KEPT_ANIMS). */
i16 anims[28] = {ANIM_COMBAT_STAND_SMALL,
                 ANIM_COMBAT_STAND_LARGE,
                 ANIM_UNREADY_SMALL,
                 ANIM_UNREADY_LARGE,
                 ANIM_READY_SMALL,
                 ANIM_READY_LARGE,
                 ANIM_FIRE_SMALL,
                 ANIM_FIRE_LARGE,
                 ANIM_KNEELING_SMALL,
                 ANIM_KNEELING_LARGE,
                 ANIM_KNEEL_FIRE_SMALL,
                 ANIM_KNEEL_FIRE_LARGE,
                 ANIM_RELOAD_SMALL,
                 ANIM_RELOAD_LARGE,
                 ANIM_COMBAT_RUN_SMALL,
                 ANIM_COMBAT_RUN_LARGE,
                 ANIM_RUN,
                 ANIM_COMBAT_RUN_LARGE,
                 ANIM_START_RUN_SMALL,
                 ANIM_START_RUN_LARGE,
                 ANIM_STOP_RUN_SMALL,
                 ANIM_STOP_RUN_LARGE,
                 ANIM_ADVANCE_SMALL,
                 ANIM_ADVANCE_LARGE,
                 ANIM_RETREAT_SMALL,
                 ANIM_RETREAT_LARGE,
                 -1,
                 -1};
i16 map25Anims[14] = {ANIM_COMBAT_STAND_SMALL,
                      ANIM_STAND,
                      ANIM_COMBAT_RUN_SMALL,
                      ANIM_RUN,
                      ANIM_START_RUN_SMALL,
                      ANIM_START_RUN,
                      ANIM_START_RUN_LARGE,
                      ANIM_START_RUN,
                      ANIM_STOP_RUN_SMALL,
                      ANIM_STAND,
                      ANIM_STOP_RUN_LARGE,
                      ANIM_STAND,
                      -1,
                      -1};
}

u8 &finalHit = gameGlobal<u8>(0);

void GameQuit::run()
{
    cycleProcess->paused = 1;
    systemShutdown(1);
}

/* The Silencer kneels up and turns to direction 6; a second later the end
 * sequence runs. */
EndSequence::EndSequence()
{
    u16 animPid;
    u16 turnPid;
    Process *anim = nullptr;
    Process *turn = nullptr;
    Process *first;
    Process *wait;
    wait = new WaitProcess(60, 0);
    controlledNpc.held = 1;
    killItemProcesses(1, ANY_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    if (controlledNpc.isKneeling() != 0U) {
        animPid = controlledNpc.doAnim(ANIM_KNEEL_END, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
        anim = findProcess(animPid);
    }
    turnPid = turnNpc(1, 6, 1, 1);
    if (turnPid != 0U) {
        turn = findProcess(turnPid);
    }
    if ((anim != nullptr) && (turn != nullptr)) {
        anim->block(turn)->block(wait)->block(this);
    } else {
        if (anim != nullptr) {
            first = anim;
        } else {
            first = turn;
        }
        first->block(wait)->block(this);
    }
}

/* The Silencer dies; two seconds after its animation the game ends. */
void EndSequence::run()
{
    Process *anim;
    Process *wait;
    Process *quit;
    controlledNpc.clearImmortal();
    finalHit = 1;
    receiveHitIntrinsic(&controlledNpc, 0, 0x10, 0xff, 3);
    finalHit = 0;
    anim = findItemProcess(1, ANIM_TYPE);
    wait = new WaitProcess(120, 0);
    quit = new GameQuit;
    anim->block(wait)->block(quit);
    terminate(0);
}

void resetBossShield()
{
    if (bossShield != 0) {
        bossShield = 500;
    }
}

/* On map 25 the pairs are taken from anims, only as many as map25Anims has
 * (original). */
u16 npcAnim(Npc *npc, u16 anim)
{
    i16 i;
    bool big = false;
    u16 shape;
    u16 overlay;
    const Item weapon(npc->activeWeapon());
    if (!npc->isSilencer()) {
        for (i = 0; KEPT_ANIMS[i] != -1 && static_cast<ui16>(KEPT_ANIMS[i]) != anim; i++) {
            ;
        }
        if (KEPT_ANIMS[i] != -1) {
            return anim;
        }
        if (anim == ANIM_COMBAT_RUN_SMALL) {
            return ANIM_RUN;
        }
    }
    if (npc->onBaseMap()) {
        for (i = 0; map25Anims[i] != -1; i += 2) {
            if (static_cast<ui16>(anims[i]) == anim) {
                anim = anims[static_cast<i16>(i + 1)];
                break;
            }
        }
    } else if (weapon.isValid() && (smallWeapon(weapon.shape()) == 0U)) {
        for (i = 0; anims[i] != -1; i += 2) {
            if (static_cast<ui16>(anims[i]) == anim) {
                if (anim == ANIM_FIRE_SMALL || anim == ANIM_KNEEL_FIRE_SMALL) {
                    shape = weapon.shape();
                    overlay = overlayShapeOf(shape);
                    if (npc->isSilencer() && (overlay == 0x36e || overlay == 0x33b)) {
                        big = true;
                    }
                    if (anim == ANIM_FIRE_SMALL) {
                        if (big) {
                            anim = ANIM_BRIGHT_FIRE_LARGE;
                        } else {
                            anim = ANIM_FIRE_LARGE;
                        }
                    } else if (big) {
                        anim = ANIM_BRIGHT_KNEEL_FIRE;
                    } else {
                        anim = ANIM_KNEEL_FIRE_LARGE;
                    }
                } else {
                    anim = anims[static_cast<i16>(i + 1)];
                }
                break;
            }
        }
    } else if (SMALL_WEAPON_ANIMS) {
        for (i = 0; anims[i] != -1; i += 2) {
            if (static_cast<ui16>(anims[i]) == anim) {
                anim = anims[static_cast<i16>(i + 1)];
                break;
            }
        }
    }
    return anim;
}

u16 fireWeapon(Item *item, i16 x, i16 y, i16 z, char direction, i16 type, u8 aimed)
{
    SuperSprite *shot = nullptr;
    ItemPoint start;
    i16 frame;
    i16 i;
    i16 shots;
    i16 damage;
    AnimInfo info;
    ItemPoint aim;
    Item target;
    char toward;
    u16 blocker;
    u8 same;
    u16 weapon;
    i16 radius;
    Npc shooter(item->id);
    start = item->point();
    start.x += x;
    start.y += y;
    z += start.z;
    start.z = z;
    if (!shapeFits(SHAPE_SHOT, start.x, start.y, start.z, MOVE_NO_FLOOR, 0, 0, &blocker)) {
        Item hit(blocker);
        toward = ::direction(start.x, start.y, hit.x(), hit.y());
        takeHit(&hit, item->id, toward, damageOf(type), type, &start);
        radius = fireType(type)->range;
        if (radius != 0) {
            hurtArea(&start, radius, damageOf(type), type, hit.id, item->id);
        }
        if (fireTypes[type].nearSprite != 0U) {
            fireExplosion(type, start.x, start.y, start.z);
        }
        return 0;
    }
    shots = fireTypes[type].shots;
    /* Fire types 10h-16h are No Regret's (No Remorse's end at 0Fh). */
    switch (type) {
    case FIRE_RP:
    case FIRE_BA:
        frame = 0;
        break;
    case FIRE_GL_303:
        frame = direction + 0x11;
        break;
    case FIRE_PA:
        frame = direction + 1;
        break;
    case FIRE_SG_A1:
        frame = 0;
        break;
    case FIRE_PL_1:
        frame = 0x46;
        break;
    case 9:
        frame = direction + 0x11;
        break;
    case FIRE_AR_7:
        frame = direction + 0x11;
        break;
    case 8:
    case 0x100:
        frame = 0;
        break;
    case FIRE_AC_88:
        frame = 0;
        break;
    case FIRE_UV_9:
        frame = borlandRandom(5) + 0x47;
        break;
    case FIRE_ELECTRIC:
        frame = 0;
        break;
    case FIRE_EM_4:
        frame = 0x4c;
        break;
    case FIRE_BK_16:
        frame = direction + 0x50;
        break;
    case FIRE_LNR_81:
        frame = direction * 6 + 0x78;
        break;
    case 18:
        frame = 0x4c;
        break;
    case 19:
        frame = 0x4c;
        break;
    case FIRE_XP_5:
        frame = direction * 3 + 0xdc;
        break;
    case 21:
        frame = direction + 0x64;
        break;
    case FIRE_BOSS_GRENADE:
        frame = direction + 0x11;
        break;
    default:
        halt();
    }
    if (item->isNpc()) {
        same = 0;
        if (shooter.activeItem2() != 0U) {
            const Item gun(shooter.activeItem2());
            weapon = weaponOf(gun.shape());
            if (weapon == static_cast<ui16>(type)) {
                same = 1;
            }
        }
        damage = shooter.weaponDamage(same);
        if (WEAK_WEAPONS_TAKE_TYPE_DAMAGE && damage <= 1) {
            damage = damageOf(type);
        }
        info = animInfo(item->shape(), shooter.lastAnimSet());
    } else {
        damage = damageOf(type);
        info.sixteenDirections = 1;
    }
    if (aimed != 0U) {
        if (item->isControlled()) {
            target = findTarget(item->id, direction, info.sixteenDirections);
        } else if (NPCS_AIM_AT_ATTACK_TARGET && item->isNpc()) {
            target = npcs[shooter.id].attackTarget;
        } else {
            target = controlledNpcNumber();
        }
    }
    if (target.isValid()) {
        aimPoint(item->id, target.id, &aim);
    } else {
        aim.x = 0xffff;
    }
    if (item->isControlled() && (reticleShown == 0U)) {
        showReticle();
    }
    for (i = 0; i < shots; i++) {
        shot = new SuperSprite;
        if (frame == 0) {
            shot->place(0x1d9, &start);
            if (aim.x == 0xffff) {
                if (item->id == controlledActor && (reticleShown != 0U)) {
                    shot->launchAtReticle(direction, damage, type, item->id, target.id, z);
                } else {
                    shot->launchInDirection(direction, damage, type, item->id, target.id, aimed);
                }
            } else {
                shot->launch(direction, aim, damage, type, item->id, target.id, 1);
            }
        } else {
            shot->place(0x1d9, frame, &start);
            if (aim.x == 0xffff) {
                if (item->id == controlledActor && (reticleShown != 0U)) {
                    shot->launchAtReticle(direction, damage, type, item->id, target.id, z);
                } else {
                    shot->launchInDirection(direction, damage, type, item->id, target.id, aimed);
                }
            } else {
                shot->launch(direction, aim, damage, type, item->id, target.id, 1);
            }
        }
    }
    if (shot != nullptr) {
        return shot->pid;
    }
    return 0;
}

namespace {
/* How far a hit item is pushed, by the direction of the hit (takeHit);
 * with no direction, HURL_X_NO_DIRECTION and HURL_Y_NO_DIRECTION (hurlOf). */
i16 hurlX[16] = {0, 1, 2, 2, 2, 2, 2, 1, 0, -1, -2, -2, -2, -2, -2, -1};
i16 hurlY[16] = {-2, -2, -2, -1, 0, 1, 2, 2, 2, 2, 2, 1, 0, -1, -2, -2};
/* No Regret's death sounds of women, of the armoured guards and of men,
 * and its hurt cries (takeHit). */
i16 femaleDeathSounds[4] = {0xd8, 0x10, 0x21f, 0x220};
i16 otherDeathSounds[13] = {0x22b, 0x22e, 0x231, 0x232, 0x233, 0x235, 0x223, 0x22c, 0x22d, 0x22f, 0x234, 0x237, 0x238};
i16 maleDeathSounds[9] = {0x88, 0x8c, 0x8f, 0x221, 0x224, 0x227, 0x228, 0x229, 0x22a};
i16 femaleHurtSounds[1] = {0xd8};
i16 maleHurtSounds[9] = {0x8f, 0x230, 0x239, 0x23a, 0x23b, 0x23c, 0x23d, 0x23e, 0x23f};
/* No Remorse's death sounds of women and men, and of an EM-4 death
 * (takeHit). */
i16 remorseFemaleDeathSounds[2] = {0xd8, 0x10};
i16 remorseMaleDeathSounds[3] = {0x88, 0x8c, 0x8f};
i16 emDeathSounds[2] = {0xd9, 0xda};

/* A hit on an NPC as its death or its hurt takes it (npcTakesHit): the
 * NPC's shape and facing when the hit came, the hit's direction, its damage
 * after the shields and its fire type, where it struck, and whether the NPC
 * is organic. */
struct NpcHit {
    u16 shape;
    char facing;
    u8 direction;
    i16 damage;
    u16 type;
    const ItemPoint *point;
    bool organic;
};

/* The damage of a hit by the difficulty: a fifth (difficulty 1) or a third
 * (2) for the controlled NPC and the Silencer, five or three times for the
 * others; a third of the RP, SG-A1, BA and AC-88's on a robot; at most 250
 * and at least 1. */
i16 scaledDamage(Item *item, i16 damage, u16 type)
{
    if (item->isControlled() || item->isSilencer()) {
        if (controlledNpc.difficulty == 1) {
            damage /= 5;
        } else if (controlledNpc.difficulty == 2) {
            damage /= 3;
        }
    } else if (controlledNpc.difficulty == 1) {
        damage *= 5;
    } else if (controlledNpc.difficulty == 2) {
        damage *= 3;
    }
    if (!isOrganic(item) && (type == FIRE_RP || type == FIRE_SG_A1 || type == FIRE_BA || type == FIRE_AC_88)) {
        damage /= 3;
    }
    if (damage > 250) {
        damage = 250;
    } else if (damage == 0) {
        damage = 1;
    }
    return damage;
}

/* No Regret's boss robot while its shield lasts: the shield takes the
 * damage and flares at here, which is only set when the hit had no point
 * (original); its last flare is longer, with a sound. */
void bossRobotShieldTakes(Item *item, i16 damage, const ItemPoint &here)
{
    bossShield -= damage;
    if (bossShield <= 0) {
        bossShield = 0;
        createSpriteProcess(SHAPE_SHIELD_FLARE, 10, 0x13, 3, here.x, here.y, here.z);
        item->playSound(SOUND_BOSS_SHIELD_DOWN);
    } else {
        createSpriteProcess(SHAPE_SHIELD_FLARE, 0, 9, 3, here.x, here.y, here.z);
    }
}

/* No Remorse's boss (Vargas) while his shield lasts: he teleports out and
 * back in and the shield takes the damage; a hit while he teleports is
 * lost. */
void vargasShieldTakes(Npc &npc, i16 damage)
{
    const auto *anim = static_cast<const AnimPrimitive *>(findItemProcess(npc.id, ANIM_TYPE));
    if ((anim != nullptr) && (anim->anim == ANIM_TELEPORT_OUT || anim->anim == ANIM_TELEPORT_IN)) {
        return;
    }
    npc.doAnim(ANIM_TELEPORT_OUT, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
    npc.doAnim(ANIM_TELEPORT_IN, DIRECTION_CURRENT, 1, 0);
    bossShield -= damage;
    if (bossShield < 0) {
        bossShield = 0;
    }
}

/* An NPC the player did not control turns on the controlled NPC when hit:
 * one in combat that was guarding attacks, one not in combat takes its
 * third activity and attacks if that is a fight. When the controlled NPC
 * hit it from near on the screen (unknown77Bit5) and it can shoot back, it
 * stops finding its way. */
void turnOnAttacker(Item *item, Npc &npc, u16 hitter)
{
    const Npc attacker(hitter);
    if (npc.isInCombat() != 0U) {
        if (npc.activity() == ACTIVITY_GUARD) {
            npc.setActivity(ACTIVITY_ATTACK);
        }
        npc.setAttackTarget(controlledActor);
    } else {
        npc.setActivity(ACTIVITY_DEFAULT2);
        if (npc.isInCombat() != 0U) {
            npc.setAttackTarget(controlledActor);
        }
    }
    if (attacker.isValid() && attacker.isNpc() && attacker.isControlled() && (npc.unknown77Bit5() != 0U)) {
        const i16 distance = mapDistance(itemX[item->id], itemY[item->id], itemZ[item->id], itemX[attacker.id],
                                         itemY[attacker.id], itemZ[attacker.id]);
        if (distance <= 0x280 && item->isOnScreen()) {
            const char toward =
                closestDirection(itemX[item->id], itemY[item->id], itemX[attacker.id], itemY[attacker.id], 16, 0, 15);
            if (fireDistance(&npc, hitter, toward, 0, 0, 0) != 0U) {
                killItemProcesses(item->id, PATH_FINDER_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
            }
        }
    }
}

/* The Silencer's shield takes a hit of a fire type it stops
 * (FireType::shieldMask; the graviton shield, type 3, is mask 4) while his
 * energy pays the fire type's cost: it flashes at the hit (the ionic and
 * plasma shields) or shows a sphere around him (the graviton shield), one
 * sprite at a time. Returns whether it took the hit. */
bool silencerShieldTakes(Npc &npc, u16 type, const ItemPoint &point)
{
    i16 shield = npc.shieldType();
    if (shield == 3) {
        shield = 4;
    }
    if (shield == 0) {
        return false;
    }
    const FireType *fire = fireType(type);
    if ((shield & fire->shieldMask) == 0) {
        return false;
    }
    const i16 cost = fire->shieldCost;
    i16 mana = npc.mana();
    if (mana < cost) {
        return false;
    }
    mana -= cost;
    npc.setMana(mana);
    npc.restartSound(SOUND_SHIELD_ABSORBS);
    updateStatus(STATUS_ENERGY);
    if (silencerSprite == 0U) {
        if (shield == 1) {
            createSpriteProcess(SHAPE_SHIELD_FLASH, 7, 13, 4, point.x, point.y, point.z);
        } else if (shield == 2) {
            createSpriteProcess(SHAPE_SHIELD_FLASH, 0, 6, 4, point.x, point.y, point.z);
        } else {
            ItemPoint around;
            around.set(controlledNpc.x() + 16, controlledNpc.y() + 24, controlledNpc.z());
            silencerSprite = createSpriteProcess(SHAPE_SHIELD_SPHERE, 0, 8, 4, around.x, around.y, around.z);
            findProcess(silencerSprite)->listenTo(controller->pid);
        }
    }
    return true;
}

/* The start of an NPC's death: its sounds and processes end; if the
 * player controlled it, the target goes, and the Silencer's death begins
 * (not when he froze) or the control goes back to him. */
void beginDeath(Item *item, Npc &npc, u16 type)
{
    npc.stopSounds();
    killItemProcesses(item->id, ANY_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    if (item->isControlled()) {
        setTarget(0);
        if (item->isSilencer()) {
            if (type != FIRE_BK_16 && type != 18) {
                new DeathSilence;
            }
        } else {
            setControlledNpcNumber(0);
        }
    }
}

/* No Regret's: a dead NPC torn apart in flames, where that shape has room.
 * Returns whether it is. */
bool tearApart(Item *item, Npc &npc)
{
    item->push();
    if (!shapeFits(SHAPE_BURNING_GIBS, itemX[item->id], itemY[item->id], itemZ[item->id], MOVE_STANDS, item->id, 0,
                   nullptr)) {
        item->pop();
        return false;
    }
    item->popToCoords(itemX[item->id], itemY[item->id], itemZ[item->id]);
    npc.setShape(SHAPE_BURNING_GIBS);
    setFirstFrame(item->id, ANIM_FALL_BACKWARDS);
    npc.doAnim(ANIM_FALL_BACKWARDS, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
    return true;
}

/* A dead NPC burning, where the burning shape has room: it runs a few steps
 * in flames, each turning by chance, falls backwards, then forwards a few
 * times, rolls and cries out. Returns whether it burns. */
bool burnToDeath(Item *item, Npc &npc)
{
    item->push();
    if (!shapeFits(SHAPE_BURNING, itemX[item->id], itemY[item->id], itemZ[item->id], MOVE_STANDS, item->id, 0,
                   nullptr)) {
        item->popToCoords(itemX[item->id], itemY[item->id], itemZ[item->id]);
        return false;
    }
    item->popToCoords(itemX[item->id], itemY[item->id], itemZ[item->id]);
    char spin = npc.direction();
    npc.setShape(SHAPE_BURNING);
    setFirstFrame(item->id, ANIM_WALK);
    i16 spins = borlandRandom(9);
    if ((spin & 1) != 0) {
        spin = (spin + 1) & 0xf;
    }
    for (i16 i = 0; i < spins; i++) {
        spin = (spin + randomBetween(-1, 1) * 2) & 0xf;
        npc.doAnim(ANIM_WALK, spin, EVEN_IF_BUSY, 0);
    }
    npc.doAnim(ANIM_FALL_BACKWARDS, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
    spins = borlandRandom(3) + 1;
    for (i16 i = 0; i < spins; i++) {
        npc.doAnim(ANIM_FALL_FORWARDS, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
    }
    npc.doAnim(ANIM_KNEEL_ROLL_LEFT, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
    makeNpcSound(item->id);
    return true;
}

/* A death by an explosive fire type (GL-303, an explosion, AR-7, 12) is in
 * flames when the damage is heavy for the fire type (damageOf): No
 * Regret's tears the NPC apart from 8 tenths by chance, or at an
 * explosion's last hit (not fire type 12), and burns it from 6 tenths; No
 * Remorse's burns it from 3 fifths (GL-303) or 2 fifths. Returns whether
 * the NPC dies so. */
bool diesInFlames(Item *item, Npc &npc, i16 damage, u16 type)
{
    i16 heavy = damageOf(type);
    if (TAKE_HIT_NO_REGRET) {
        const i16 threshold = static_cast<i16>(heavy * 8) / 10;
        heavy = static_cast<i16>(heavy * 6) / 10;
        if (((damage >= threshold && oneIn(2)) || (finalHit != 0U)) && type != 12) {
            return tearApart(item, npc);
        }
    } else if (type == FIRE_GL_303) {
        heavy = static_cast<i16>(heavy * 3) / 5;
    } else {
        heavy = static_cast<i16>(heavy * 2) / 5;
    }
    return damage >= heavy && burnToDeath(item, npc);
}

/* The shape a dead organic NPC lies in by the fire type that killed it:
 * PL-1's plasma, UV-9's skeleton (with violence), EM-4's, and No Regret's
 * own: BK-16 and 18 freeze it, LNR-81's, XP-5's gibs (with violence). The
 * electric fire type makes the Silencer twitch. */
void setDeathShape(Item *item, Npc &npc, u16 type, bool organic)
{
    if (type == FIRE_ELECTRIC) {
        if (npc.isSilencer()) {
            npc.doAnim(ANIM_ELECTROCUTED, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
        }
        return;
    }
    if (!organic) {
        return;
    }
    u16 dead;
    u16 anim = ANIM_FALL_BACKWARDS;
    if (type == FIRE_PL_1) {
        dead = SHAPE_PLASMA_DEATH;
        anim = ANIM_STAND;
    } else if (type == FIRE_UV_9 && violenceEnabled()) {
        dead = SHAPE_SKELETON_DEATH;
    } else if (type == FIRE_EM_4) {
        dead = SHAPE_EM_DEATH;
    } else if (TAKE_HIT_NO_REGRET && (type == FIRE_BK_16 || type == 18)) {
        dead = SHAPE_FROZEN;
    } else if (TAKE_HIT_NO_REGRET && type == FIRE_LNR_81) {
        dead = SHAPE_LNR_DEATH;
    } else if (TAKE_HIT_NO_REGRET && type == FIRE_XP_5 && violenceEnabled()) {
        dead = SHAPE_GIBS;
    } else {
        return;
    }
    item->setShape(dead);
    setFirstFrame(item->id, anim);
}

/* No Regret's kneeling guards die in their kneeling death shapes, unless
 * the fire type gave them another. */
void kneelingDeathShape(Item *item, u16 shape)
{
    if (item->shape() != shape) {
        return;
    }
    u16 dead;
    if (shape == SHAPE_HQ_GUARD_KNEELING_A || shape == SHAPE_HQ_GUARD_KNEELING_B) {
        dead = SHAPE_HQ_GUARD_KNEELING_DEATH;
    } else if (shape == SHAPE_ENFORCER_KNEELING_A || shape == SHAPE_ENFORCER_KNEELING_B) {
        dead = SHAPE_ENFORCER_KNEELING_DEATH;
    } else if (shape == SHAPE_LMC_GUARD_KNEELING_A || shape == SHAPE_LMC_GUARD_KNEELING_B) {
        dead = SHAPE_LMC_GUARD_KNEELING_DEATH;
    } else if (shape == SHAPE_STORM_TROOPER_KNEELING_A || shape == SHAPE_STORM_TROOPER_KNEELING_B) {
        dead = SHAPE_STORM_TROOPER_KNEELING_DEATH;
    } else {
        return;
    }
    item->setShape(dead);
    setFirstFrame(item->id, ANIM_FALL_BACKWARDS);
}

/* How a dead NPC falls (fallDead). */
enum class Fall { BACKWARDS, FORWARDS, EITHER };

/* By the hit's direction against the NPC's facing, of 16: forwards when
 * the hit came the way it faced, up to three directions either side of it
 * or four after it, backwards when from further round; either way when it
 * is four before it, when the NPC faced no way, and for the android and the
 * plasma and skeleton deaths. */
Fall fallFor(char facing, u8 direction, u16 deadShape)
{
    if (facing >= NO_DIRECTION || deadShape == SHAPE_ANDROID || deadShape == SHAPE_PLASMA_DEATH ||
        deadShape == SHAPE_SKELETON_DEATH) {
        return Fall::EITHER;
    }
    if (facing == direction) {
        return Fall::FORWARDS;
    }
    for (i16 steps = 1; steps <= 4; steps++) {
        if (((facing + steps) & 0xf) == direction) {
            return Fall::FORWARDS;
        }
        if (((facing - steps) & 0xf) == direction) {
            return steps == 4 ? Fall::EITHER : Fall::FORWARDS;
        }
    }
    return Fall::BACKWARDS;
}

/* A dead NPC falls (fallFor), either way by chance; backwards when its
 * shape cannot fall forwards. */
void fallDead(Npc &npc, u16 deadShape, char facing, u8 direction)
{
    bool forwards = false;
    if (hasAnim(deadShape, ANIM_FALL_FORWARDS)) {
        const Fall fall = fallFor(facing, direction, deadShape);
        forwards = fall == Fall::FORWARDS || (fall == Fall::EITHER && borlandRandom(2) == 0);
    }
    npc.doAnim(forwards ? ANIM_FALL_FORWARDS : ANIM_FALL_BACKWARDS, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
}

/* An NPC cries out, unless it already does. */
void cryOut(Npc &npc, i16 sound)
{
    if (npc.isSoundPlaying(sound) == 0U) {
        npc.playSound(sound);
    }
}

/* No Regret's end, on the last map: when none of the boss robots lives,
 * the end sequence follows a wait. */
void endIfBossRobotsDead()
{
    AreaSearch search;
    bool allDead = true;
    search.init(0, 0, 0x7fff, 0x7fff, LoopScript(SCRIPT_SHAPE, SCRIPT_INT, SHAPE_BOSS_ROBOT, SCRIPT_EQUAL, SCRIPT_END),
                -1);
    while (search.isValid()) {
        Npc other(search.id);
        if (other.isDead() == 0U) {
            allDead = false;
            break;
        }
        search.next();
    }
    if (allDead) {
        WaitProcess *wait = new WaitProcess(0x1a4, 0);
        EndSequence *end = new EndSequence;
        wait->resume();
        wait->block(end);
    }
}

/* No Regret's death of an NPC: frozen ones shatter once they lie still,
 * the others die in flames or in the fire type's shape, or fall (the
 * kneeling guards in their own) and cry out; an NPC with its hit points
 * still gone drops what it carries (not the Silencer) and is dead, and the
 * last boss robot's death ends the game. */
void noRegretDeath(Item *item, Npc &npc, const NpcHit &hit)
{
    beginDeath(item, npc, hit.type);
    bool inFlames = false;
    if (hit.shape == SHAPE_FROZEN) {
        if (!npc.isBusy()) {
            npc.clearDead();
            item->setShape(SHAPE_FROZEN_SHATTERING);
            setFirstFrame(item->id, ANIM_FALL_BACKWARDS);
        }
    } else if (hit.type == FIRE_GL_303 || hit.type == FIRE_EXPLOSION || hit.type == FIRE_AR_7 || hit.type == 12) {
        if (hit.organic && violenceEnabled()) {
            inFlames = diesInFlames(item, npc, hit.damage, hit.type);
        }
    } else {
        setDeathShape(item, npc, hit.type, hit.organic);
    }
    if (!inFlames) {
        kneelingDeathShape(item, hit.shape);
        fallDead(npc, item->shape(), hit.facing, hit.direction);
        if ((npc.hasDeathCry() != 0U) && violenceEnabled()) {
            i16 sound;
            if (npc.isFemale() != 0U) {
                sound = femaleDeathSounds[borlandRandom(4)];
            } else if (hit.shape == SHAPE_STORM_TROOPER || hit.shape == SHAPE_ELITE_GUARD ||
                       hit.shape == SHAPE_STORM_TROOPER_KNEELING_A || hit.shape == SHAPE_STORM_TROOPER_KNEELING_B ||
                       hit.shape == SHAPE_ELITE_GRENADIER) {
                sound = otherDeathSounds[borlandRandom(13)];
            } else {
                sound = maleDeathSounds[borlandRandom(9)];
            }
            cryOut(npc, sound);
        }
    }
    if (npc.hp() == 0) {
        if (!npc.isSilencer()) {
            npc.destroyContents();
        }
        npc.setDead();
        if (hit.shape == SHAPE_BOSS_ROBOT && npcMap(&controlledNpc.id) == 0xe6) {
            endIfBossRobotsDead();
        }
    }
}

/* No Regret's hurt NPC: the Silencer shocked by the electric fire type
 * sparks; a hurt organic NPC (the Silencer at fire type 8 too) may cry out
 * (the controlled NPC one time in three); EM-4 and the electric fire type
 * make the Silencer cry out and the robots feint by chance, flaring. */
void noRegretHurt(Item *item, Npc &npc, const NpcHit &hit)
{
    if (item->isSilencer() && hit.type == FIRE_ELECTRIC) {
        const u16 pid = createSpriteProcess(SHAPE_SHOCK_SPARKS, 0, 9, 4, itemX[controlledNpc.id],
                                            itemY[controlledNpc.id], itemZ[controlledNpc.id]);
        if (pid != 0U) {
            silencerSprite = pid;
            findProcess(pid)->listenTo(controller->pid);
        }
    }
    if (hit.damage == 0) {
        return;
    }
    if (hit.organic || (hit.type == 8 && item->isSilencer())) {
        const i16 sound =
            (npc.isFemale() != 0U) ? femaleHurtSounds[borlandRandom(1)] : maleHurtSounds[borlandRandom(9)];
        if ((npc.isSoundPlaying(sound) == 0U) && (!npc.isControlled() || oneIn(3))) {
            npc.playSound(sound);
        }
    }
    if (hit.type != FIRE_EM_4 && hit.type != FIRE_ELECTRIC) {
        return;
    }
    if (item->isSilencer()) {
        if (hit.damage > 0) {
            npc.playSound((borlandRandom(2) != 0) ? 5 : 0x14);
        }
        return;
    }
    const u16 shape = item->shape();
    if ((shape == SHAPE_VETRON || shape == SHAPE_THERMATRON || shape == SHAPE_CHEDITRON || shape == SHAPE_ANDROID ||
         shape == SHAPE_AVATRON || shape == SHAPE_CAMETRON || shape == SHAPE_CRYOTRON) &&
        borlandRandom(10) >= 6) {
        Attack *attack = static_cast<Attack *>(firstProcess(item->id, ATTACK_TYPE));
        if (attack != nullptr) {
            attack->startFeint();
            const i16 frame = borlandRandom(2) * 10;
            createSpriteProcess(SHAPE_SHIELD_FLARE, frame, frame + 9, 3, hit.point->x, hit.point->y, hit.point->z);
        }
    }
}

/* No Remorse's death of an NPC: it dies in flames or in the fire type's
 * shape, or falls and cries out (organic, with violence); then it is
 * dead. */
void noRemorseDeath(Item *item, Npc &npc, const NpcHit &hit)
{
    beginDeath(item, npc, hit.type);
    bool inFlames = false;
    if (hit.type == FIRE_GL_303 || hit.type == FIRE_EXPLOSION || hit.type == FIRE_AR_7 || hit.type == 12) {
        if (hit.organic && violenceEnabled()) {
            inFlames = diesInFlames(item, npc, hit.damage, hit.type);
        }
    } else {
        setDeathShape(item, npc, hit.type, hit.organic);
    }
    if (!inFlames) {
        fallDead(npc, item->shape(), hit.facing, hit.direction);
        if (hit.organic && violenceEnabled()) {
            i16 sound;
            if (hit.type == FIRE_EM_4) {
                sound = emDeathSounds[borlandRandom(2)];
            } else if (npc.isFemale() != 0U) {
                sound = remorseFemaleDeathSounds[borlandRandom(2)];
            } else {
                sound = remorseMaleDeathSounds[borlandRandom(3)];
            }
            cryOut(npc, sound);
        }
    }
    npc.setDead();
}

/* No Remorse's hurt NPC: an organic one (the Silencer at fire type 8 too)
 * cries out; EM-4 and the electric fire type make the Silencer twitch and
 * the robots feint one time in three. */
void noRemorseHurt(Item *item, Npc &npc, const NpcHit &hit)
{
    if (hit.organic || (hit.type == 8 && item->isSilencer())) {
        cryOut(npc, (npc.isFemale() != 0U) ? SOUND_HURT_FEMALE : SOUND_HURT_MALE);
    }
    if (hit.type != FIRE_EM_4 && hit.type != FIRE_ELECTRIC) {
        return;
    }
    if (item->isSilencer()) {
        if (hit.damage > 0) {
            killItemProcesses(item->id, ANIM_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
            npc.doAnim(ANIM_ELECTROCUTED, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
        }
        return;
    }
    const u16 shape = item->shape();
    if ((shape == SHAPE_VETRON || shape == SHAPE_THERMATRON || shape == SHAPE_CHEDITRON || shape == SHAPE_ANDROID) &&
        oneIn(3)) {
        Attack *attack = static_cast<Attack *>(firstProcess(item->id, ATTACK_TYPE));
        if (attack != nullptr) {
            attack->startFeint();
        }
    }
}

/* An NPC hit (takeHit): the bosses' shields take it first; a dead NPC takes
 * none (but No Regret's frozen ones). The NPC turns on the player, the
 * Silencer's shield may take the hit, an immortal NPC loses nothing, and
 * the hit points go down to its death or its hurt. Returns whether it was
 * hurt: not when the Silencer's shield took the hit or Vargas teleported. */
bool npcTakesHit(Item *item, u16 hitter, u8 direction, i16 damage, u16 type, ItemPoint *point)
{
    ItemPoint here;
    Npc npc(item->id);
    const char facing = npc.direction();
    /* A hit without a point (receiveHit's, the usecode's) is at the NPC:
     * the original's small shields sparked at the null pointer (No
     * Regret's sets it). */
    if (point == nullptr) {
        here = npc.point();
        point = &here;
    }
    const u16 shape = npc.shape();
    if (TAKE_HIT_NO_REGRET) {
        if (shape == SHAPE_BOSS_ROBOT && (bossShield != 0)) {
            bossRobotShieldTakes(item, damage, here);
            damage = 0;
        }
    } else if (shape == SHAPE_VARGAS && (bossShield != 0)) {
        vargasShieldTakes(npc, damage);
        return false;
    }
    /* A dead NPC takes no more hits, but No Regret's frozen ones (they shatter). */
    if ((npc.isDead() != 0U) && (!TAKE_HIT_NO_REGRET || shape != SHAPE_FROZEN)) {
        return true;
    }
    bool hurt = true;
    const u32 now = TAKE_HIT_NO_REGRET ? systemTimer->time() : systemTimer->ticks;
    npc.setHitTime(now);
    if (!item->isControlled() && !item->isSilencer()) {
        turnOnAttacker(item, npc, hitter);
    }
    if (item->isSilencer() && silencerShieldTakes(npc, type, *point)) {
        damage = 0;
        hurt = false;
    }
    if (npc.isImmortal() != 0U) {
        damage = 0;
    }
    if (item->isControlled() && damage != 0) {
        updateStatus(STATUS_HEALTH);
    }
    i16 hp = npc.hp();
    hp -= damage;
    if (hp < 0) {
        hp = 0;
    }
    npc.setHp(hp);
    const NpcHit hit = {shape, facing, direction, damage, type, point, isOrganic(item)};
    if (TAKE_HIT_NO_REGRET) {
        if (hp == 0) {
            noRegretDeath(item, npc, hit);
        } else {
            noRegretHurt(item, npc, hit);
        }
    } else if (hp == 0) {
        noRemorseDeath(item, npc, hit);
    } else if (damage != 0) {
        noRemorseHurt(item, npc, hit);
    }
    return hurt;
}

/* The push on one axis of a hit from direction: table's for the 16
 * directions; for none (NO_DIRECTION: the hit at the item's x and y),
 * noDirection, the word each game's original reads past its table. A larger
 * direction, which only the usecode could give (the games' usecode gives
 * none), is taken as none. */
i16 hurlOf(const i16 (&table)[16], u8 direction, i16 noDirection)
{
    if (direction >= NO_DIRECTION) {
        return noDirection;
    }
    return table[direction];
}

/* An item that is no NPC takes the hit itself (Item::gotHit: the fire type
 * and the damage as its force), and one with weight that a grenade or an
 * explosion hit is hurled away from the hit, 10 to 25 times as far as
 * hurlOf says for its direction. */
void thingTakesHit(Item *item, u8 direction, i16 damage, u16 type)
{
    i16 force = type << 8;
    force |= damage;
    u8 handled;
    item->gotHit(0x4000, force, &handled);
    if ((handled == 0U) || !item->isNpc()) {
        const i16 weight = item->weight();
        if ((weight != 0) && (type == FIRE_EXPLOSION || type == FIRE_GL_303)) {
            /* BCC computes the arguments right to left: the y's random
             * number before the x's (No Remorse's 1128:103e). C++ leaves
             * the order to the compiler (Clang takes the x's first), so it
             * is written out. */
            const i16 hurlDy = static_cast<i16>(hurlOf(hurlY, direction, HURL_Y_NO_DIRECTION) * randomBetween(10, 25));
            const i16 hurlDx = static_cast<i16>(hurlOf(hurlX, direction, HURL_X_NO_DIRECTION) * randomBetween(10, 25));
            hurl(item, hurlDx, hurlDy, 0, 2);
        }
    }
}
}

bool takeHit(Item *item, u16 hitter, u8 direction, i16 damage, u16 type, ItemPoint *point)
{
    damage = scaledDamage(item, damage, type);
    if (!item->isNpc()) {
        thingTakesHit(item, direction, damage, type);
        return true;
    }
    return npcTakesHit(item, hitter, direction, damage, type, point);
}
