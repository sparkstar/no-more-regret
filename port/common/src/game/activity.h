/* Code36 (file name unknown; No Regret's 1118, No Remorse's 1100; the addresses
 * are No Regret's): the processes of the NPC activities that Npc::setActivity
 * starts. Each is an Activity, a periodic process on its NPC; ATTACK.C's Attack
 * is one too (attack.h). Their constructors are out of line: new passes a null
 * this and the constructor allocates (the sizes are those it asks for), and
 * each puts the process's name in processNames. The BCC sizes are each game's
 * (activity_game.h): No Remorse's Activity has no interval and lastTime, so
 * every member after Activity's sits 6 bytes lower. */
#ifndef PORT_ACTIVITY_H
#define PORT_ACTIVITY_H
#include "common.h"
#include "kprocess.h"
#include "activity_game.h"

/* Process::type */
constexpr int ACTIVITY_TYPE = 0x205;
constexpr int STAND_TYPE = 0x258;
constexpr int SURRENDER_TYPE = 0x25f;
constexpr int PACE_TYPE = 0x255;
constexpr int GUARD_TYPE = 0x25e;
constexpr int LOITER_TYPE = 0x257;
constexpr int ROLLING_THUNDER_TYPE = 0x263;

class Activity : public PeriodicProcess {
public:
    explicit Activity(Loading loading) : PeriodicProcess(loading) {}
    u16 npc;
    /* The seconds between the NPC's idle actions, 10-24 at random (No
     * Regret's, with lastTime and intervalPassed). */
    i16 interval;
    /* When it last acted (SystemTimer::ticks), 0 at first one time in three. */
    u32 lastTime;

    explicit Activity(u16 ownerNpc); /* 1118:0000 */
    /* 1 when interval seconds have passed since lastTime, which it then
     * sets (1118:011d). */
    bool intervalPassed();
    /* Puts the NPC in its default activity 1 when it sees the controlled NPC
     * (1118:033e). */
    [[nodiscard]] bool checkAlert() const;
};
ASSERT_SIZE(Activity, ACTIVITY_SIZE);

class Stand : public Activity {
public:
    /* UNCALLED: nothing calls it. */
    explicit Stand(Loading loading) : Activity(loading) {}
    explicit Stand(u16 ownerNpc); /* 1118:03bd */
    void run() override;
};
ASSERT_SIZE(Stand, ACTIVITY_SIZE);

class Surrender : public Activity {
public:
    explicit Surrender(Loading loading) : Activity(loading) {}
    /* Set once run has played the NPC's surrender sound (No Remorse's run
     * plays it once, 1100:04ab). */
    u8 soundPlayed;
    u16 unknown48;

    explicit Surrender(u16 ownerNpc); /* 1118:0453 */
    void onKill() override;           /* the original's destructor */
    void run() override;
};
ASSERT_SIZE(Surrender, SURRENDER_SIZE);

class Pace : public Activity {
public:
    /* UNCALLED: nothing calls it. */
    explicit Pace(Loading loading) : Activity(loading) {}
    u16 unknown48;
    /* The walks that failed in a row. */
    i16 failures;

    explicit Pace(u16 ownerNpc); /* 1118:0641 */
    void run() override;
};
ASSERT_SIZE(Pace, ACTIVITY_SIZE + 4);

class Guard : public Activity {
public:
    explicit Guard(Loading loading) : Activity(loading) {}
    explicit Guard(u16 ownerNpc); /* 1118:0932 */
    void run() override;
};
ASSERT_SIZE(Guard, ACTIVITY_SIZE);

class Loiter : public Activity {
public:
    /* UNCALLED: nothing calls it. */
    explicit Loiter(Loading loading) : Activity(loading) {}
    i16 count;
    /* Where the NPC stood when it began. */
    u16 homeX;
    u16 homeY;
    u8 homeZ;

    Loiter(u16 ownerNpc, i16 loiterCount); /* 1118:0aa9 */
    void run() override;
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(Loiter, ACTIVITY_SIZE + 7);

/* No Regret's. */
class RollingThunder : public Activity {
public:
    /* UNCALLED: nothing calls it. */
    explicit RollingThunder(Loading loading) : Activity(loading) {}
    /* When it last looked for another target (SystemTimer::ticks). */
    u32 lastSeek;

    explicit RollingThunder(u16 ownerNpc); /* 1118:0ef5 */
    /* 1 once 90 ticks have passed since lastSeek, which it then sets
     * (1118:0f8b). */
    bool seekDue();
    /* Takes an NPC it can shoot within 400 as the target (1118:154d). */
    bool seek();
    void run() override;
};
ASSERT_SIZE(RollingThunder, ACTIVITY_SIZE + 4);

/* 1 when npc sees the controlled NPC in front of it; with mode, a kneeling
 * NPC in animation 1Eh or 1Fh looks aside (1118:0192). */
bool seesControlledNpc(u16 npc, u8 mode);
#endif
