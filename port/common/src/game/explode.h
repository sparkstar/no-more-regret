/* Code45 (file name unknown, among the game modules; No Regret's 1160, No
 * Remorse's 1148; the addresses are No Regret's): turning NPCs, explosions
 * and whether an item fits at a point (usecode intrinsics). turnNpc's
 * parameters are each game's (explode_game.h). */
#ifndef PORT_EXPLODE_H
#define PORT_EXPLODE_H
#include "common.h"
#include "item.h"
#include "animfrm.h"
#include "explode_game.h"

/* The explosion of fire type at x,y,z (1160:02f8). */
void fireExplosion(u16 type, u16 x, u16 y, u16 z);
/* Sends the controller a space key (pressed or released) unless the NPC
 * movedActor is busy (1160:0806). WORLD.C passes the mouse point, which
 * it does not read. */
void mouseFire(u8 pressed, i16 x, i16 y);
/* The offset of the first moving frame of the controlled NPC's fire
 * animation in direction; 0 when no frame moves (1160:087b). */
bool fireOffset(u8 direction, u16 *x, u16 *y, u16 *z);
/* The controlled NPC's first moving frame of its fire animation, 0 for none
 * (1160:098a). */
AnimFrame *controlledFrame();
/* Intrinsic 12h (1160:0a77). */
void explodeIntrinsic(Item *item, i16 type, u8 destroys);
/* Explodes item with explosion type (0-2), destroying the item with destroys
 * and hurting what is near with damages; 0 (1160:0aaf). */
i16 explode(Item *item, i16 type, u8 destroys, u8 damages);

bool canExistAt(u16 shape, u16 x, u16 y, u16 z);
/* No Regret's: turns npc from one direction to another, a turn animation a
 * step; the pid of the last, 0 for none (1160:00bf). */
u16 turnFrom(u16 npc, char from, char to, u8 speed, u8 unknown);
/* Turns npc toward direction and returns the pid of the animation, 0 for
 * none (1160:0084, Actor::I_turnToward). */
u16 turnNpc(u16 npc, char direction, u8 speed, u8 unknown);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
bool unknown0000(u16 shape, u16 frame, u16 x, u16 y, u8 z);
/* UNCALLED: nothing calls it. */
void unknown0a93(Item *item, i16 type, u8 destroys);
#endif
