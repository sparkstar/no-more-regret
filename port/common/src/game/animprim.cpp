/* ANIMPRIM.C (Code46, No Regret's 1168, No Remorse's 1150; the addresses are
 * No Regret's): an NPC's animation process, the games' differences as each
 * game's values (animprim_game.h). */
#include "animprim.h"
#include "animcach.h"
#include "anchor.h"
#include "boboboom.h"
#include "box.h"
#include "collide.h"
#include "control.h"
#include "crusader.h"
#include "distance.h"
#include "gravity.h"
#include "itemdata.h"
#include "itemfind.h"
#include "kernel.h"
#include "npc.h"
#include "systimer.h"
#include "type.h"
#include "shapes.h"

AnimPrimitive::AnimPrimitive(u16 npc, u16 newAnim, u8 newDirection, ItemPoint *newTargetPoint, u16 newTouchItem,
                             u16 newUnknown49)
{
    nameProcess(pid, "AnimPrimitive");
    setItem(npc);
    setType(ANIM_TYPE);
    suspend();
    this->anim = newAnim;
    this->targetPoint = newTargetPoint;
    this->direction = newDirection;
    started = 0;
    stayPut = 0;
    this->touchItem = newTouchItem;
    this->unknown49 = newUnknown49;
    blockedByItem = 0;
    status.value = 0;
    reachedTarget = 0;
    loopAtEnd = 1;
    failPending = 0;
    reticleHidden = 0;
    hasTarget = 0;
    eventMask |= EVENT_RELATED_DIED;
    if (this->targetPoint != nullptr) {
        saveState();
    }
    const Npc self(npc);
    info = animInfo(self.shape(), this->anim);
    frames = info.frames;
}

/* A move of the controlled NPC that ends with only two opposite parts of
 * its base standing on something pushes it off the edge. */
bool AnimPrimitive::canStep(ItemPoint from, ItemPoint to, u16 /*unused*/)
{
    Support support;
    Npc npc(item);
    const u16 shape = npc.shape();
    u16 lastAnim;
    if (targetPoint != nullptr) {
        testSupport(&support, shape, *targetPoint, 1, item);
    } else if (blockedByItem != 0U) {
        testSupport(&support, shape, from, 1, item);
    } else {
        testSupport(&support, shape, to, 1, item);
    }
    if (support.standing() == 0U) {
        if (targetPoint != nullptr) {
            status.bits.unsupported = 1;
            fail(0);
        } else if (LEDGE_HOLDS_CONTROLLED_NPC) {
            if (npc.isControlled()) {
                if (!support.only(SUPPORT_NORTH, SUPPORT_SOUTH) && !support.only(SUPPORT_SOUTH, SUPPORT_NORTH) &&
                    (!support.only(SUPPORT_EAST, SUPPORT_WEST)) && !support.only(SUPPORT_WEST, SUPPORT_EAST)) {
                    lastAnim = npc.lastAnimSet();
                    if (controlledActor == 1 && lastAnim != ANIM_JUMP_LANDING && lastAnim != ANIM_QUICK_JUMP &&
                        lastAnim != ANIM_COMBAT_ROLL_LEFT && lastAnim != ANIM_COMBAT_ROLL_RIGHT &&
                        lastAnim != ANIM_KNEEL_ROLL_LEFT && lastAnim != ANIM_KNEEL_ROLL_RIGHT &&
                        lastAnim != ANIM_ROLL_FORWARD) {
                        npc.doAnim(ANIM_JUMP_LANDING, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
                    }
                    hurl(&npc, to.x - from.x, to.y - from.y, to.z - from.z, 4);
                    goto done;
                }
                return false;
            }
            fall(&npc);
        } else {
            lastAnim = npc.lastAnimSet();
            if (lastAnim != ANIM_JUMP_LANDING && lastAnim != ANIM_QUICK_JUMP && lastAnim != ANIM_COMBAT_ROLL_LEFT &&
                lastAnim != ANIM_COMBAT_ROLL_RIGHT && lastAnim != ANIM_KNEEL_ROLL_LEFT &&
                lastAnim != ANIM_KNEEL_ROLL_RIGHT) {
                npc.doAnim(ANIM_JUMP_LANDING, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
            }
            hurl(&npc, to.x - from.x, to.y - from.y, to.z - from.z, 2);
        }
        /* The controlled NPC's hurl comes here too. */
    done:
        return true;
    }
    return false;
}

namespace {
/* A test move of anim (AnimPrimitive::init): the animation's process
 * toward direction, to point; nullptr for an animation the tests do not
 * have. */
AnimPrimitive *newTestAnim(i16 anim, u16 item, u8 direction, ItemPoint *point)
{
    switch (anim) {
    case ANIM_COMBAT_ROLL_LEFT:
        return new CombatRollLeftAnim(item, direction, point, 0);
    case ANIM_COMBAT_ROLL_RIGHT:
        return new CombatRollRightAnim(item, direction, point, 0);
    case ANIM_KNEEL_ROLL_LEFT:
        return new KneelRollLeftAnim(item, direction, point, 0);
    case ANIM_KNEEL_ROLL_RIGHT:
        return new KneelRollRightAnim(item, direction, point, 0);
    case ANIM_KNEELING_SLIDE_LEFT:
        return new KneelingSlideLeftAnim(item, direction, point, 0);
    case ANIM_KNEELING_SLIDE_RIGHT:
        return new KneelingSlideRightAnim(item, direction, point, 0);
    case ANIM_SLIDE_LEFT:
        return new SlideLeftAnim(item, direction, point, 0);
    case ANIM_SLIDE_RIGHT:
        return new SlideRightAnim(item, direction, point, 0);
    case ANIM_WALK:
        return new WalkAnim(item, direction, point, 0);
    case ANIM_RUN:
        return new RunAnim(item, direction, point, 0);
    case ANIM_COMBAT_RUN_SMALL:
        return new CombatRunSmallAnim(item, direction, point, 0);
    case ANIM_COMBAT_RUN_LARGE:
        return new CombatRunLargeAnim(item, direction, point, 0);
    case ANIM_START_RUN:
        return new StartRunAnim(item, direction, point, 0);
    case ANIM_START_RUN_SMALL:
        return new StartRunSmallAnim(item, direction, point, 0);
    case ANIM_START_RUN_LARGE:
        return new StartRunLargeAnim(item, direction, point, 0);
    case ANIM_ADVANCE_SMALL:
        return new AdvanceSmallAnim(item, direction, point, 0);
    case ANIM_ADVANCE_LARGE:
        return new AdvanceLargeAnim(item, direction, point, 0);
    case ANIM_RETREAT_SMALL:
        return new RetreatSmallAnim(item, direction, point, 0);
    case ANIM_RETREAT_LARGE:
        return new RetreatLargeAnim(item, direction, point, 0);
    case ANIM_KNEELING_RETREAT:
        return new KneelingRetreatAnim(item, direction, point, 0);
    case ANIM_KNEELING_ADVANCE:
        return new KneelingAdvanceAnim(item, direction, point, 0);
    case ANIM_ROLL_FORWARD:
        return new RollForwardAnim(item, direction, point, 0);
    default:
        return nullptr;
    }
}

/* A test move runs at once, waited on by the anchor: whether it ended
 * (went all its way). */
bool testEnded(AnimPrimitive *test, Process *anchor)
{
    if (test == nullptr) {
        halt();
    }
    test->block(anchor);
    test->run();
    AnimStatus testStatus;
    testStatus = anchor->result;
    return testStatus.bits.ended != 0U;
}

/* The point step from from toward toward. */
ItemPoint offsetPoint(const ItemPoint &from, char toward, i16 step)
{
    ItemPoint point;
    point.set(from.x + (directionX16[static_cast<int>(toward)] * step),
              from.y + (directionY16[static_cast<int>(toward)] * step), from.z);
    return point;
}

/* The first of the scores from first to before end that is more than 30h
 * (the comparison with the best so far that the original has never runs);
 * -1 for none. */
i16 firstFarScore(const i16 *scores, i16 first, i16 end)
{
    for (i16 i = first; i < end; i++) {
        if (scores[i] > 0x30) {
            return i;
        }
    }
    return -1;
}

/* The controlled NPC stands instead of a blocked walk or run: kneeling, in
 * the combat stance with the reticle shown when it acts, or at peace. */
u16 standInstead(Npc &npc)
{
    u16 stand;
    if (npc.isKneeling() != 0U) {
        stand = ANIM_KNEELING_SMALL;
    } else if (npc.isActing()) {
        showReticle();
        stand = ANIM_COMBAT_STAND_SMALL;
    } else {
        stand = ANIM_STAND;
    }
    return npcAnim(&npc, stand);
}
}

/* The controlled NPC's moves that are tried first, when nothing touched
 * its process (unknown77Bit5): walks, runs, rolls, advances and retreats;
 * No Regret's (BLOCKED_MOVE_SEARCH) slides, kneeling moves and the roll
 * forward too. */
bool AnimPrimitive::searchesBlockedMove(Npc &npc) const
{
    return npc.isControlled() && (targetPoint == nullptr) &&
           (anim == ANIM_WALK || anim == ANIM_RUN || anim == ANIM_KNEEL_ROLL_LEFT || anim == ANIM_KNEEL_ROLL_RIGHT ||
            anim == ANIM_COMBAT_ROLL_LEFT || anim == ANIM_COMBAT_ROLL_RIGHT || anim == ANIM_COMBAT_RUN_SMALL ||
            anim == ANIM_COMBAT_RUN_LARGE || anim == ANIM_START_RUN || anim == ANIM_START_RUN_SMALL ||
            anim == ANIM_START_RUN_LARGE || anim == ANIM_ADVANCE_SMALL || anim == ANIM_ADVANCE_LARGE ||
            anim == ANIM_RETREAT_SMALL || anim == ANIM_RETREAT_LARGE ||
            (BLOCKED_MOVE_SEARCH &&
             (anim == ANIM_KNEELING_RETREAT || anim == ANIM_KNEELING_ADVANCE || anim == ANIM_KNEELING_SLIDE_LEFT ||
              anim == ANIM_KNEELING_SLIDE_RIGHT || anim == ANIM_SLIDE_LEFT || anim == ANIM_SLIDE_RIGHT ||
              anim == ANIM_ROLL_FORWARD))) &&
           (npc.unknown77Bit5() == 0U);
}

/* A roll (No Regret's slides too) tested from the NPC's place; blocked, it
 * is tested from the points 16, 32 and 48 behind and ahead of it in turn
 * (where they are free), up to one that goes all its way, which the NPC
 * goes to. Else, the first point whose test got more than 30h: No Regret's
 * NPC moves there (else it takes the large stance), No Remorse's goes
 * there by the sweep (else it kneels). */
void AnimPrimitive::findRollRoom(Npc &npc, Process *anchor, ItemPoint from, u16 shape)
{
    i16 scores[10] = {};
    ItemPoint cells[10];
    HitSweep sweep;
    ItemPoint pt = npc.point();
    if (testEnded(newTestAnim(anim, item, direction, &pt), anchor)) {
        return;
    }
    for (i16 i = 1; i < 7; i++) {
        const char toward = ((i & 1) != 0) ? direction : static_cast<char>((direction + 8) & 0xf);
        pt = offsetPoint(from, toward, static_cast<i16>(i + 1) / 2 * 16);
        const ItemPoint cell = pt;
        sweep.reset();
        if (!sweep.step(shape, from, pt, MOVE_STANDS, item, 0)) {
            continue;
        }
        if (testEnded(newTestAnim(anim, item, direction, &pt), anchor)) {
            sweep.apply();
            return;
        }
        if (BLOCKED_MOVE_SEARCH) {
            scores[i] = mapDistance(cell.x, cell.y, pt.x, pt.y);
            cells[i] = cell;
        } else {
            scores[i] = mapDistance(from.x, from.y, pt.x, pt.y);
        }
    }
    const i16 best = firstFarScore(scores, 1, 7);
    if (BLOCKED_MOVE_SEARCH) {
        if (best != -1) {
            npc.moveTo(cells[best]);
        } else if (npc.isKneeling() != 0U) {
            anim = ANIM_KNEELING_LARGE;
        } else {
            anim = ANIM_COMBAT_STAND_LARGE;
        }
    } else if (best != -1) {
        const char toward = ((best & 1) != 0) ? direction : static_cast<char>((direction + 8) & 0xf);
        pt = offsetPoint(from, toward, static_cast<i16>(best + 1) / 2 * 16);
        sweep.step(shape, from, pt, MOVE_STANDS, item, 0);
        sweep.apply();
    } else {
        anim = ANIM_KNEELING_SMALL;
    }
}

/* A walk, a run, an advance or a retreat tested from the NPC's place;
 * blocked, it is tested from the points 16 to 80 to either side in turn
 * (where they are free), up to one that goes all its way, which the NPC
 * goes to. Else a burning NPC stops, No Regret's moves to the first point
 * whose test got more than 30h, and the NPC stands (standInstead). No
 * Remorse's anchor ends after. */
void AnimPrimitive::findWalkRoom(Npc &npc, Process *anchor, ItemPoint from, u16 shape)
{
    static constexpr char STEPS[10] = {0x10, 0x10, 0x20, 0x20, 0x30, 0x30, 0x40, 0x40, 0x50, 0x50};
    i16 scores[10] = {};
    ItemPoint cells[10];
    HitSweep sweep;
    ItemPoint pt = from;
    bool moved = testEnded(newTestAnim(anim, item, direction, &pt), anchor);
    for (i16 j = 0; j < 10 && !moved; j++) {
        const char toward = static_cast<char>((direction + (((j & 1) != 0) ? 4 : 0xc)) & 0xf);
        pt = offsetPoint(from, toward, STEPS[j]);
        const ItemPoint cell = pt;
        sweep.reset();
        if (!sweep.step(shape, from, pt, MOVE_STANDS, item, 0)) {
            continue;
        }
        if (testEnded(newTestAnim(anim, item, direction, &pt), anchor)) {
            if (!BLOCKED_MOVE_SEARCH) {
                pt = offsetPoint(from, toward, STEPS[j]);
            }
            sweep.apply();
            moved = true;
        } else if (BLOCKED_MOVE_SEARCH) {
            scores[j] = mapDistance(cell.x, cell.y, pt.x, pt.y);
            cells[j] = cell;
        }
    }
    if (!moved) {
        if (npc.shape() == SHAPE_BURNING) {
            anim = ANIMS;
        } else {
            const i16 best = BLOCKED_MOVE_SEARCH ? firstFarScore(scores, 0, 10) : -1;
            if (best != -1) {
                npc.moveTo(cells[best]);
            } else {
                anim = standInstead(npc);
            }
        }
    }
    if (!BLOCKED_MOVE_SEARCH && (anchor->hasFlags(PROCESS_DEAD) == 0U)) {
        anchor->terminate(0);
    }
}

/* The start of the animation: the controlled NPC's moves are tried first
 * (searchesBlockedMove): a test move runs at once, waited on by an anchor
 * process (No Regret's the anchor process, No Remorse's a test anchor of
 * its own, ended after the walks'), and a blocked one looks for room
 * (findRollRoom, findWalkRoom). Then the animation's information, its first
 * frame, and the NPC's last animation and combat stance. */
void AnimPrimitive::init()
{
    Npc npc(item);
    started = 1;
    turnTo(direction);
    npcs[npc.id].unknown77_4 = 0;
    if (searchesBlockedMove(npc)) {
        const ItemPoint from(itemX[npc.id], itemY[npc.id], itemZ[npc.id]);
        Process *anchor = BLOCKED_MOVE_SEARCH ? static_cast<Process *>(anchorProcess) : new TestAnchor;
        const u16 shape = npc.shape();
        if (anim == ANIM_KNEEL_ROLL_LEFT || anim == ANIM_KNEEL_ROLL_RIGHT || anim == ANIM_COMBAT_ROLL_LEFT ||
            anim == ANIM_COMBAT_ROLL_RIGHT ||
            (BLOCKED_MOVE_SEARCH && (anim == ANIM_KNEELING_SLIDE_LEFT || anim == ANIM_KNEELING_SLIDE_RIGHT ||
                                     anim == ANIM_SLIDE_LEFT || anim == ANIM_SLIDE_RIGHT))) {
            findRollRoom(npc, anchor, from, shape);
        } else {
            findWalkRoom(npc, anchor, from, shape);
        }
    }
    if (anim != ANIMS) {
        info = animInfo(npc.shape(), anim);
        frames = info.frames;
        startFrame();
        npc.setLastAnim(anim);
        if (npc.isControlled()) {
            if (npc.isActing()) {
                npc.setInCombat();
            } else {
                npc.clrInCombat();
            }
        }
    }
    npc.setDirection(direction);
}

namespace {
/* The NPC's frames go two at a time: the controlled NPC's at frame skip 3,
 * the others' from 2 or off the screen. */
bool skipsFrames(Npc &npc)
{
    if (npc.isControlled()) {
        return frameSkip == 3;
    }
    return frameSkip >= 2 || !npc.isOnScreen();
}
}

/* A two-step animation starts at its middle, or at its end or start for
 * the second step; a looping one goes on from its second frame (third when
 * the frames go two at a time). */
void AnimPrimitive::startFrame()
{
    Npc npc(item);
    if (info.twoStep) {
        if (npc.isFirstStep() != 0U) {
            if (info.looping) {
                npc.setAnimFrame(frames - 1);
            } else {
                npc.setAnimFrame(0);
            }
        } else {
            npc.setAnimFrame(frames >> 1);
        }
    } else if (info.looping) {
        if (npc.lastAnimSet() == static_cast<ui16>(anim)) {
            npc.setAnimFrame(skipsFrames(npc) ? 2 : 1);
        } else {
            npc.setAnimFrame(0);
        }
    } else {
        npc.setAnimFrame(0);
    }
}

void AnimPrimitive::event(u32 event, EventData /*data*/)
{
    switch (event) {
    case EVENT_RELATED_DIED:
        flags &= ~PROCESS_BLOCKED;
        break;
    }
}

/* UNCALLED: nothing calls it. */
void AnimPrimitive::saveState()
{
    Npc npc(item);
    savedFirstStep = npcs[npc.id].firstStep;
    savedLastAnim = npc.lastAnimSet();
    savedDirection = npc.direction();
    savedFlipped = npc.isFlipped();
    savedUnknown77_4 = npcs[npc.id].unknown77_4;
    savedAnimFrame = npcs[npc.id].animFrame;
}

void AnimPrimitive::restoreState()
{
    Npc npc(item);
    if (savedFirstStep != 0U) {
        npcs[npc.id].firstStep = 1;
    } else {
        npcs[npc.id].firstStep = 0;
    }
    npc.setLastAnim(savedLastAnim);
    npc.setDirection(savedDirection);
    if (savedFlipped != 0U) {
        npc.setFlags(npc.flags() | ITEM_FLIPPED);
    } else {
        npc.setFlags(npc.flags() & ~ITEM_FLIPPED);
    }
    if (savedUnknown77_4 != 0U) {
        npcs[npc.id].unknown77_4 = 1;
    } else {
        npcs[npc.id].unknown77_4 = 0;
    }
    npc.setAnimFrame(savedAnimFrame);
}

/* The result is the animation's status, whatever is passed. */
void AnimPrimitive::terminate(u32 /*value*/)
{
    Npc npc(item);
    finish();
    status.bits.ended = 1;
    Process::terminate(status.value);
    if ((targetPoint == nullptr) && !npc.inVoid()) {
        if (info.destroyActor) {
            npc.destroy();
            return;
        }
        if (info.endLoop && (loopAtEnd != 0U)) {
            npc.doAnim(anim, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
        }
    }
}

/* The next frame, or the animation's end (terminated: false). The frames
 * go two at a time while the NPC skips frames (skipsFrames; the skipped
 * one's events come too) but at the end, in the quick jump and while the
 * controlled NPC is held. A two-step animation's first step ends at its
 * middle (of three frames at the second), its second step's end goes on
 * from the start when it loops; its looping second step ends there unless
 * another of its animation follows. */
bool AnimPrimitive::nextFrame(Npc &npc, u16 shape, bool &first, u8 &frame, AnimFrame &skipped)
{
    fast = skipsFrames(npc) ? 1 : 0;
    if (first) {
        first = false;
        frame = npcs[npc.id].animFrame;
        if ((fast != 0U) && frame == 2) {
            getAnimFrame(shape, anim, direction, 1, &skipped);
        } else {
            fast = 0;
        }
    } else {
        frame = npcs[npc.id].animFrame;
        if ((fast != 0U) && frame != static_cast<i16>(frames - 1) && frame != static_cast<i16>(frames - 2) &&
            anim != ANIM_QUICK_JUMP && (controlledNpc.held == 0U)) {
            frame = npcs[npc.id].animFrame + 2;
            getAnimFrame(shape, anim, direction, frame - 1, &skipped);
        } else {
            frame = npcs[npc.id].animFrame + 1;
            fast = 0;
        }
    }
    if (info.twoStep) {
        if (npc.isFirstStep() != 0U) {
            const i16 half = frames >> 1;
            if (((frame == half || ((fast != 0U) && frame == static_cast<i16>(half - 1))) && frames != 3) ||
                (frame == 2 && frames == 3)) {
                npcs[npc.id].firstStep = 0;
                terminate(0);
                return false;
            }
            if (frames == frame) {
                if (!info.looping) {
                    npc.setAnimFrame(0);
                } else if (skipsFrames(npc)) {
                    fast = 1;
                    getAnimFrame(shape, anim, direction, 1, &skipped);
                    npc.setAnimFrame(0);
                } else {
                    npc.setAnimFrame(1);
                }
                frame = npcs[npc.id].animFrame;
            }
        } else if (info.looping && frame == static_cast<i16>(frames - 1)) {
            static_cast<void>(findItemProcess(item, type));
            const auto *other = static_cast<AnimPrimitive *>(nextItemProcess());
            if (other == nullptr || other->anim != anim) {
                npcs[npc.id].firstStep = 1;
                terminate(0);
                return false;
            }
        }
    }
    if (frames <= frame) {
        if (frames != 1) {
            npcs[npc.id].firstStep = 1;
        }
        terminate(0);
        return false;
    }
    return true;
}

/* A frame's events: its sound, its usecode event, the NPC's sound and its
 * shot. Returns whether the process lives on. */
bool AnimPrimitive::frameEvents(Npc &npc, const AnimFrame &frame)
{
    if (frame.sound() != 0U) {
        npc.restartSound(frame.sound());
    }
    if (frame.usecode) {
        npc.calledFromAnim();
        if (hasFlags(PROCESS_DEAD) != 0U) {
            return false;
        }
    }
    if (frame.npcSound) {
        makeNpcSound(item);
        if (hasFlags(PROCESS_DEAD) != 0U) {
            return false;
        }
    }
    if (frame.dx != 0 || frame.dy != 0 || frame.dz != 0) {
        fireAtFrame(item, frame);
    }
    return true;
}

/* Whether the animation runs this pass: one past the animations ends; an
 * NPC off the fast area waits, as one falling (not a test); the first run
 * starts it (init); a failure waiting ends it; No Regret's rolling
 * thunder's shot ends unless it faces its target. */
bool AnimPrimitive::runs(Npc &npc, bool &first)
{
    if (anim >= ANIMS) {
        terminate(0);
        return false;
    }
    if ((npc.inFastArea() == 0U) || ((targetPoint == nullptr) && (npc.isFalling() != 0U))) {
        return false;
    }
    if (started != 0U) {
        first = false;
    } else {
        first = true;
        init();
        if ((hasFlags(PROCESS_DEAD) != 0U) || anim == ANIMS) {
            return false;
        }
    }
    if (failPending != 0U) {
        fail(0);
        return false;
    }
    if (ROLLING_THUNDER && (hasTarget != 0U) && (anim == ANIM_FIRE_SMALL || anim == ANIM_FIRE_LARGE)) {
        const Npc target(targetItem);
        const char toward =
            closestDirection(itemX[npc.id], itemY[npc.id], itemX[target.id], itemY[target.id], 16, 0, 15);
        if (direction != toward) {
            terminate(0);
            return false;
        }
        hasTarget = 0;
    }
    return true;
}

/* Where the frame takes the NPC from pos: its step ahead (a quarter turn
 * off for a rotated animation) and up or down, not below 0; a flying NPC in
 * combat follows its target's height. moving: the frame steps or climbs.
 * The frame's flip goes on the NPC. */
ItemPoint AnimPrimitive::frameDestination(Npc &npc, AnimFrame &current, const ItemPoint &pos, bool &moving) const
{
    if (current.flipped) {
        npc.setFlags(npc.flags() | ITEM_FLIPPED);
    } else {
        npc.setFlags(npc.flags() & ~ITEM_FLIPPED);
    }
    const i16 step = current.deltaDir * 2;
    moving = step != 0 || current.deltaZ != 0;
    if (static_cast<i16>(pos.z + current.deltaZ) < 0) {
        current.deltaZ = -pos.z;
    }
    const char dir = info.rotated ? static_cast<char>((direction + 4) & 0xf) : direction;
    ItemPoint next;
    next.set(pos.x + (directionX16[static_cast<int>(dir)] * step), pos.y + (directionY16[static_cast<int>(dir)] * step),
             pos.z + current.deltaZ);
    if (npc.isInCombat() != 0U) {
        const Npc target(npcs[npc.id].attackTarget);
        if (target.isValid() && info.flying) {
            if (npc.z() > target.z()) {
                next.z -= 1;
            } else if (npc.z() < target.z()) {
                next.z += 1;
            }
        }
    }
    return next;
}

/* A test's frame (to targetPoint, which follows it): on the ground a step
 * (if blocked by no item, without the floor, but not up), in the air a
 * sweep (if blocked by an item, straight up or down). Touching the target
 * marks the status; a test that reaches its target (not unstoppable) goes
 * to its next frame. */
AnimPrimitive::FrameMove AnimPrimitive::testMove(u16 shape, const AnimFrame &current, ItemPoint pos, ItemPoint &next,
                                                 i16 frameFlags, HitSweep &sweep, u16 &hit)
{
    const ItemPoint save = next;
    bool ok;
    if (current.onGround) {
        ok = sweep.step(shape, *targetPoint, next, frameFlags, item, touchItem);
        if (!ok && (sweep.hitItem == 0U)) {
            next = save;
            ok = sweep.step(shape, *targetPoint, next, MOVE_NO_FLOOR, item, touchItem);
            if (next.z > save.z) {
                ok = false;
            }
        }
        next = ok ? sweep.end : pos;
    } else {
        ok = sweep.test(shape, *targetPoint, next, frameFlags, item, touchItem);
        if (!ok && (sweep.hitItem != 0U)) {
            next.set(pos.x, pos.y, pos.z + current.deltaZ);
            ok = sweep.test(shape, *targetPoint, next, frameFlags, item, touchItem);
        }
    }
    hit = sweep.hitItem;
    *targetPoint = next;
    if (sweep.targetFirst != 0U) {
        status.bits.touched = 1;
    }
    if (ok) {
        return FrameMove::ADVANCE;
    }
    if ((sweep.targetFirst != 0U) && !info.unstoppable) {
        reachedTarget = 1;
        return FrameMove::NEXT_FRAME;
    }
    return FrameMove::BLOCKED;
}

/* A frame on the ground: the NPC moves by collideMove (if blocked by no
 * item, again without the floor). Stopped, the animation ends; blocked,
 * an unstoppable animation stays put and goes on, one blocked by no item
 * goes on, one blocked by an item is blocked. */
AnimPrimitive::FrameMove AnimPrimitive::collideStep(Npc &npc, ItemPoint &next, bool moving, i16 frameFlags, u16 &hit)
{
    const ItemPoint save = next;
    /* collideMove sets it, and it is read only when collideMove failed. */
    u8 stopped = 0;
    bool ok = !moving || npc.collideMove(&next, 1, frameFlags, touchItem, &stopped, &hit);
    if (hasFlags(PROCESS_DEAD) != 0U) {
        return FrameMove::ENDED;
    }
    if (!ok && (hit == 0U)) {
        next = save;
        ok = npc.collideMove(&next, 1, MOVE_NO_FLOOR, touchItem, &stopped, &hit);
        if (hasFlags(PROCESS_DEAD) != 0U) {
            return FrameMove::ENDED;
        }
    }
    if (ok) {
        next = npc.point();
        return FrameMove::ADVANCE;
    }
    next = save;
    if (stopped != 0U) {
        status.bits.touched = 1;
        terminate(0);
        return FrameMove::ENDED;
    }
    if (info.unstoppable) {
        stayPut = 1;
        return FrameMove::ADVANCE;
    }
    if (hit == 0U) {
        return FrameMove::ADVANCE;
    }
    blockedByItem = 1;
    if (hasFlags(PROCESS_DEAD) != 0U) {
        return FrameMove::ENDED;
    }
    return FrameMove::BLOCKED;
}

/* A frame in the air: the NPC sweeps (if blocked by an item, straight up
 * or down) and takes what is on it along. Blocked, touching first ends the
 * animation; an unstoppable one stays put and goes on. */
AnimPrimitive::FrameMove AnimPrimitive::sweepStep(Npc &npc, u16 shape, const AnimFrame &current, ItemPoint pos,
                                                  ItemPoint &next, bool moving, i16 frameFlags, HitSweep &sweep,
                                                  u16 &hit)
{
    bool ok = !moving || sweep.test(shape, pos, next, frameFlags, item, touchItem);
    if (!ok && (sweep.hitItem != 0U)) {
        next.set(pos.x, pos.y, pos.z + current.deltaZ);
        ok = sweep.test(shape, pos, next, frameFlags, item, touchItem);
    }
    hit = sweep.hitItem;
    if (ok) {
        if (moving) {
            npc.grab();
            sweep.apply();
        }
        if (hasFlags(PROCESS_DEAD) != 0U) {
            return FrameMove::ENDED;
        }
        return FrameMove::ADVANCE;
    }
    blockedByItem = 1;
    if (hasFlags(PROCESS_DEAD) != 0U) {
        return FrameMove::ENDED;
    }
    if (sweep.targetFirst != 0U) {
        status.bits.touched = 1;
        terminate(0);
        return FrameMove::ENDED;
    }
    next = pos;
    if (info.unstoppable) {
        stayPut = 1;
        return FrameMove::ADVANCE;
    }
    return FrameMove::BLOCKED;
}

/* The frame went on: its frame shows with its events and the skipped
 * one's (not for a test), the frame's repeat waits, and a step on the
 * ground ends the run where the NPC stands (canStep). Returns whether the
 * run goes on (a test's next frame). */
bool AnimPrimitive::advanceFrame(Npc &npc, const AnimFrame &current, const AnimFrame &skipped, const ItemPoint &pos,
                                 const ItemPoint &next)
{
    if (targetPoint == nullptr) {
        npc.setFrame(current.frame);
        if ((fast != 0U) && !frameEvents(npc, skipped)) {
            return false;
        }
        if (!frameEvents(npc, current)) {
            return false;
        }
    }
    if (hasFlags(PROCESS_DEAD) != 0U) {
        return false;
    }
    if ((targetPoint == nullptr) && (info.repeat != 0U)) {
        AccWait *wait = new AccWait(static_cast<ui16>(info.repeat * ((fast != 0U) ? 2 : 1)), item);
        wait->block(this);
        wait->listenTo(pid);
        wait->resume();
    }
    if (current.onGround) {
        if (canStep(pos, next, 0)) {
            return false;
        }
        if (hasFlags(PROCESS_DEAD) != 0U) {
            return false;
        }
    }
    return true;
}

/* The frame was blocked: a test fails; else the NPC steps where it can
 * (canStep: not a flying one) and the animation goes on blocked
 * (unknown373f). */
void AnimPrimitive::blockedFrame(const ItemPoint &pos, const ItemPoint &next, u16 hit)
{
    if (targetPoint != nullptr) {
        fail(0);
        return;
    }
    if (!info.flying) {
        canStep(pos, next, hit);
        if (hasFlags(PROCESS_DEAD) != 0U) {
            return;
        }
    }
    unknown373f();
}

/* The animation's frames (runs): each the next frame (nextFrame) and
 * its move (frameDestination), by a test's target point (testMove), in
 * place (stayPut), on the ground (collideStep) or in the air (sweepStep);
 * it advances (advanceFrame) or is blocked (blockedFrame). A test runs all
 * its frames at once. */
void AnimPrimitive::run()
{
    Npc npc(item);
    const u16 shape = npc.shape();
    bool first;
    if (!runs(npc, first)) {
        return;
    }
    AnimFrame current;
    AnimFrame skipped;
    u8 frame;
    HitSweep sweep;
    do {
        if (!nextFrame(npc, shape, first, frame, skipped)) {
            return;
        }
        npc.setAnimFrame(frame);
        getAnimFrame(shape, anim, direction, frame, &current);
        if (fast != 0U) {
            current.deltaDir += skipped.deltaDir;
            current.deltaZ += skipped.deltaZ;
        }
        const ItemPoint pos = (targetPoint != nullptr) ? *targetPoint : npc.point();
        bool moving;
        ItemPoint next = frameDestination(npc, current, pos, moving);
        if (reachedTarget != 0U) {
            continue;
        }
        const i16 frameFlags = current.onGround ? (MOVE_STANDS | MOVE_CLIMB) : MOVE_NO_FLOOR;
        u16 hit = 0;
        sweep.reset();
        FrameMove moved;
        if (targetPoint != nullptr) {
            moved = testMove(shape, current, pos, next, frameFlags, sweep, hit);
        } else if (stayPut != 0U) {
            next = pos;
            moved = info.unstoppable ? FrameMove::ADVANCE : FrameMove::BLOCKED;
        } else if (current.onGround) {
            moved = collideStep(npc, next, moving, frameFlags, hit);
        } else {
            moved = sweepStep(npc, shape, current, pos, next, moving, frameFlags, sweep, hit);
        }
        if (moved == FrameMove::ENDED) {
            return;
        }
        if (moved == FrameMove::NEXT_FRAME) {
            continue;
        }
        if (hasFlags(PROCESS_DEAD) != 0U) {
            halt();
        }
        if (moved == FrameMove::BLOCKED) {
            blockedFrame(pos, next, hit);
            return;
        }
        if (!advanceFrame(npc, current, skipped, pos, next)) {
            return;
        }
    } while (targetPoint != nullptr);
}

void AnimPrimitive::fail(u32 /*value*/)
{
    Process::fail(status.value);
    if (killingAll == 0U) {
        Npc npc(item);
        finish();
        if ((targetPoint == nullptr) && !npc.inVoid() && info.destroyActor) {
            npc.destroy();
        }
    }
}

void AnimPrimitive::holdReticle()
{
    reticleHidden = 1;
    reticleHolds++;
}

void AnimPrimitive::finish()
{
    if (targetPoint != nullptr) {
        restoreState();
        return;
    }
    const Npc npc(item);
    if (!npc.inVoid() && (anim == ANIM_START_RUN_SMALL || anim == ANIM_START_RUN_LARGE)) {
        npcs[npc.id].firstStep = 0;
    }
    if (reticleHidden != 0U) {
        if (reticleHolds != 0U) {
            reticleHolds--;
        }
        if (!npc.inVoid() && item == controlledActor) {
            showReticle();
        }
    }
}

void AnimPrimitive::unknown373f()
{
    const Npc npc(item);
    fail(0);
}

/* What the attack hits: the search was taken out; its box is still worked out
 * (original). */
/* UNCALLED: nothing calls it. */
void AnimPrimitive::checkWeaponHit(ItemPoint *point, AnimFrame *frame)
{
    Npc npc(item);
    if (info.attack && frame->attackRange != 0) {
        const AreaSearch search{};
        Box box;
        ShapeInfo shapeInfo;
        i16 sizeX;
        i16 sizeY;
        i16 sizeZ;
        [[maybe_unused]] i16 left;
        [[maybe_unused]] i16 top;
        [[maybe_unused]] i16 right;
        [[maybe_unused]] i16 bottom;
        char facing;
        facing = npc.direction();
        shapeInfo = typeFlags.info(npc.shape());
        sizeX = shapeInfo.x * 16;
        sizeY = shapeInfo.y * 16;
        sizeZ = shapeInfo.z * 8;
        /* The else holds an expression that makes no code, as assert does
         * with NDEBUG: only the jump over it is left (original). */
        if (sizeZ == 0 && !shapeInfo.fixed) {
            sizeZ = 1;
        } else {
            (void)0;
        }
        box.set(point->x - sizeX + 1, point->y - sizeY + 1, point->z, point->x, point->y, point->z + sizeZ - 1);
        box.moveBy(directionX16[static_cast<int>(facing)] * 32 * frame->attackRange,
                   directionY16[static_cast<int>(facing)] * 32 * frame->attackRange, 0);
        left = box.x1;
        top = box.y1;
        right = box.x2;
        bottom = box.y2;
        left -= 0x100;
        top -= 0x100;
        right += 0x100;
        bottom += 0x100;
    }
}

/* UNCALLED: nothing calls it. */
void AnimPrimitive::unknown38d9() {}

void AnimPrimitive::turnTo(u8 toward)
{
    Npc npc(item);
    if (toward == DIRECTION_CURRENT || toward == DIRECTION_TURN_RIGHT || toward == DIRECTION_TURN_LEFT) {
        direction = npc.direction();
    } else {
        direction = toward;
    }
    if (info.sixteenDirections) {
        if (toward == DIRECTION_TURN_RIGHT) {
            direction = (direction + 1) & 0xf;
        } else if (toward == DIRECTION_TURN_LEFT) {
            direction = (direction - 1) & 0xf;
        }
    } else {
        if ((direction & 1) != 0) {
            direction = (direction - 1) & 0xf;
        }
        if (toward == DIRECTION_TURN_RIGHT) {
            direction = (direction + 2) & 0xf;
        } else if (toward == DIRECTION_TURN_LEFT) {
            direction = (direction - 2) & 0xf;
        }
    }
    npc.setDirection(direction);
}
