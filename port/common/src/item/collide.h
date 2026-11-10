/* Code31 (10f0, file name unknown, among the ..\ITEM modules): moving a
 * shape's box through the map, what it hits on the way and where it stops.
 * A Sweep keeps the result; apply() moves the item there and calls the hit
 * events. */
#ifndef PORT_COLLIDE_H
#define PORT_COLLIDE_H
#include "common.h"
#include "itemdata.h"

/* Sweep::options: an item under the end within the drop fails the move. */
constexpr int SWEEP_FLOOR_FAILS = 4;
/* Sweep::test's flags. moveItem, moveShape and HitSweep::step take 0 as
 * MOVE_STANDS. Without MOVE_NO_FLOOR the end must stand, and only
 * MOVE_STANDS lets a solid item under it hold it. */
constexpr int MOVE_STANDS = 1;    /* a solid item within the drop under the end holds it */
constexpr int MOVE_NO_FLOOR = 4;  /* the end needs nothing under it */
constexpr int MOVE_STEP_DOWN = 8; /* the drop is 9, not 0 */
constexpr int MOVE_CLIMB = 0x10;  /* HitSweep::step tries a blocked move 8 and 9 higher */

/* An item a move met, and how far the move got before it. */
struct Hit {
    u16 item;
    i16 x;
    i16 y;
    i16 z;
    Hit *next;
};
ASSERT_SIZE(Hit, 0xc);

/* The move Sweep::test checks against each item near it. */
struct Move {
    /* From start to end. */
    i16 x;
    i16 y;
    i16 z;
    ItemPoint start;
    ItemPoint end;
    /* The move looks for a floor under the end (test() without MOVE_NO_FLOOR). */
    u8 fall;
    /* An item under the end is within the drop but Sweep::options has
     * SWEEP_FLOOR_FAILS. */
    u8 blockedBelow;
    /* A solid item under the end holds it. */
    u8 supported;
    /* The moving shape is damaging. */
    u8 damaging;
    /* Nothing blocking was hit and the end is on the map. */
    u8 clear;
};
ASSERT_SIZE(Move, 0x15);

class Sweep {
public:
    /* The blocking kinds the moving shape stops at (blockMask). */
    u16 mask;
    /* 2 and SWEEP_FLOOR_FAILS, which test() with MOVE_STANDS clears. Bit 2 is
     * not read. */
    u16 options;
    /* The box of the moving shape. */
    i16 width;
    i16 depth;
    i16 height;
    u8 unknown0a[6];
    /* How far the move gets: to the first blocking hit, or all of it. */
    i16 moveX;
    i16 moveY;
    i16 moveZ;
    /* The whole move asked for. */
    i16 fullX;
    i16 fullY;
    i16 fullZ;
    /* How far below the end a floor still holds the item (9 with flag 8),
     * then how far below it the floor found is. */
    i16 drop;
    /* Where the move ends. */
    ItemPoint end;
    /* The axes the move was stopped on: 1 x, 2 y, 4 z. */
    u16 blockedAxes;
    /* The item that moves (0 once apply() is done). */
    u16 item;
    /* The first item that blocks the move. */
    u16 hitItem;
    /* Every item met (freed by the destructor). */
    Hit *hits;
    /* The item the move is aimed at. */
    u16 target;
    /* The first solid item in the way of fits(). */
    u16 blocker;
    /* The target is the first item that blocks the move. */
    u8 targetFirst;
    /* The target blocks the move. */
    u8 targetHit;
    /* The move leaves the map or the fast area. */
    u8 outside;

    Sweep();  /* 10f0:0000 */
    ~Sweep(); /* 10f0:0048 */
    /* Forgets the hits (10f0:0077). */
    void reset();
    void clearHits(); /* 10f0:0095 */
    /* Nothing blocks shape at point; turned swaps its width and depth
     * (10f0:00d9). */
    bool fits(u16 shape, ItemPoint *point, u8 turned);
    /* Keeps a hit on item at x,y,z from the start; a blocking hit nearer
     * than the first so far becomes the first (10f0:0228). */
    void addHit(u16 metItem, i16 x, i16 y, i16 z, u8 blocking);
    /* Checks the move against other (10f0:032e). */
    void collide(u16 other, Move *move);
    /* Moves shape's box from start to x,y,z, stopping at the first item
     * that blocks it; flags are MOVE_*. mask 0 takes the shape's own. 1 when
     * the move is clear (10f0:0b39). */
    bool test(u16 shape, ItemPoint &start, i16 x, i16 y, i16 z, u8 flags, u16 movingItem, u16 testMask, u16 aimedItem);
    bool test(u16 shape, ItemPoint &from, ItemPoint &to, u8 flags, u16 movingItem, u16 aimedItem, u16 testMask = 0)
    {
        return test(shape, from, to.x, to.y, to.z, flags, movingItem, testMask, aimedItem);
    }
    /* Moves item to x,y,z; flags 0 means MOVE_STANDS (10f0:1151). */
    bool moveItem(u16 movingItem, u16 x, u16 y, u16 z, u8 flags, u16 aimedItem);
    /* Moves shape from from to to; flags 0 means MOVE_STANDS (10f0:11c5). */
    bool moveShape(u16 shape, ItemPoint &from, ItemPoint &to, u8 flags, u16 movingItem, u16 aimedItem);
    /* Puts the item where the move ends and calls the hit events of the
     * items it met (10f0:123a). */
    void apply();
    bool move(u16 movingItem, ItemPoint *point, u8 flags)
    {
        return moveItem(movingItem, point->x, point->y, point->z, flags, 0);
    }
    /* Nothing stops item's line of sight from start to end, other not
     * counted. */
    bool lineOfSight(ItemPoint &start, ItemPoint &endPoint, u16 other, u16 viewerItem)
    {
        return test(0, start, endPoint.x, endPoint.y, endPoint.z, MOVE_NO_FLOOR, other, 0, viewerItem);
    }
};
ASSERT_SIZE(Sweep, 0x34);

/* A move that climbs a step of 8 or 9 when it is blocked (Item::collideMove). */
class HitSweep : public Sweep {
public:
    /* How far the item went up (down when negative) on the way. */
    i16 climb;

    bool move(u16 movingItem, ItemPoint *point, u8 flags, u16 aimedItem)
    {
        return collideMove(movingItem, point->x, point->y, point->z, flags, aimedItem);
    }
    bool collideMove(u16 movingItem, u16 x, u16 y, u16 z, u8 flags, u16 aimedItem); /* 10f0:15b4 */
    /* moveShape(), keeping to the floor; with MOVE_CLIMB a blocked move tries
     * again 8 and 9 higher (10f0:162f). */
    bool step(u16 shape, ItemPoint &from, ItemPoint &to, u8 flags, u16 movingItem, u16 aimedItem);
};
ASSERT_SIZE(HitSweep, 0x36);

/* Nothing blocks shape at x,y,z; else blocker gets the first solid item in
 * the way. The other three arguments are not read (10f0:1874). */
bool shapeFits(u16 shape, u16 x, u16 y, u16 z, u8 flags, u16 item, u16 target, u16 *blocker);
#endif
