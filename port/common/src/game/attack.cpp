/* ATTACK.C (Code37, No Regret's 1120, No Remorse's 1108; the addresses are No
 * Regret's): an NPC's attack. With a tactic it runs that tactic's script from
 * static\combat.dat, a byte command and its word arguments each run; without
 * one it runs the generic attack. The games' differences are each game's values
 * (attack_game.h). */
#include <cstdlib>
#include "gamestate.h"
#include "attack.h"
#include "activity.h"
#include "animcach.h"
#include "animprim.h"
#include "control.h"
#include "crusader.h"
#include "distance.h"
#include "dtable.h"
#include "explode.h"
#include "flex.h"
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

/* The commands of a tactic's script, each with its word arguments: a value is a
 * script word or, at DATA_BASE and above, the data word it names (nextValue);
 * "at" names a data word (nextWord). An offset is from the tactic's entry.
 * Block 0 runs first, then block 1 over and over. The tactics of the game never
 * use the commands marked unused. */
enum {
    TACTIC_SET_UNKNOWN59 = 0x81,   /* value (unused) */
    TACTIC_CLEAR_UNKNOWN59 = 0x82, /* (unused) */
    TACTIC_SET_TARGET = 0x84,      /* npc */
    TACTIC_WALK = 0x85,            /* a step ahead */
    TACTIC_RUN = 0x86,             /* (unused) */
    TACTIC_RETREAT = 0x87,         /* (unused) */
    TACTIC_TURN_LEFT = 0x88,       /* a quarter turn */
    TACTIC_TURN_RIGHT = 0x89,
    TACTIC_FIRE_SMALL = 0x8a, /* when the target is in the line of fire */
    TACTIC_FIRE_LARGE = 0x8b, /* (unused) */
    TACTIC_STAND = 0x8c,      /* (unused) */
    TACTIC_WALK_HOME = 0x8d,
    TACTIC_WALK_TO_TARGET = 0x8e, /* (unused) */
    TACTIC_WALK_HALFWAY = 0x8f,   /* to the point between the NPC and the target */
    TACTIC_KNEEL_FIRE = 0x92,     /* (unused) */
    TACTIC_WAIT = 0x93,           /* ticks, or WAIT_RANDOM; divided by the difficulty */
    TACTIC_LOITER = 0x94,         /* value: a Loiter */
    TACTIC_FACE_TARGET = 0x95,
    TACTIC_SET_ACTIVITY = 0x96,         /* activity (unused) */
    TACTIC_SET_TACTIC = 0x97,           /* tactic (unused) */
    TACTIC_MOVE_HOME = 0x98,            /* puts the NPC home at once, facing homeDirection (unused) */
    TACTIC_STOP = 0x99,                 /* ends the attack (unused) */
    TACTIC_JUMP_IF_WITHIN_480 = 0x9a,   /* offset: when the target is 480 or nearer */
    TACTIC_JUMP_IF_BEYOND_160 = 0x9b,   /* offset (unused) */
    TACTIC_JUMP_UNLESS_IN_RANGE = 0x9c, /* offset: unless the target is in the line of fire (fireDistance) */
    TACTIC_JUMP_IF_IN_RANGE = 0x9d,     /* offset */
    TACTIC_JUMP_UNLESS_ONE_IN = 0x9e,   /* n offset: unless a 1 in n chance comes up (unused) */
    TACTIC_LOOP = 0x9f,                 /* count: runs up to TACTIC_END_LOOP count times */
    TACTIC_WALK_TO_WAYPOINT = 0xa6,     /* frame: to the waypoint (shape 33Ah) of that frame and of the NPC's
                                           waypointGroup */
    TACTIC_FACE_NORTH = 0xa7,           /* (unused) */
    TACTIC_FACE_SOUTH = 0xa8,           /* (unused) */
    TACTIC_FACE_EAST = 0xa9,
    TACTIC_FACE_WEST = 0xaa,
    TACTIC_FACE_NORTHEAST = 0xab,       /* (unused) */
    TACTIC_FACE_NORTHWEST = 0xac,       /* (unused) */
    TACTIC_FACE_SOUTHEAST = 0xad,       /* (unused) */
    TACTIC_FACE_SOUTHWEST = 0xae,       /* (unused) */
    TACTIC_SET_DATA = 0xaf,             /* value at: the script has the value first (unused) */
    TACTIC_ADD_DATA = 0xb0,             /* at value (unused) */
    TACTIC_SUB_DATA = 0xb1,             /* at value (unused) */
    TACTIC_MUL_DATA = 0xb2,             /* at value (unused) */
    TACTIC_DIV_DATA = 0xb3,             /* at value (unused) */
    TACTIC_GET_DIRECTION = 0xb4,        /* at (unused) */
    TACTIC_SET_DIRECTION = 0xb5,        /* direction (unused) */
    TACTIC_GET_TARGET_DIRECTION = 0xb6, /* at (unused) */
    TACTIC_KNEELING_RETREAT = 0xb7,     /* (unused) */
    TACTIC_KNEELING_ADVANCE = 0xb8,     /* (unused) */
    TACTIC_KNEELING_SLIDE_LEFT = 0xb9,  /* (unused) */
    TACTIC_JUMP = 0xc0,                 /* offset */
    TACTIC_END_LOOP = 0xc1,
    TACTIC_RESTART = 0xff /* the end of a block: block 1 runs from its start */
};
/* TACTIC_WAIT's argument for 20 to 69 ticks at random. */
constexpr int WAIT_RANDOM = 0x7d01;

u8 &tacticsLoaded = gameGlobal<u8>(0);
namespace {
CombatTactic *(&tactics)[32] = gameGlobal<CombatTactic *[32]>();
}

Attack::~Attack() = default;

Attack::Attack(u16 ownerNpc) : Activity(ownerNpc)
{
    nameProcess(pid, "Attack");
    setType(ATTACK_TYPE);
    Npc self(item);
    feinted = 0;
    readyTime = 0;
    kneeling = 0;
    aggressive = 0;
    doubleDelay = 0;
    targetSighted = 0;
    sightedTime = 0;
    stoppedToFire = 0;
    feint = 0;
    greeted = 0;
    fromGuard = 0;
    lastSpoke = 0;
    speech = -1;
    lastReady = 0;
    readyDelay = 0;
    lastFire = 0;
    fireDelay = 0;
    justMoved = 0;
    lastSearch = 0;
    const u16 weapon = self.activeWeapon();
    u16 shape;
    Npc actor(ownerNpc);
    const Item gun(weapon);
    if (gun.isValid()) {
        shape = gun.shape();
        switch (shape) {
        case SHAPE_UV_9:
        case SHAPE_GL_303:
        case SHAPE_AR_7:
            readyDelay = 0x3c;
            switch (controlledNpc.difficulty) {
            case 1:
                fireDelay = 0x78;
                break;
            case 2:
                fireDelay = 0x5a;
                break;
            default:
                if (itemShape[actor.id] == BOSS_SHAPE) {
                    fireDelay = BOSS_FIRE_DELAY;
                } else {
                    fireDelay = 0x3c;
                }
            }
            break;
        default:
            readyDelay = 0x1e;
            switch (controlledNpc.difficulty) {
            case 1:
                fireDelay = readyDelay;
                break;
            case 2:
                fireDelay = 0x14;
                break;
            case 3:
                fireDelay = 0xf;
                break;
            default:
                fireDelay = 0;
            }
        }
    }
    if (tacticsLoaded == 0U) {
        loadTactics();
    }
    homeDirection = actor.direction();
    setTactic(actor.combatTactic());
    if (npcs[self.id].attackTarget != controlledActor && controlledActor == 1) {
        self.setAttackTarget(1);
    }
    npcs[self.id].aimedBurst = 0;
    if (hasAnim(self.shape(), ANIM_STAND)) {
        setFirstFrame(self.id, ANIM_STAND);
        if (STANDING_PUTS_WEAPON_AWAY) {
            npcs[self.id].weaponDrawn = 0;
        }
    }
    eventMask |= EVENT_RELATED_DIED;
}

bool Attack::readyToSpeak()
{
    const u32 now = systemTimer->ticks;
    if (lastSpoke != 0U) {
        if (lastSpoke + 480 <= now) {
            lastSpoke = now;
            return true;
        }
        return false;
    }
    lastSpoke = now;
    return true;
}

bool Attack::stillReady()
{
    const u32 now = systemTimer->ticks;
    u32 delay = 120;
    if (doubleDelay != 0U) {
        delay *= 2;
    }
    if (readyTime != 0U) {
        lastReady = readyTime;
        if (readyTime + delay >= now) {
            return true;
        }
    }
    readyTime = 0;
    doubleDelay = 0;
    return false;
}

i16 Attack::nextValue()
{
    u16 value;
    const u16 at = position;
    position += 2;
    value = *reinterpret_cast<u16 *>(script + at);
    if (value >= DATA_BASE) {
        value -= DATA_BASE;
        return data[value];
    }
    return value;
}

u16 Attack::nextWord()
{
    u16 value;
    const u16 at = position;
    position += 2;
    value = *reinterpret_cast<u16 *>(script + at);
    return value;
}

void Attack::setData(u16 at, u16 value)
{
    if (at >= DATA_BASE) {
        at -= DATA_BASE;
        data[at] = value;
    } else {
        halt();
    }
}

i16 Attack::getData(u16 at)
{
    if (at >= DATA_BASE) {
        at -= DATA_BASE;
    } else {
        halt();
    }
    return data[at];
}

void Attack::setBlock(i16 newBlock)
{
    this->block = newBlock;
    switch (this->block) {
    case 0:
        blockStart = tactic->blocks[0];
        break;
    case 1:
        blockStart = tactic->blocks[1];
        break;
    case 2:
        blockStart = tactic->blocks[2];
        break;
    case 3:
        blockStart = tactic->blocks[3];
        break;
    }
    position = blockStart;
}

void Attack::setTactic(i16 number)
{
    if (number >= 32) {
        halt();
    }
    if (tactics[number] == nullptr) {
        halt();
    }
    tacticNumber = number;
    tactic = tactics[tacticNumber];
    script = reinterpret_cast<u8 *>(tactic);
    setBlock(0);
}

void Attack::tacticLoaded()
{
    if (tacticsLoaded == 0U) {
        loadTactics();
    }
    tactic = tactics[tacticNumber];
    script = reinterpret_cast<u8 *>(tactic);
}

void freeTactics()
{
    for (CombatTactic *&tactic : tactics) {
        delete[] reinterpret_cast<u8 *>(tactic);
        tactic = nullptr;
    }
    tacticsLoaded = 0;
}

void Attack::loadTactics()
{
    Flex flex("static\\combat.dat", 0, -1);
    i16 i;
    for (i = 0; i < 32; i++) {
        const s32 size = flex.objectSize(i);
        if (size != 0) {
            tactics[i] = reinterpret_cast<CombatTactic *>(new u8[size]);
            if (tactics[i] == nullptr) {
                allocationFailure();
            }
            flex.readObject(i, tactics[i], -1, 0);
        } else {
            tactics[i] = nullptr;
        }
    }
    tacticsLoaded = 1;
}

namespace {
/* No Remorse's: The lines an NPC may say when it first gets ready to fire, by its shape
 * (attack): 2FDh and 319h, 384h, 371h, 4D1h and 528h, 1B4h. */
i16 readyLines2fd[6] = {0x15, 0x78, 0x80, 0x83, 0xdc, 0xdd};
i16 readyLines384[6] = {0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xe7};
i16 readyLines371[4] = {0xfc, 0xfd, 0xfe, 0xc8};
i16 readyLines4d1[4] = {0xcc, 0xcd, 0xce, 0xcf};
i16 readyLines1b4[3] = {0xc7, 0xca, 0xc9};
/* And later, at most every 480 ticks (readyToSpeak): 2DFh, 383h. */
i16 lines2df[3] = {0x82, 0x84, 0x85};
i16 lines383[5] = {0x9b, 0x9c, 0x9d, 0x9e, 0x9f};
}

namespace {
/* The attack's target: the controlled NPC (the Silencer when there is
 * none); No Regret's keeps a crawling spider it aims at. */
void aimAtPlayer(Npc &self)
{
    if (ATTACK_NO_REGRET) {
        const Npc target(npcs[self.id].attackTarget);
        if (npcs[self.id].attackTarget == controlledActor ||
            (target.isValid() && !target.inVoid() && itemShape[target.id] == SHAPE_CRAWLING_SPIDER)) {
            return;
        }
    } else if (npcs[self.id].attackTarget == controlledActor) {
        return;
    }
    if (controlledActor == 0U) {
        self.setAttackTarget(1);
    } else {
        self.setAttackTarget(controlledActor);
    }
}

/* The target is gone: none, in the ethereal void, dead or off the screen;
 * No Regret's crawling spider never is. */
bool targetGone(Npc &target)
{
    if (!ATTACK_NO_REGRET) {
        return !target.isValid() || target.z() == ETHEREAL_Z || (target.isDead() != 0U) || !target.isOnScreen();
    }
    if (!target.isValid() || target.z() == ETHEREAL_Z ||
        (itemShape[target.id] != SHAPE_CRAWLING_SPIDER && (target.isDead() != 0U)) || !target.isOnScreen()) {
        return !target.isValid() || itemShape[target.id] != SHAPE_CRAWLING_SPIDER;
    }
    return false;
}

/* The direction from the NPC to its target, of the directions of its stance
 * (info: 16, or 8, every other one of 16). */
u8 targetDirection(Npc &self, Npc &target, const AnimInfo &info)
{
    const i16 directions = info.sixteenDirections ? 16 : 8;
    u8 direction = closestDirection(itemX[self.id], itemY[self.id], itemX[target.id], itemY[target.id], directions, 0,
                                    directions - 1);
    if (!info.sixteenDirections) {
        direction *= 2;
    }
    return direction;
}
}

/* The NPC without a target: none any more, and it walks to a place near by
 * at random (not kneeling). */
void Attack::wander(Npc &self)
{
    self.setAttackTarget(0);
    if (kneeling != 0U) {
        return;
    }
    ItemPoint point = self.point();
    point.x += randomBetween(-0x200, 0x200);
    point.y += randomBetween(-0x200, 0x200);
    justMoved = 1;
    PathFinder *finder = new PathFinder(item, 0, point, 12, 0, 0x40, 1);
    finder->block(this);
}

/* The NPC readies its weapon. */
void Attack::drawWeapon(Npc &self, u32 now)
{
    readyTime = now;
    self.doAnim(ANIM_READY_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
}

/* An aimed burst, by chance (more often with doubleDelay): four times the
 * shots. */
void Attack::aimedBurstByChance(Npc &self)
{
    if (((doubleDelay != 0U) && oneIn(2)) || oneIn(5)) {
        npcs[self.id].aimedBurst = 1;
        shots *= 4;
    }
}

/* A burst's first shot: its animation starts and the attack waits for it,
 * listening to it while shots are left. */
void Attack::fireBurst(Npc &self, u32 now)
{
    const u16 startedPid = self.doAnim(ANIM_FIRE_SMALL, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
    lastFire = now;
    if (readyTime == 0U) {
        readyTime = now;
    }
    if (startedPid == 0U) {
        return;
    }
    Process *process = findProcess(startedPid);
    process->block(this);
    if (--shots != 0) {
        process->listenTo(this->pid);
    }
}

/* No Remorse's: whether the NPC's line still plays (it fires when the line
 * is over). */
bool Attack::stillSpeaking(Npc &self)
{
    if (speech == -1) {
        return false;
    }
    if (self.isSoundPlaying(speech) != 0U) {
        return true;
    }
    speech = -1;
    return false;
}

/* No Remorse's: when it first gets ready to fire, one time in three, the
 * NPC says a line of its shape's (2FDh and 319h, 384h, 371h, 4D1h and 528h,
 * 1B4h); later, at most every 480 ticks (readyToSpeak), 2DFh's and 383h's.
 * Returns whether it speaks. */
bool Attack::speakBeforeFiring(Npc &self, u16 shape)
{
    i16 line = -1;
    if (greeted == 0U) {
        greeted = 1;
        if (!oneIn(3)) {
            return false;
        }
        switch (shape) {
        case 0x2fd:
        case 0x319:
            line = readyLines2fd[borlandRandom(6)];
            break;
        case 0x384:
            line = readyLines384[borlandRandom(6)];
            break;
        case 0x371:
            line = readyLines371[borlandRandom(4)];
            break;
        case 0x4d1:
        case 0x528:
            line = readyLines4d1[borlandRandom(4)];
            break;
        case 0x1b4:
            line = readyLines1b4[borlandRandom(3)];
            break;
        default:
            break;
        }
    } else if (readyToSpeak()) {
        switch (shape) {
        case 0x2df:
            line = lines2df[borlandRandom(3)];
            break;
        case 0x383:
            line = lines383[borlandRandom(5)];
            break;
        default:
            break;
        }
    }
    if (line == -1) {
        return false;
    }
    speech = line;
    self.playSound(line);
    return true;
}

/* No Regret's feint: the NPC walks and retreats, or readies and unreadies
 * its weapon, by chance a few times more, and attacks when that is done
 * (endFeint). */
void Attack::feintNoRegret(Npc &self)
{
    /* Both cases of the switch set it. */
    u16 last = 0;
    u16 waitPid = 0;
    switch (borlandRandom(2)) {
    case 0:
        self.doAnim(ANIM_WALK, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
        self.doAnim(ANIM_RETREAT_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
        last = self.doAnim(ANIM_WALK, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
        if (!oneIn(3)) {
            waitPid = self.doAnim(ANIM_RETREAT_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
            self.doAnim(ANIM_WALK, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
            self.doAnim(ANIM_RETREAT_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
        }
        break;
    case 1: {
        const u8 drawn = self.isWeaponDrawn();
        const u16 first = (drawn != 0U) ? ANIM_UNREADY_SMALL : ANIM_READY_SMALL;
        const u16 second = (drawn != 0U) ? ANIM_READY_SMALL : ANIM_UNREADY_SMALL;
        self.doAnim(first, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
        self.doAnim(second, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
        last = self.doAnim(first, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
        if (!oneIn(3)) {
            waitPid = self.doAnim(second, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
            self.doAnim(first, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
            self.doAnim(second, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
        }
        break;
    }
    default:
        break;
    }
    Process *wait = findProcess((waitPid != 0U) ? waitPid : last);
    wait->block(this);
    endFeint();
}

/* No Remorse's feint, until feintEnd (then the NPC attacks as
 * ACTIVITY_ATTACK): in the combat stance, one time in three it turns at
 * random, or fires a burst when it faces its target with the weapon out;
 * else it readies or unreadies the weapon. Returns whether it did. */
bool Attack::feintNoRemorse(Npc &self, Npc &target, const Item &gun, const AnimInfo &info, u16 stance, u32 now)
{
    if (feintEnd < now) {
        feint = 0;
        self.setCurrentActivity(ACTIVITY_ATTACK);
        return false;
    }
    if (stance != ANIM_COMBAT_STAND_SMALL) {
        return false;
    }
    if (oneIn(3)) {
        if (oneIn(3)) {
            turnNpc(item, borlandRandom(16), 1, static_cast<u8>(self.isWeaponDrawn() == 0U));
            return true;
        }
        if (self.isWeaponDrawn() != 0U) {
            const u8 direction = targetDirection(self, target, info);
            if (self.direction() != direction || stillSpeaking(self)) {
                return true;
            }
            shots = burstSize(gun.shape());
            if (shots >= 3) {
                aimedBurstByChance(self);
            } else {
                shots = 1;
            }
            fireBurst(self, now);
        }
        return true;
    }
    const i16 anim = self.lastAnimSet();
    if (anim == ANIM_UNREADY_SMALL || anim == ANIM_UNREADY_LARGE) {
        self.doAnim(ANIM_READY_SMALL, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
    } else {
        self.doAnim(ANIM_UNREADY_SMALL, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
    }
    return true;
}

/* The NPC can fire at the target it faces: No Regret's may cry out first
 * (one time in three, or after an interval when hurt), No Remorse's says
 * its line (speakBeforeFiring); the weapon comes out if it is not, and the
 * burst comes once the ready and fire delays are over. */
void Attack::fireWhenReady(Npc &self, u16 shape, u16 weapon, Item &gun, u32 now)
{
    if (ATTACK_NO_REGRET) {
        if (self.isHurt() == 0U) {
            if (oneIn(3)) {
                self.playHurtSound();
            }
        } else if (intervalPassed()) {
            self.playHurtSound();
        }
        if (self.isWeaponDrawn() == 0U) {
            drawWeapon(self, now);
            return;
        }
    } else {
        if (speakBeforeFiring(self, shape)) {
            return;
        }
        if (self.isWeaponDrawn() == 0U) {
            drawWeapon(self, now);
            return;
        }
        if (stillSpeaking(self)) {
            return;
        }
    }
    const u32 sinceReady = now - lastReady;
    if (sinceReady <= static_cast<u32>(readyDelay)) {
        if (sinceReady != 0U) {
            setTimeout(readyDelay - sinceReady);
        }
        return;
    }
    const u32 sinceFire = now - lastFire;
    if (sinceFire <= static_cast<u32>(fireDelay)) {
        setTimeout(fireDelay - sinceFire);
        return;
    }
    if (weapon != 0U) {
        gun = Item(weapon);
        shots = burstSize(gun.shape());
        if (shots >= 3) {
            aimedBurstByChance(self);
        }
    } else {
        shots = 1;
    }
    fireBurst(self, now);
}

/* The NPC cannot fire at the target it faces: it walks to it (not
 * kneeling) or waits; No Regret's, every 90 ticks (searchDue), looks for a
 * crawling spider near by to shoot at instead, and turns from a spider back
 * to the controlled NPC. */
void Attack::waitOrChase(Npc &self, Npc &target)
{
    if (!ATTACK_NO_REGRET) {
        if (kneeling == 0U) {
            chase();
            return;
        }
        setTimeout(15);
        return;
    }
    bool walk = false;
    bool found = false;
    if (itemShape[target.id] != SHAPE_CRAWLING_SPIDER && searchDue()) {
        AreaSearch search;
        search.init(itemX[self.id] - 400, itemY[self.id] - 400, itemX[self.id] + 400, itemY[self.id] + 400,
                    LoopScript(SCRIPT_SHAPE, SCRIPT_INT, SHAPE_CRAWLING_SPIDER, SCRIPT_EQUAL, SCRIPT_END), -1);
        if (search.isValid()) {
            const u16 other = search.id;
            const char toward = closestDirection(itemX[self.id], itemY[self.id], itemX[other], itemY[other], 16, 0, 15);
            if (fireDistance(&self, search.id, toward, 0, 0, 0) != 0U) {
                self.setAttackTarget(search.id);
                found = true;
            } else {
                walk = true;
            }
        } else {
            walk = true;
        }
    } else if (itemShape[target.id] == SHAPE_CRAWLING_SPIDER) {
        self.setAttackTarget(controlledActor);
    } else {
        walk = true;
    }
    if (walk && (kneeling == 0U)) {
        chase();
        return;
    }
    if (!found) {
        setTimeout(15);
    }
}

/* The NPC does not face its target: ready when still ready from before,
 * stopped by its walk to fire, or with a line of fire (of 16 directions);
 * ready, it takes out its weapon or turns to the target; kneeling, it
 * turns; else it walks to it. */
void Attack::turnToTarget(Npc &self, Npc &target, const AnimInfo &info, u8 direction, u32 now)
{
    u8 ready;
    if (stillReady() || (stoppedToFire != 0U)) {
        sightTarget();
        stoppedToFire = 0;
        ready = 1;
    } else {
        if (!info.sixteenDirections) {
            direction = closestDirection(itemX[self.id], itemY[self.id], itemX[target.id], itemY[target.id], 16, 0, 15);
        }
        /* The distance's low byte (original). */
        ready = static_cast<u8>(fireDistance(&self, target.id, direction, 0, 0, 0));
        if (ready != 0U) {
            sightTarget();
        }
    }
    if (ATTACK_NO_REGRET) {
        if ((self.isWeaponDrawn() == 0U) && (ready != 0U)) {
            drawWeapon(self, now);
            return;
        }
        if ((ready != 0U) || (kneeling != 0U)) {
            turnNpc(item, direction, 1, static_cast<u8>(self.isWeaponDrawn() == 0U));
            return;
        }
    } else {
        const u8 drawnAtTurn = self.isWeaponDrawn();
        if ((drawnAtTurn == 0U) && (ready != 0U)) {
            drawWeapon(self, now);
            return;
        }
        if ((ready != 0U) || (kneeling != 0U)) {
            turnNpc(item, direction, 1, static_cast<u8>(drawnAtTurn == 0U));
            return;
        }
    }
    chase();
}

/* The attack without a tactic: the NPC finds its target (aimAtPlayer),
 * wanders when it has none, feints, walks to the target when it cannot see
 * it or it is too high, fires bursts when it faces it, else turns to it.
 * No Regret's feinted NPC attacks aggressively after; No Remorse's NPCs
 * speak before they fire. */
void Attack::attack()
{
    Npc self(item);
    const char facing = self.direction();
    const u32 now = systemTimer->ticks;
    const u16 shape = self.shape();
    if (self.isBusy()) {
        return;
    }
    if (ATTACK_NO_REGRET && feinted != 0U) {
        feinted = 0;
        self.setActivity(ACTIVITY_AGGRESSIVE_ATTACK);
        return;
    }
    const u16 weapon = self.activeWeapon();
    Item gun(weapon);
    aimAtPlayer(self);
    Npc target(npcs[self.id].attackTarget);
    if (targetGone(target)) {
        wander(self);
        return;
    }
    const u16 stance = (self.isInCombat() != 0U) ? ANIM_COMBAT_STAND_SMALL : ANIM_STAND;
    const AnimInfo info = animInfo(shape, stance);
    if (feint != 0U) {
        if (ATTACK_NO_REGRET) {
            feintNoRegret(self);
            return;
        }
        if (feintNoRemorse(self, target, gun, info, stance, now)) {
            return;
        }
    }
    const u8 direction = targetDirection(self, target, info);
    const i16 distance = mapDistance(itemX[self.id], itemY[self.id], itemZ[self.id], itemX[target.id], itemY[target.id],
                                     itemZ[target.id]);
    const i16 selfZ = itemZ[self.id];
    const i16 targetZ = itemZ[target.id];
    const i16 height = abs(selfZ - targetZ) * 2;
    const bool tooHigh = height >= distance;
    const bool onScreen = self.isMostlyOnScreen();
    /* No Regret's kneeling NPC fires on from off the screen and from below. */
    if (ATTACK_NO_REGRET ? ((kneeling == 0U) && (!onScreen || tooHigh))
                         : (((kneeling == 0U) && !onScreen) || tooHigh)) {
        chase();
        return;
    }
    if (direction != facing) {
        turnToTarget(self, target, info, direction, now);
        return;
    }
    const i16 roll = borlandRandom(10);
    /* It acts when it just moved, is still ready, its dodge is over, or by
     * the roll: 3 in 10, aggressive 4. */
    const bool acts = onScreen && ((justMoved != 0U) || stillReady() || dodgeOver(&self) || roll < 3 ||
                                   ((aggressive != 0U) && roll < 4));
    if (!acts) {
        setTimeout(20);
        return;
    }
    justMoved = 0;
    const u8 ready = self.recentlyHit() ? 1 : canFire(static_cast<char>(direction));
    if ((targetSighted != 0U) && (oneIn(5) || sightExpired())) {
        targetSighted = 0;
    }
    if (ready != 0U) {
        fireWhenReady(self, shape, weapon, gun, now);
    } else {
        waitOrChase(self, target);
    }
}

namespace {
/* The animation a tactic's command starts (the walk, the run, the
 * retreat, the kneeling fire, the stand, the kneeling moves); ANIMS for
 * another command. */
i16 tacticAnim(i16 command)
{
    switch (command) {
    case TACTIC_WALK:
        return ANIM_WALK;
    case TACTIC_RUN:
        return ANIM_RUN;
    case TACTIC_RETREAT:
        return ANIM_RETREAT_SMALL;
    case TACTIC_KNEEL_FIRE:
        return ANIM_KNEEL_FIRE;
    case TACTIC_STAND:
        return ANIM_STAND;
    case TACTIC_KNEELING_RETREAT:
        return ANIM_KNEELING_RETREAT;
    case TACTIC_KNEELING_ADVANCE:
        return ANIM_KNEELING_ADVANCE;
    case TACTIC_KNEELING_SLIDE_LEFT:
        return ANIM_KNEELING_SLIDE_LEFT;
    default:
        return ANIMS;
    }
}

/* The direction a tactic's command faces; -1 for another command. */
i16 tacticFacing(i16 command)
{
    switch (command) {
    case TACTIC_FACE_NORTH:
        return DIRECTION_NORTH;
    case TACTIC_FACE_NORTHEAST:
        return DIRECTION_NORTHEAST;
    case TACTIC_FACE_EAST:
        return DIRECTION_EAST;
    case TACTIC_FACE_SOUTHEAST:
        return DIRECTION_SOUTHEAST;
    case TACTIC_FACE_SOUTH:
        return DIRECTION_SOUTH;
    case TACTIC_FACE_SOUTHWEST:
        return DIRECTION_SOUTHWEST;
    case TACTIC_FACE_WEST:
        return DIRECTION_WEST;
    case TACTIC_FACE_NORTHWEST:
        return DIRECTION_NORTHWEST;
    default:
        return -1;
    }
}

/* The distance from the NPC to the target. */
i16 targetDistance(Npc &self, Npc &target)
{
    return mapDistance(itemX[self.id], itemY[self.id], itemZ[self.id], itemX[target.id], itemY[target.id],
                       itemZ[target.id]);
}
}

/* A tactic's jump: to the offset of its next word when jump, else past
 * it. */
void Attack::jumpIf(bool jump)
{
    if (jump) {
        position = nextValue();
    } else {
        nextValue();
    }
}

/* The NPC fires with anim when the target is in its line of fire (the
 * distance's low byte, original). */
void Attack::fireIfInRange(Npc &self, Npc &target, i16 anim)
{
    const u8 inRange = fireDistance(&self, target.id, self.direction(), 0, 0, 0);
    if (inRange != 0U) {
        self.doAnim(anim, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
    }
}

/* The NPC walks to the waypoint (shape 33Ah) of frame near it (512 either
 * way) of its waypoint group (any, without one). */
void Attack::walkToWaypoint(Npc &self, i16 frame)
{
    AreaSearch search;
    const i16 quality = self.waypointGroup();
    search.init(itemX[self.id] - 0x200, itemY[self.id] - 0x200, itemX[self.id] + 0x200, itemY[self.id] + 0x200,
                LoopScript(SCRIPT_SHAPE, SCRIPT_INT, 0x33a, SCRIPT_EQUAL, SCRIPT_END), -1);
    while (search.isValid()) {
        if (search.frame() == static_cast<ui16>(frame) &&
            ((quality == 0) || search.qualityLow() == static_cast<ui16>(quality))) {
            PathFinder *finder = new PathFinder(item, search.id, 100, 0, 0x40, 1);
            finder->block(this);
            return;
        }
        search.next();
    }
}

/* The NPC walks halfway to the target. */
void Attack::walkHalfway(Npc &self, Npc &target)
{
    const i16 x = itemX[self.id];
    const i16 y = itemY[self.id];
    const i16 dx = x - static_cast<i16>(itemX[target.id]);
    const i16 dy = y - static_cast<i16>(itemY[target.id]);
    ItemPoint point;
    point.x = x - (dx >> 1);
    point.y = y - (dy >> 1);
    point.z = itemZ[self.id];
    PathFinder *finder = new PathFinder(item, 0, point, 12, 0, 0x40, 1);
    finder->block(this);
}

/* The attack's next step: the generic attack (tactic 0), or the next
 * command of the tactic's script (combat.dat), while the NPC is idle and
 * its target, the controlled NPC, lives. The commands that start an
 * animation (tacticAnim) or face a way (tacticFacing) are tables; the
 * others walk, turn, fire, wait, keep data words, and jump. */
void Attack::run()
{
    if (tacticNumber == 0) {
        attack();
        return;
    }
    Npc self(item);
    Npc target(controlledNpcNumber());
    if (self.isBusy() || (target.isDead() != 0U)) {
        return;
    }
    i16 stance;
    if (self.isInCombat() != 0U) {
        stance = self.usesSmallAnims() ? ANIM_COMBAT_STAND_SMALL : ANIM_COMBAT_STAND_LARGE;
    } else {
        stance = ANIM_STAND;
    }
    const AnimInfo info = animInfo(itemShape[self.id], stance);
    const i16 directions = info.sixteenDirections ? 16 : 8;
    const i16 command = script[position++];
    const i16 anim = tacticAnim(command);
    if (anim != ANIMS) {
        self.doAnim(anim, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
        return;
    }
    const i16 facing = tacticFacing(command);
    if (facing != -1) {
        turnNpc(item, facing, 1, 1);
        return;
    }
    u16 at;
    switch (command) {
    case TACTIC_SET_UNKNOWN59:
        unknown59 = nextValue();
        return;
    case TACTIC_CLEAR_UNKNOWN59:
        unknown59 = 0;
        return;
    case TACTIC_SET_TARGET:
        self.setAttackTarget(nextValue());
        return;
    case TACTIC_WALK_HOME: {
        PathFinder *finder = new PathFinder(item, 0, self.home(), 100, 0, 0x40, 1);
        finder->block(this);
        return;
    }
    case TACTIC_WALK_TO_WAYPOINT:
        walkToWaypoint(self, nextValue());
        return;
    case TACTIC_WALK_TO_TARGET: {
        PathFinder *finder = new PathFinder(item, target.id, 12, 0, 0x40, 1);
        finder->block(this);
        return;
    }
    case TACTIC_WALK_HALFWAY:
        walkHalfway(self, target);
        return;
    case TACTIC_MOVE_HOME: {
        self.setDirection(homeDirection);
        self.push();
        const ItemPoint home = self.home();
        self.popToCoords(home.x, home.y, home.z);
        return;
    }
    case TACTIC_TURN_LEFT:
        turnNpc(item, directions == 8 ? (self.direction() - 2) & 7 : (self.direction() - 4) & 0xf, 1, 1);
        return;
    case TACTIC_TURN_RIGHT:
        turnNpc(item, directions == 8 ? (self.direction() + 2) & 7 : (self.direction() + 4) & 0xf, 1, 1);
        return;
    case TACTIC_FACE_TARGET:
        turnNpc(item,
                closestDirection(itemX[self.id], itemY[self.id], itemX[target.id], itemY[target.id], directions, 0,
                                 directions - 1),
                1, 1);
        return;
    case TACTIC_FIRE_SMALL:
        fireIfInRange(self, target, ANIM_FIRE_SMALL);
        return;
    case TACTIC_FIRE_LARGE:
        fireIfInRange(self, target, ANIM_FIRE_LARGE);
        return;
    case TACTIC_LOITER: {
        Loiter *loiter = new Loiter(item, nextValue());
        loiter->block(this);
        return;
    }
    case TACTIC_SET_DATA: {
        /* BCC computes the arguments right to left: the value before the
         * word (1120:0d45). C++ leaves the order to the compiler (Clang
         * takes the word first), so it is written out. */
        const u16 value = nextValue();
        at = nextWord();
        setData(at, value);
        return;
    }
    case TACTIC_ADD_DATA:
        at = nextWord();
        setData(at, getData(at) + nextValue());
        return;
    case TACTIC_SUB_DATA:
        at = nextWord();
        setData(at, getData(at) - nextValue());
        return;
    case TACTIC_MUL_DATA:
        at = nextWord();
        setData(at, getData(at) * nextValue());
        return;
    case TACTIC_DIV_DATA:
        at = nextWord();
        setData(at, getData(at) / nextValue());
        return;
    case TACTIC_GET_DIRECTION:
        setData(nextWord(), self.direction());
        return;
    case TACTIC_SET_DIRECTION:
        self.setDirection(nextValue());
        return;
    case TACTIC_GET_TARGET_DIRECTION:
        setData(nextWord(), target.direction());
        return;
    case TACTIC_SET_ACTIVITY:
        self.setActivity(nextValue());
        return;
    case TACTIC_SET_TACTIC:
        setTactic(nextValue());
        return;
    case TACTIC_WAIT: {
        i16 ticks = nextValue();
        if (ticks == WAIT_RANDOM) {
            ticks = randomBelow(50) + 20;
        }
        setTimeout(ticks / controlledNpc.difficulty);
        return;
    }
    case TACTIC_STOP:
        terminate(0);
        return;
    case TACTIC_JUMP:
        position = nextValue();
        return;
    case TACTIC_RESTART:
        if (block == 0) {
            setBlock(1);
        }
        position = blockStart;
        return;
    case TACTIC_JUMP_IF_WITHIN_480:
        jumpIf(targetDistance(self, target) <= 480);
        return;
    case TACTIC_JUMP_IF_BEYOND_160:
        jumpIf(targetDistance(self, target) > 160);
        return;
    case TACTIC_JUMP_UNLESS_IN_RANGE:
        jumpIf(fireDistance(&self, target.id, self.direction(), 0, 0, 0) == 0U);
        return;
    case TACTIC_JUMP_IF_IN_RANGE:
        jumpIf(fireDistance(&self, target.id, self.direction(), 0, 0, 0) != 0U);
        return;
    case TACTIC_JUMP_UNLESS_ONE_IN: {
        const i16 chance = nextValue();
        jumpIf(!oneIn(chance));
        return;
    }
    case TACTIC_LOOP:
        loopCount = nextValue();
        loopStart = position;
        return;
    case TACTIC_END_LOOP:
        loopCount--;
        if (loopCount > 0) {
            position = loopStart;
        }
        return;
    default:
        return;
    }
}

u8 Attack::canFire(char direction)
{
    Npc self(item);
    if (stillReady()) {
        return 1;
    }
    if (targetSighted != 0U) {
        return 1;
    }
    return fireDistance(&self, npcs[self.id].attackTarget, direction, 0, 0, 0);
}

void Attack::chase()
{
    PathFinder *finder;
    const Npc self(item);
    doubleDelay = 0;
    targetSighted = 0;
    justMoved = 1;
    finder = new PathFinder(item, npcs[self.id].attackTarget, 12, 0, 0x40, 0);
    finder->block(this);
}

bool Attack::searchDue()
{
    const u32 now = systemTimer->ticks;
    if (lastSearch + 90 <= now) {
        lastSearch = now;
        return true;
    }
    return false;
}

void Attack::startFeint()
{
    if (!TIMED_FEINT && feinted != 0U) {
        return;
    }
    feint = 1;
    if (TIMED_FEINT) {
        feintEnd = static_cast<i16>(borlandRandom(10) + 8);
        feintEnd *= 60;
        feintEnd += systemTimer->ticks;
    }
}

/* self is not used; only its frame slot is left (original). */
void Attack::endFeint()
{
    const Npc self(item);
    feint = 0;
    feinted = 1;
}

/* When the shot it listens to ends, the NPC fires again until the burst is
 * over. */
void Attack::event(u32 event, EventData eventData)
{
    u16 startedPid;
    Process *process;
    Npc self(item);
    AnimInfo info;
    i16 directions;
    char direction;
    Npc target(npcs[self.id].attackTarget);
    i16 anim;
    switch (event) {
    case EVENT_RELATED_DIED: {
        const u32 now = systemTimer->ticks;
        process = findProcess(eventData.number());
        if ((target.isDead() != 0U) || (process->hasFlags(PROCESS_FAILED) != 0U)) {
            shots = 0;
            return;
        }
        anim = ANIM_FIRE_SMALL;
        info = animInfo(self.shape(), anim);
        if (info.sixteenDirections) {
            directions = 16;
        } else {
            directions = 8;
        }
        direction = closestDirection(itemX[self.id], itemY[self.id], itemX[target.id], itemY[target.id], directions, 0,
                                     directions - 1);
        startedPid = self.doAnim(anim, direction, 1, 0);
        lastFire = now;
        if (startedPid != 0U) {
            process = findProcess(startedPid);
            process->block(this);
            if (--shots != 0) {
                process->listenTo(this->pid);
                return;
            }
            npcs[self.id].aimedBurst = 0;
            if (self.shape() != SHAPE_THERMATRON && self.shape() != SHAPE_VETRON && !dodgeOver(&self) && oneIn(2)) {
                setTimeout(60);
            }
        }
    }
    }
}

void Attack::sightTarget()
{
    sightedTime = systemTimer->ticks;
    targetSighted = 1;
}

bool Attack::sightExpired() const
{
    const u32 now = systemTimer->ticks;
    u32 delay = 60;
    if (doubleDelay != 0U) {
        delay *= 2;
    }
    if (sightedTime + delay > now) {
        return false;
    }
    return true;
}

void Attack::setStoppedToFire()
{
    stoppedToFire = 1;
}
