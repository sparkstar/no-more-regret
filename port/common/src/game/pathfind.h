/* Code38 (file name unknown): the path finder, which takes an NPC to a point or
 * to an item. */
#ifndef PORT_PATHFIND_H
#define PORT_PATHFIND_H
#include "common.h"
#include "kprocess.h"
#include "itemdata.h"

/* Process::type */
constexpr int PATH_FINDER_TYPE = 0x204;

class PathFinder : public EventProcess {
public:
    /* UNCALLED: nothing calls it. */
    explicit PathFinder(Loading loading) : EventProcess(loading) {}
    /* The point it goes to. */
    ItemPoint target;
    /* The direction of the next step: 10h when it cannot go on, FFh when it
     * has arrived. */
    u8 heading;
    u8 lastHeading;
    /* The direction the last test step went. */
    u8 tryHeading;
    /* The direct way is blocked: it goes around, turning to one side. */
    u8 detour;
    u8 detourSide;
    /* The target item is solid and has a height: touching it arrives. */
    u8 solid;
    /* The nearest mapDistance to target so far. */
    i16 distance;
    /* It arrives within this mapDistance of target, 0 for none. */
    i16 stopDistance;
    u16 maxSteps;
    u16 step;
    /* Only tests the way, all steps in one run, without walking. */
    u8 probe;
    u8 arrived;
    /* It turns to the target when it ends. */
    u8 turnAtEnd;
    /* It stands when it ends without turning; cleared when it stops to
     * shoot. */
    u8 standAtEnd;
    /* The item it goes to, 0 for a point. */
    u16 targetItem;
    /* Which way it tries first, random. */
    u8 randomSide;

    PathFinder(u16 npc, u16 goalItem, ItemPoint point, u16 newMaxSteps, u8 newProbe, u16 newStopDistance,
               u8 newTurnAtEnd);
    PathFinder(u16 npc, u16 goalItem, u16 newMaxSteps, u8 newProbe, u16 newStopDistance, u8 newTurnAtEnd);
    /* The weapon goes away while it walks. */
    void holsterWeapon();
    /* Clears the NPC's path flag and turns or stands it. */
    void finish();
    /* Tries the steps from *from and moves it to where the chosen one ends;
     * returns that step's direction. */
    u8 nextDirection(ItemPoint *from);
    void run() override;
    void terminate(u32 endResult) override;
    void fail(u32 failResult) override;
};
ASSERT_SIZE(PathFinder, 0x50);
#endif
