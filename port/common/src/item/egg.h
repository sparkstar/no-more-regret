/* Code21 (file name unknown, among the ..\ITEM modules): eggs,
 * teleport eggs and the process that watches an egg. The egg accessors are
 * Item members (item.h). */
#ifndef PORT_EGG_H
#define PORT_EGG_H
#include "common.h"
#include "kprocess.h"

/* Holds the controlled NPC still until it runs, and lets it go unless it
 * was held before. */
class NpcHold : public Process {
public:
    /* UNCALLED: nothing calls it. */
    explicit NpcHold(Loading loading) : Process(loading) {}
    u8 wasHeld;

    NpcHold();
    void run() override;
};
ASSERT_SIZE(NpcHold, 0x33);

/* Takes the controlled NPC to egg of map. */
class Teleporter : public Process {
public:
    explicit Teleporter(Loading loading) : Process(loading) {}
    u16 map;
    u16 egg;

    /* To the destination of teleport egg eggNumber. */
    explicit Teleporter(u16 eggNumber);
    /* To egg of map, or of the current map. */
    Teleporter(u16 targetMap, u16 targetEgg, u8 hold);
    Teleporter(u16 targetEgg, u8 hold);
    /* Kills the NPC's teleports and, with hold, holds it still. */
    void start(u8 hold);
    void run() override;
    /* run's on the map, and its end, which go on after the map's loading's
     * wait (the port's). */
    void arrive(u16 x, u16 y, u8 z, u16 oldMap, u8 sameMap);
    void arrived();
};
ASSERT_SIZE(Teleporter, 0x36);

/* The process type of EggHatcher. */
constexpr int EGG_HATCHER_TYPE = 0x20f;

/* Hatches its egg when the controlled NPC comes into the egg's range and
 * unhatches it when it leaves. */
class EggHatcher : public Process {
public:
    explicit EggHatcher(Loading loading) : Process(loading) {}
    u8 inRange;

    explicit EggHatcher(u16 egg);
    void run() override;
    /* Draws the egg's range when the egg display is on. */
    void show();
};
ASSERT_SIZE(EggHatcher, 0x33);

/* The egg display is on. */
extern u8 &showEggs;

/* A teleport to egg of map, or of the current map; the pid of its process.
 * The usecode's intrinsics (86h and 9Ah in No Regret, 79h and 96h in No
 * Remorse); No Remorse's startGame, game menu and map jumps call the first. */
u16 teleportToEgg(u16 map, u16 egg, u8 hold);
u16 teleportToEgg(u16 egg, u8 hold);
#endif
