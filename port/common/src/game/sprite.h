/* SPRITE.C (Code43): sprites, the processes that show an animation at a map
 * point, and the super sprites, the shots that fly across the map. The frames
 * are drawn by SPMGR.C (spmgr.h). */
#ifndef PORT_SPRITE_H
#define PORT_SPRITE_H
#include "common.h"
#include "kprocess.h"
#include "item.h"
#include "collide.h"

/* Process::type */
constexpr int SPRITE_TYPE = 0x249;
constexpr int SUPER_SPRITE_TYPE = 0x250;

class Sprite : public EventProcess {
public:
    /* UNCALLED: nothing calls it. */
    explicit Sprite(Loading loading) : EventProcess(loading) {}
    i16 firstFrame;
    i16 lastFrame;
    i16 frame;
    u16 repeats;
    /* A fire shape (326h and others): with frameSkip it shows every
     * other frame. */
    u8 fire;
    /* Its entry in SPMGR.C's table (spmgr.h), -1 for none. */
    i16 handle;

    Sprite(u16 shape, u16 newFirstFrame, u16 newLastFrame, u16 newRepeats, u16 x, u16 y, u8 z);
    void onKill() override; /* the original's destructor, Moves the sprite by dx,dy,dz. */
    void moveBy(i16 dx, i16 dy, char dz) const;
    void shift(i16 dx, i16 dy, i16 dz) const { moveBy(dx, dy, dz); }
    void run() override;
    void cleanup() override;
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(Sprite, 0x41);

/* A shot flying across the map. */
class SuperSprite : public EventProcess {
public:
    explicit SuperSprite(Loading loading) : EventProcess(loading) {}
    /* The steps of a homing shot, per tick. */
    i16 vx;
    i16 vy;
    i16 vz;
    /* Its entry in SPMGR.C's table, -1 for none. */
    i16 sprite;
    u16 shape;
    /* Where it was, where it is, where it goes next, where it started. */
    ItemPoint last;
    ItemPoint now;
    ItemPoint next;
    ItemPoint start;
    /* Where it last splashed; x -1 for none. */
    ItemPoint splash;
    u8 launched;
    /* A straight shot is at start plus counter/steps of the move dx, dy,
     * dz (distance is in cells of 16). */
    s32 steps;
    s32 dy;
    s32 dx;
    s32 counter;
    u16 damage;
    i16 distance;
    i16 dz;
    /* Its fire type (firetype.h). */
    u16 fire;
    /* The point it was launched at. */
    ItemPoint target;
    /* The item the sweep hit. */
    u16 hitItem;
    Sweep sweep;
    /* Who fired it and at whom. */
    u16 source;
    u16 targetItem;
    /* Times a fire type 5 shot bounced off a roof (at most 5). */
    i16 bounces;
    /* A fire type 9 shot steers toward targetItem until its time is up. */
    u8 homing;
    u8 expired;
    u32 startTime;
    u32 unknownb9;
    char direction;
    u16 frame;

    SuperSprite();
    void onKill() override; /* the original's destructor, The shot is shape (and frame) at point. */
    void place(u16 newShape, u16 newFrame, ItemPoint *point);
    void place(u16 newShape, ItemPoint *point);
    /* Makes the shot an item of shape and frame at point; its number, 0
     * when none. */
    /* UNCALLED: nothing calls it. */
    u16 createItem(u16 createdShape, u16 createdFrame, ItemPoint *point);
    /* Launches the shot toward point; an inexact shot spreads. */
    void launch(char newDirection, ItemPoint point, u16 newDamage, u16 newFire, u16 newSource, u16 newTarget,
                u8 inexact);
    /* Launches the shot far along direction. */
    void launchInDirection(char newDirection, u16 newDamage, u16 newFire, u16 newSource, u16 newTarget, u8 inexact);
    /* Launches the shot from source past the reticle at height z. */
    void launchAtReticle(char newDirection, u16 newDamage, u16 newFire, u16 newSource, u16 newTarget, u8 z);
    /* Sweeps the shot from one point to another; 1 when nothing is in the way,
     * else hitItem is what stopped it. */
    bool move(ItemPoint from, ItemPoint to);
    /* Goes on to next and turns the sprite's frame. */
    void advance();
    /* Finds where the move from from to to hits, hurts what is there and
     * explodes. */
    void hit(ItemPoint from, ItemPoint to);
    /* Destroys the shot's item, or ends the process. */
    void finish();
    /* On an item at least 3 by 3 by 3, pulls point to 2 cells inside its low
     * x and y edges. */
    void clampToItem(Item clampItem, ItemPoint *point);
    /* Hurts what is near point and shows the explosion. */
    void explodeAt(ItemPoint *point) const;
    void run() override;
    void event([[maybe_unused]] u32 event, [[maybe_unused]] EventData data) override {}
};
ASSERT_SIZE(SuperSprite, 0xc0);

/* Intrinsic 027: starts a sprite of shape at x,y,z and returns its pid. */
u16 createSpriteProcess(u16 shape, u16 firstFrame, u16 lastFrame, u16 repeats, u16 x, u16 y, u8 z);
/* Moves x,y,z of a fire shape to where the weapon's muzzle shows on the
 * screen (createSpriteProcess, each game's). */
void adjustPoint(u16 shape, u16 *x, u16 *y, u8 *z);
/* 1 for a shape of fire, an explosion's blast (1150:1fb1). Nothing calls it
 * in the original, whose callers test the shapes themselves. */
bool isFireShape(u16 shape);

/* Nothing calls it, in the original either. */
/* UNCALLED: nothing calls it. */
u16 unknown0000(u16 /*unused*/);
#endif
