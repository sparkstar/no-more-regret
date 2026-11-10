/* ANIMPRIM.C (Code46; No Regret's 1168, No Remorse's 1150; the addresses are
 * No Regret's): an NPC's animation process, and the animations NPC.C's
 * Npc::doAnim starts, one class each (their vtables are NPC.C's,
 * 1480:17cd-22ad). */
#ifndef PORT_ANIMPRIM_H
#define PORT_ANIMPRIM_H
#include "common.h"
#include "kprocess.h"
#include "astream.h"
#include "itemdata.h"
#include "animprim_game.h"

class AnimFrame;
class HitSweep;
class Npc;

/* The process type of the animations. */
constexpr int ANIM_TYPE = 0xf0;
/* Npc::doAnim's direction for the NPC's own. */
constexpr int DIRECTION_CURRENT = 0x10;
/* The NPC's own direction turned one step clockwise or anticlockwise (two
 * steps with 8 directions; AnimPrimitive::turnTo). */
constexpr int DIRECTION_TURN_RIGHT = 0x11;
constexpr int DIRECTION_TURN_LEFT = 0x12;

/* The animation numbers (Npc::doAnim), named after their AnimPrimitive classes;
 * 5, 1Bh and 35h are named from anim.dat and the keys that start them. A move
 * of an animation with AnimInfo::rotated goes a quarter turn right, so a
 * negative one goes left. 6, 11h, 13h and 16h keep their numbers (Anim06 and so
 * on): nothing asks for them, and anim.dat has them only as a still frame of a
 * few robots and machines. */
constexpr int ANIM_STAND = 0x00;
constexpr int ANIM_WALK = 0x01;
constexpr int ANIM_RETREAT_SMALL = 0x02;
constexpr int ANIM_RUN = 0x03;
constexpr int ANIM_COMBAT_STAND_SMALL = 0x04;
/* Ctrl and a move key: a dive and a roll ahead that ends kneeling. */
constexpr int ANIM_ROLL_FORWARD = 0x05;
constexpr int ANIM_READY_SMALL = 0x07;
constexpr int ANIM_FIRE_SMALL = 0x08;
constexpr int ANIM_RELOAD_SMALL = 0x0a;
constexpr int ANIM_UNREADY_SMALL = 0x0b;
constexpr int ANIM_READY_LARGE = 0x0c;
constexpr int ANIM_FIRE_LARGE = 0x0d;
constexpr int ANIM_RELOAD = 0x0e;
constexpr int ANIM_RELOAD_LARGE = 0x0f;
constexpr int ANIM_UNREADY_LARGE = 0x10;
constexpr int ANIM_FALL_BACKWARDS = 0x12;
constexpr int ANIM_FALL_FORWARDS = 0x14;
constexpr int ANIM_KNEEL_ROLL_LEFT = 0x17;
constexpr int ANIM_KNEEL_ROLL_RIGHT = 0x18;
constexpr int ANIM_STOP_RUN_LARGE = 0x19;
constexpr int ANIM_KNEEL_FIRE = 0x1a;
/* A step to the right, kneeling: CONTROL.C picks it wherever it picks
 * ANIM_SLIDE_RIGHT for a standing NPC. */
constexpr int ANIM_KNEELING_SLIDE_RIGHT = 0x1b;
constexpr int ANIM_SLIDE_LEFT = 0x1c;
constexpr int ANIM_SLIDE_RIGHT = 0x1d;
constexpr int ANIM_LOOK_LEFT = 0x1e;
constexpr int ANIM_LOOK_RIGHT = 0x1f;
constexpr int ANIM_TELEPORT_IN = 0x20;
constexpr int ANIM_TELEPORT_OUT = 0x21;
constexpr int ANIM_START_RUN_SMALL = 0x22;
constexpr int ANIM_START_RUN_LARGE = 0x23;
constexpr int ANIM_ADVANCE_SMALL = 0x24;
constexpr int ANIM_COMBAT_STAND_LARGE = 0x25;
constexpr int ANIM_START_RUN = 0x26;
constexpr int ANIM_STOP_RUN_SMALL = 0x27;
constexpr int ANIM_KNEEL_START = 0x28;
constexpr int ANIM_KNEEL_END = 0x29;
constexpr int ANIM_KNEEL_FIRE_SMALL = 0x2a;
constexpr int ANIM_KNEEL_FIRE_LARGE = 0x2b;
constexpr int ANIM_ADVANCE_LARGE = 0x2c;
constexpr int ANIM_RETREAT_LARGE = 0x2d;
constexpr int ANIM_KNEELING_SMALL = 0x2e;
constexpr int ANIM_KNEELING_LARGE = 0x2f;
constexpr int ANIM_COMBAT_RUN_SMALL = 0x30;
constexpr int ANIM_COMBAT_RUN_LARGE = 0x31;
constexpr int ANIM_BRIGHT_KNEEL_FIRE = 0x32;
constexpr int ANIM_KNEELING_RETREAT = 0x33;
constexpr int ANIM_KNEELING_ADVANCE = 0x34;
/* A step to the left, kneeling (ANIM_SLIDE_LEFT standing). */
constexpr int ANIM_KNEELING_SLIDE_LEFT = 0x35;
constexpr int ANIM_BRIGHT_FIRE_LARGE = 0x36;
constexpr int ANIM_ELECTROCUTED = 0x37;
constexpr int ANIM_JUMP_FORWARD = 0x38;
constexpr int ANIM_SURRENDER = 0x39;
constexpr int ANIM_QUICK_JUMP = 0x3a;
constexpr int ANIM_JUMP_LANDING = 0x3b;
constexpr int ANIM_SURRENDER_STAND = 0x3c;
constexpr int ANIM_COMBAT_ROLL_LEFT = 0x3d;
constexpr int ANIM_COMBAT_ROLL_RIGHT = 0x3e;
constexpr int ANIM_FINISH_FIRING = 0x3f;
/* The number past the last animation: Npc::busyAnim's "none". */
constexpr int ANIMS = 0x40;

/* An animation's state, which it ends with as its result
 * (Process::terminate). */
struct AnimBits {
    ui16 unknown00 : 1;
    /* A test step (with a point) touched the item it may touch. */
    ui16 touched : 1;
    /* Set by AnimPrimitive::terminate. */
    ui16 ended : 1;
    ui16 unknown03 : 1;
    /* The test move ends where the NPC would not stand (canStep). */
    ui16 unsupported : 1;
    ui16 unknown05 : 11;
    ui16 unknown10 : 16;
};
ASSERT_SIZE(AnimBits, 4);

class AnimStatus {
public:
    union {
        u32 value;
        AnimBits bits;
    };

    AnimStatus() {}
    // NOLINTNEXTLINE(google-explicit-constructor): the status is its word, as the original's
    AnimStatus(u32 newValue) : value(newValue) {}
};
ASSERT_SIZE(AnimStatus, 4);

class AnimPrimitive : public EventProcess {
public:
    explicit AnimPrimitive(Loading loading) : EventProcess(loading) {}
    i16 anim;
    char direction;
    /* The animation's frames per direction (info.frames). */
    u8 frames;
    /* init() has run. */
    u8 started;
    /* A blocked move of an unstoppable animation: the NPC stays put. */
    u8 stayPut;
    /* A move was blocked by an item (canStep tests the start). */
    u8 blockedByItem;
    /* The move with a point met its target: the frames go on in place. */
    u8 reachedTarget;
    /* An end-loop animation starts again when it ends (1 at first). */
    u8 loopAtEnd;
    /* run() fails the animation. */
    u8 failPending;
    /* holdReticle took the reticle away for the animation. */
    u8 reticleHidden;
    /* The frames go two at a time: the NPC is off the screen or the game
     * skips frames (frameSkip). */
    u8 fast;
    /* No Regret's rolling thunder aims its animations (setTarget); No Remorse
     * has no hasTarget and targetItem, and its info sits after unknown49. */
    u8 hasTarget;
    AnimStatus status;
    /* The item a move with a point may touch; nothing reads the word after
     * it. */
    u16 touchItem;
    u16 unknown49;
    /* The item the animation aims at (No Regret's). */
    u16 targetItem;
    u8 unknown4d[2];
    /* The point it moves to, when it has one: a test move, which leaves
     * the NPC as it was (saveState, restoreState). */
    ItemPoint *targetPoint;
    /* The NPC as it was before a test move. */
    u8 savedFirstStep;
    u16 savedLastAnim;
    u8 savedDirection;
    u8 savedFlipped;
    u8 savedUnknown77_4;
    u8 savedAnimFrame;
    /* The animation's header (animInfo). */
    AnimInfo info;

    AnimPrimitive(u16 npc, u16 newAnim, u8 newDirection, ItemPoint *newTargetPoint, u16 newTouchItem,
                  u16 newUnknown49 = 0); /* 1168:0000 */
    void setTarget(u16 newTargetItem)
    {
        hasTarget = 1;
        targetItem = newTargetItem;
    }
    /* The first run: the controlled NPC's moves that are blocked try the
     * points beside it (1168:0174), and its steps (the port's). */
    void init();
    bool searchesBlockedMove(Npc &npc) const;
    void findRollRoom(Npc &npc, Process *anchor, ItemPoint from, u16 shape);
    void findWalkRoom(Npc &npc, Process *anchor, ItemPoint from, u16 shape);
    /* run's steps (the port's): whether it runs, the next frame, a frame's
     * events and destination, its move by a test's target point, on the
     * ground or in the air (each returns how it went), and what follows
     * an advance or a block. */
    enum class FrameMove { ENDED, NEXT_FRAME, ADVANCE, BLOCKED };
    bool runs(Npc &npc, bool &first);
    bool nextFrame(Npc &npc, u16 shape, bool &first, u8 &frame, AnimFrame &skipped);
    bool frameEvents(Npc &npc, const AnimFrame &frame);
    ItemPoint frameDestination(Npc &npc, AnimFrame &current, const ItemPoint &pos, bool &moving) const;
    FrameMove testMove(u16 shape, const AnimFrame &current, ItemPoint pos, ItemPoint &next, i16 frameFlags,
                       HitSweep &sweep, u16 &hit);
    FrameMove collideStep(Npc &npc, ItemPoint &next, bool moving, i16 frameFlags, u16 &hit);
    FrameMove sweepStep(Npc &npc, u16 shape, const AnimFrame &current, ItemPoint pos, ItemPoint &next, bool moving,
                        i16 frameFlags, HitSweep &sweep, u16 &hit);
    bool advanceFrame(Npc &npc, const AnimFrame &current, const AnimFrame &skipped, const ItemPoint &pos,
                      const ItemPoint &next);
    void blockedFrame(const ItemPoint &pos, const ItemPoint &next, u16 hit);
    /* The frame the animation starts at (1168:1e63). */
    void startFrame();
    void run() override;                            /* 1168:1fcd */
    void event(u32 event, EventData data) override; /* 1168:3327 */
    /* UNCALLED: nothing calls it. */
    void saveState();                    /* 1168:336b */
    void restoreState();                 /* 1168:3415 */
    void terminate(u32 result) override; /* 1168:3528 */
    void fail(u32 result) override;      /* 1168:35e4 */
    /* The animation takes the reticle away until it ends (1168:3673). */
    void holdReticle();
    /* The end: the NPC as it was after a test move, or its reticle back
     * (1168:3684). */
    void finish();
    virtual void unknown373f(); /* 1168:373f */
    /* The box an attack frame reaches; nothing is done with it (1168:376a). */
    /* UNCALLED: nothing calls it. */
    void checkWeaponHit(ItemPoint *point, AnimFrame *frame);
    /* Empty; nothing calls it (1168:38d9). */
    /* UNCALLED: nothing calls it. */
    void unknown38d9();
    /* The NPC stands at to after moving from from; 0 when it falls or is
     * pushed (1168:38de). */
    bool canStep(ItemPoint from, ItemPoint to, u16 hit);
    /* Turns the animation to direction; 10h keeps the NPC's, 11h and 12h
     * turn one step right and left (1168:3ae0). */
    void turnTo(u8 toward);
};
/* BCC's: No Regret 5Eh, No Remorse 5Bh (without hasTarget and targetItem). */
ASSERT_SIZE(AnimPrimitive, ANIM_PRIMITIVE_SIZE);

/* The animations. None declares anything of its own: each is its number and a
 * vtable, named after what it shows or by the number. */
class StandAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit StandAnim(Loading loading) : AnimPrimitive(loading) {}
    StandAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_STAND, newDirection, nullptr, 0) {}
};
class WalkAnim : public AnimPrimitive {
public:
    explicit WalkAnim(Loading loading) : AnimPrimitive(loading) {}
    WalkAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_WALK, newDirection, nullptr, 0) {}
    WalkAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_WALK, newDirection, point, newTouchItem)
    {
    }
};
class RetreatSmallAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit RetreatSmallAnim(Loading loading) : AnimPrimitive(loading) {}
    RetreatSmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_RETREAT_SMALL, newDirection, nullptr, 0) {}
    RetreatSmallAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_RETREAT_SMALL, newDirection, point, newTouchItem)
    {
    }
};
class RunAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit RunAnim(Loading loading) : AnimPrimitive(loading) {}
    RunAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_RUN, newDirection, nullptr, 0) {}
    RunAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_RUN, newDirection, point, newTouchItem)
    {
    }
};
class CombatStandSmallAnim : public AnimPrimitive {
public:
    explicit CombatStandSmallAnim(Loading loading) : AnimPrimitive(loading) {}
    CombatStandSmallAnim(u16 npc, u8 newDirection)
        : AnimPrimitive(npc, ANIM_COMBAT_STAND_SMALL, newDirection, nullptr, 0)
    {
    }
};
class RollForwardAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit RollForwardAnim(Loading loading) : AnimPrimitive(loading) {}
    RollForwardAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_ROLL_FORWARD, newDirection, nullptr, 0) {}
    RollForwardAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_ROLL_FORWARD, newDirection, point, newTouchItem)
    {
    }
};
class Anim06 : public AnimPrimitive {
public:
    explicit Anim06(Loading loading) : AnimPrimitive(loading) {}
    Anim06(u16 npc, u8 newDirection) : AnimPrimitive(npc, 0x06, newDirection, nullptr, 0) {}
};
class ReadySmallAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit ReadySmallAnim(Loading loading) : AnimPrimitive(loading) {}
    ReadySmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_READY_SMALL, newDirection, nullptr, 0) {}
};
class FireSmallAnim : public AnimPrimitive {
public:
    explicit FireSmallAnim(Loading loading) : AnimPrimitive(loading) {}
    FireSmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_FIRE_SMALL, newDirection, nullptr, 0) {}
};
class UnreadySmallAnim : public AnimPrimitive {
public:
    explicit UnreadySmallAnim(Loading loading) : AnimPrimitive(loading) {}
    UnreadySmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_UNREADY_SMALL, newDirection, nullptr, 0) {}
};
class ReadyLargeAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit ReadyLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    ReadyLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_READY_LARGE, newDirection, nullptr, 0) {}
};
class FireLargeAnim : public AnimPrimitive {
public:
    explicit FireLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    FireLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_FIRE_LARGE, newDirection, nullptr, 0) {}
};
class ReloadLargeAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit ReloadLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    ReloadLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_RELOAD_LARGE, newDirection, nullptr, 0) {}
};
class UnreadyLargeAnim : public AnimPrimitive {
public:
    explicit UnreadyLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    UnreadyLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_UNREADY_LARGE, newDirection, nullptr, 0) {}
};
class Anim11 : public AnimPrimitive {
public:
    explicit Anim11(Loading loading) : AnimPrimitive(loading) {}
    Anim11(u16 npc, u8 newDirection) : AnimPrimitive(npc, 0x11, newDirection, nullptr, 0) {}
};
class FallBackwardsAnim : public AnimPrimitive {
public:
    explicit FallBackwardsAnim(Loading loading) : AnimPrimitive(loading) {}
    FallBackwardsAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_FALL_BACKWARDS, newDirection, nullptr, 0) {}
};
class Anim13 : public AnimPrimitive {
public:
    explicit Anim13(Loading loading) : AnimPrimitive(loading) {}
    Anim13(u16 npc, u8 newDirection) : AnimPrimitive(npc, 0x13, newDirection, nullptr, 0) {}
};
class FallForwardsAnim : public AnimPrimitive {
public:
    explicit FallForwardsAnim(Loading loading) : AnimPrimitive(loading) {}
    FallForwardsAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_FALL_FORWARDS, newDirection, nullptr, 0) {}
};
class Anim16 : public AnimPrimitive {
public:
    explicit Anim16(Loading loading) : AnimPrimitive(loading) {}
    Anim16(u16 npc, u8 newDirection) : AnimPrimitive(npc, 0x16, newDirection, nullptr, 0) {}
};
class KneelRollLeftAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelRollLeftAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelRollLeftAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEEL_ROLL_LEFT, newDirection, nullptr, 0) {}
    KneelRollLeftAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_KNEEL_ROLL_LEFT, newDirection, point, newTouchItem)
    {
    }
};
class KneelRollRightAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelRollRightAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelRollRightAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEEL_ROLL_RIGHT, newDirection, nullptr, 0)
    {
    }
    KneelRollRightAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_KNEEL_ROLL_RIGHT, newDirection, point, newTouchItem)
    {
    }
};
class StopRunLargeAnim : public AnimPrimitive {
public:
    explicit StopRunLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    StopRunLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_STOP_RUN_LARGE, newDirection, nullptr, 0) {}
};
class KneelingSlideRightAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelingSlideRightAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelingSlideRightAnim(u16 npc, u8 newDirection)
        : AnimPrimitive(npc, ANIM_KNEELING_SLIDE_RIGHT, newDirection, nullptr, 0)
    {
    }
    KneelingSlideRightAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_KNEELING_SLIDE_RIGHT, newDirection, point, newTouchItem)
    {
    }
};
class SlideLeftAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit SlideLeftAnim(Loading loading) : AnimPrimitive(loading) {}
    SlideLeftAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_SLIDE_LEFT, newDirection, nullptr, 0) {}
    SlideLeftAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_SLIDE_LEFT, newDirection, point, newTouchItem)
    {
    }
};
class SlideRightAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit SlideRightAnim(Loading loading) : AnimPrimitive(loading) {}
    SlideRightAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_SLIDE_RIGHT, newDirection, nullptr, 0) {}
    SlideRightAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_SLIDE_RIGHT, newDirection, point, newTouchItem)
    {
    }
};
class LookLeftAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit LookLeftAnim(Loading loading) : AnimPrimitive(loading) {}
    LookLeftAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_LOOK_LEFT, newDirection, nullptr, 0) {}
};
class LookRightAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit LookRightAnim(Loading loading) : AnimPrimitive(loading) {}
    LookRightAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_LOOK_RIGHT, newDirection, nullptr, 0) {}
};
class TeleportInAnim : public AnimPrimitive {
public:
    explicit TeleportInAnim(Loading loading) : AnimPrimitive(loading) {}
    TeleportInAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_TELEPORT_IN, newDirection, nullptr, 0) {}
};
class TeleportOutAnim : public AnimPrimitive {
public:
    explicit TeleportOutAnim(Loading loading) : AnimPrimitive(loading) {}
    TeleportOutAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_TELEPORT_OUT, newDirection, nullptr, 0) {}
};
class StartRunSmallAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit StartRunSmallAnim(Loading loading) : AnimPrimitive(loading) {}
    StartRunSmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_START_RUN_SMALL, newDirection, nullptr, 0) {}
    StartRunSmallAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_START_RUN_SMALL, newDirection, point, newTouchItem)
    {
    }
};
class StartRunLargeAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit StartRunLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    StartRunLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_START_RUN_LARGE, newDirection, nullptr, 0) {}
    StartRunLargeAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_START_RUN_LARGE, newDirection, point, newTouchItem)
    {
    }
};
class AdvanceSmallAnim : public AnimPrimitive {
public:
    explicit AdvanceSmallAnim(Loading loading) : AnimPrimitive(loading) {}
    AdvanceSmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_ADVANCE_SMALL, newDirection, nullptr, 0) {}
    AdvanceSmallAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_ADVANCE_SMALL, newDirection, point, newTouchItem)
    {
    }
};
class CombatStandLargeAnim : public AnimPrimitive {
public:
    explicit CombatStandLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    CombatStandLargeAnim(u16 npc, u8 newDirection)
        : AnimPrimitive(npc, ANIM_COMBAT_STAND_LARGE, newDirection, nullptr, 0)
    {
    }
};
class StartRunAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit StartRunAnim(Loading loading) : AnimPrimitive(loading) {}
    StartRunAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_START_RUN, newDirection, nullptr, 0) {}
    StartRunAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_START_RUN, newDirection, point, newTouchItem)
    {
    }
};
class StopRunSmallAnim : public AnimPrimitive {
public:
    explicit StopRunSmallAnim(Loading loading) : AnimPrimitive(loading) {}
    StopRunSmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_STOP_RUN_SMALL, newDirection, nullptr, 0) {}
};
class KneelStartAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelStartAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelStartAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEEL_START, newDirection, nullptr, 0) {}
};
class KneelEndAnim : public AnimPrimitive {
public:
    explicit KneelEndAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelEndAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEEL_END, newDirection, nullptr, 0) {}
};
class KneelFireSmallAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelFireSmallAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelFireSmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEEL_FIRE_SMALL, newDirection, nullptr, 0)
    {
    }
};
class KneelFireLargeAnim : public AnimPrimitive {
public:
    explicit KneelFireLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelFireLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEEL_FIRE_LARGE, newDirection, nullptr, 0)
    {
    }
};
class AdvanceLargeAnim : public AnimPrimitive {
public:
    explicit AdvanceLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    AdvanceLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_ADVANCE_LARGE, newDirection, nullptr, 0) {}
    AdvanceLargeAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_ADVANCE_LARGE, newDirection, point, newTouchItem)
    {
    }
};
class RetreatLargeAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit RetreatLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    RetreatLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_RETREAT_LARGE, newDirection, nullptr, 0) {}
    RetreatLargeAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_RETREAT_LARGE, newDirection, point, newTouchItem)
    {
    }
};
class KneelingSmallAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelingSmallAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelingSmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEELING_SMALL, newDirection, nullptr, 0) {}
};
class KneelingLargeAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelingLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelingLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEELING_LARGE, newDirection, nullptr, 0) {}
};
class CombatRunSmallAnim : public AnimPrimitive {
public:
    explicit CombatRunSmallAnim(Loading loading) : AnimPrimitive(loading) {}
    CombatRunSmallAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_COMBAT_RUN_SMALL, newDirection, nullptr, 0)
    {
    }
    CombatRunSmallAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_COMBAT_RUN_SMALL, newDirection, point, newTouchItem)
    {
    }
};
class CombatRunLargeAnim : public AnimPrimitive {
public:
    explicit CombatRunLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    CombatRunLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_COMBAT_RUN_LARGE, newDirection, nullptr, 0)
    {
    }
    CombatRunLargeAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_COMBAT_RUN_LARGE, newDirection, point, newTouchItem)
    {
    }
};
class BrightKneelFireAnim : public AnimPrimitive {
public:
    explicit BrightKneelFireAnim(Loading loading) : AnimPrimitive(loading) {}
    BrightKneelFireAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_BRIGHT_KNEEL_FIRE, newDirection, nullptr, 0)
    {
    }
};
class KneelingRetreatAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelingRetreatAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelingRetreatAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEELING_RETREAT, newDirection, nullptr, 0)
    {
    }
    KneelingRetreatAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_KNEELING_RETREAT, newDirection, point, newTouchItem)
    {
    }
};
class KneelingAdvanceAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelingAdvanceAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelingAdvanceAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_KNEELING_ADVANCE, newDirection, nullptr, 0)
    {
    }
    KneelingAdvanceAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_KNEELING_ADVANCE, newDirection, point, newTouchItem)
    {
    }
};
class KneelingSlideLeftAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit KneelingSlideLeftAnim(Loading loading) : AnimPrimitive(loading) {}
    KneelingSlideLeftAnim(u16 npc, u8 newDirection)
        : AnimPrimitive(npc, ANIM_KNEELING_SLIDE_LEFT, newDirection, nullptr, 0)
    {
    }
    KneelingSlideLeftAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_KNEELING_SLIDE_LEFT, newDirection, point, newTouchItem)
    {
    }
};
class BrightFireLargeAnim : public AnimPrimitive {
public:
    explicit BrightFireLargeAnim(Loading loading) : AnimPrimitive(loading) {}
    BrightFireLargeAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_BRIGHT_FIRE_LARGE, newDirection, nullptr, 0)
    {
    }
};
class ElectrocutedAnim : public AnimPrimitive {
public:
    explicit ElectrocutedAnim(Loading loading) : AnimPrimitive(loading) {}
    ElectrocutedAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_ELECTROCUTED, newDirection, nullptr, 0) {}
};
class JumpForwardAnim : public AnimPrimitive {
public:
    explicit JumpForwardAnim(Loading loading) : AnimPrimitive(loading) {}
    JumpForwardAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_JUMP_FORWARD, newDirection, nullptr, 0) {}
};
class SurrenderAnim : public AnimPrimitive {
public:
    explicit SurrenderAnim(Loading loading) : AnimPrimitive(loading) {}
    SurrenderAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_SURRENDER, newDirection, nullptr, 0) {}
};
class QuickJumpAnim : public AnimPrimitive {
public:
    /* UNCALLED: nothing calls it. */
    explicit QuickJumpAnim(Loading loading) : AnimPrimitive(loading) {}
    QuickJumpAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_QUICK_JUMP, newDirection, nullptr, 0) {}
};
class JumpLandingAnim : public AnimPrimitive {
public:
    explicit JumpLandingAnim(Loading loading) : AnimPrimitive(loading) {}
    JumpLandingAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_JUMP_LANDING, newDirection, nullptr, 0) {}
};
class SurrenderStandAnim : public AnimPrimitive {
public:
    explicit SurrenderStandAnim(Loading loading) : AnimPrimitive(loading) {}
    SurrenderStandAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_SURRENDER_STAND, newDirection, nullptr, 0) {}
};
class CombatRollLeftAnim : public AnimPrimitive {
public:
    explicit CombatRollLeftAnim(Loading loading) : AnimPrimitive(loading) {}
    CombatRollLeftAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_COMBAT_ROLL_LEFT, newDirection, nullptr, 0)
    {
    }
    CombatRollLeftAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_COMBAT_ROLL_LEFT, newDirection, point, newTouchItem)
    {
    }
};
class CombatRollRightAnim : public AnimPrimitive {
public:
    explicit CombatRollRightAnim(Loading loading) : AnimPrimitive(loading) {}
    CombatRollRightAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_COMBAT_ROLL_RIGHT, newDirection, nullptr, 0)
    {
    }
    CombatRollRightAnim(u16 npc, u8 newDirection, ItemPoint *point, u16 newTouchItem)
        : AnimPrimitive(npc, ANIM_COMBAT_ROLL_RIGHT, newDirection, point, newTouchItem)
    {
    }
};
class FinishFiringAnim : public AnimPrimitive {
public:
    explicit FinishFiringAnim(Loading loading) : AnimPrimitive(loading) {}
    FinishFiringAnim(u16 npc, u8 newDirection) : AnimPrimitive(npc, ANIM_FINISH_FIRING, newDirection, nullptr, 0) {}
};
#endif
