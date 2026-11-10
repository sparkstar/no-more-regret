/* CRUSADER.C (Code41): hits and shots, most of them usecode intrinsics, the
 * Silencer's death, and the processes that heal NPCs and charge the Silencer's
 * battery. */
#ifndef PORT_CRUSADER_H
#define PORT_CRUSADER_H
#include "common.h"
#include "kprocess.h"
#include "item.h"
#include "npc.h"

/* Process::type. The healer and the charger have the same. */
constexpr int DEATH_SILENCE_TYPE = 0x261;
constexpr int HEALER_TYPE = 0x254;

/* Ends the game (No Regret only). */
class GameQuit : public Process {
public:
    explicit GameQuit(Loading loading) : Process(loading) {}
    GameQuit() = default;
    void run() override;
};
ASSERT_SIZE(GameQuit, 0x32);

/* The end once the last boss (shape 5B1h) is dead: the Silencer kneels,
 * turns and dies, and the game ends (No Regret only). */
class EndSequence : public Process {
public:
    explicit EndSequence(Loading loading) : Process(loading) {}
    EndSequence();
    void run() override;
};
ASSERT_SIZE(EndSequence, 0x32);

/* The boss's shield (bossShield, CONTROL.C) is set back to 500 unless it is
 * gone (No Remorse only). */
void resetBossShield();

/* The Silencer's death: the death sound once three timer
 * events have passed. */
class DeathSilence : public PeriodicProcess {
public:
    explicit DeathSilence(Loading loading) : PeriodicProcess(loading) {}
    /* The timer events so far. */
    i16 ticks;

    DeathSilence();
    void run() override;
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(DeathSilence, 0x42);

/* Heals the controlled NPC a point a tick. */
class CruHealer : public Process {
public:
    explicit CruHealer(Loading loading) : Process(loading) {}
    /* The controlled NPC's full hit points. */
    i16 maxHp;

    CruHealer();
    void onKill() override; /* the original's destructor */
    void run() override;
};
ASSERT_SIZE(CruHealer, 0x34);

/* Charges the Silencer's battery 25 a tick. */
class BatteryCharger : public Process {
public:
    explicit BatteryCharger(Loading loading) : Process(loading) {}
    /* The controlled NPC's full energy. */
    u16 maxEnergy;

    BatteryCharger();
    void onKill() override; /* the original's destructor */
    void run() override;
};
ASSERT_SIZE(BatteryCharger, 0x34);

/* Where shooter aims at target: its centre, at a height by its shape. */
void aimPoint(u16 shooter, u16 target, ItemPoint *point);
/* 0 for the robots' shapes. */
bool isOrganic(Item *item);
/* A hit on item, its damage scaled by the difficulty: on an NPC, its
 * shields, death or hurt; on a thing, its breaking and hurling. False when
 * the Silencer's shield took it, or in No Remorse the boss's. */
bool takeHit(Item *item, u16 hitter, u8 direction, i16 damage, u16 type, ItemPoint *point);
/* Intrinsic 55h: takeHit without a point (No Regret's end sequence calls it
 * too). */
void receiveHitIntrinsic(Item *item, u16 hitter, u8 direction, i16 damage, u16 type);
/* A hit without a usecode gotHit: nothing, the push it was to give is left
 * after the return. */
void receiveHit(Item *item, u16 hitter, i16 force);
/* 0 while the controlled NPC's last animation (dodgeStart, dodgeLength)
 * lasts. */
bool dodgeOver(Item *item);
/* Intrinsic 19h: fires shots of type from item, x,y,z from it; aimed looks for
 * a target. The pid of the last shot. */
u16 fireWeapon(Item *item, i16 x, i16 y, i16 z, char direction, i16 type, u8 aimed);
/* Intrinsic 131h: the distance in cells when item can fire at target from its
 * firing frame, or from dx,dy,dz; 0 when something is in the way. */
u16 fireDistance(Item *item, u16 target, u8 direction, i16 dx, i16 dy, i16 dz);
/* Intrinsic 36h. */
u16 rangeIfVisible(Item *item, u16 other);
/* Intrinsics 0FEh and 0C8h: start a battery charger and a healer and
 * return their pids. */
u16 createBatteryCharger();
/* The animation npc plays for anim. */
u16 npcAnim(Npc *npc, u16 anim);

/* The mouse fires (mouseFire); the controller clears it when the shots end. */
extern u8 &mouseFiring;
/* fireDistance found the firing frame. */
extern u8 &firingFrameFound;
/* No Regret's end sequence's hit on the Silencer kills it the gory way
 * (EndSequence::run, takeHit). */
extern u8 &finalHit;

u16 createHealer();

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
u16 rangeWithin(Item *item, u16 other, u16 range);
#endif
