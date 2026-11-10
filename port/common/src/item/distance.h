/* Code29 (10e0, file name unknown, among the ..\ITEM modules): directions
 * and distances on the map. A direction is 0 (north, y-1) to 15 turning
 * east, 16 for none; the 8 main ones are the even numbers. The SYSTEM helper
 * module (utility.h) has the same distances under other names. */
#ifndef PORT_DISTANCE_H
#define PORT_DISTANCE_H
#include "common.h"
#include "itemdata.h"

/* No direction. */
constexpr int NO_DIRECTION = 16;
/* The 8 main directions. The screen's up is north-west: the arrow keys
 * move the controlled NPC north-west, south-east, south-west and north-east
 * (Controller::move). */
constexpr int DIRECTION_NORTH = 0;
constexpr int DIRECTION_NORTHEAST = 2;
constexpr int DIRECTION_EAST = 4;
constexpr int DIRECTION_SOUTHEAST = 6;
constexpr int DIRECTION_SOUTH = 8;
constexpr int DIRECTION_SOUTHWEST = 0xa;
constexpr int DIRECTION_WEST = 0xc;
constexpr int DIRECTION_NORTHWEST = 0xe;
/* A direction not chosen yet (Attack::attack). */
constexpr int DIRECTION_INVALID = 100;

/* The step of each of the 16 directions (1480:1699-16ba); DISTANCE.C keeps
 * those of the 8. */
extern char (&directionX16)[17];
extern char (&directionY16)[17];

/* The direction from x1,y1 to x2,y2 moved to the nearer end of low-high
 * when it is outside it; directions is 8 or 16 (10e0:0000, intrinsic
 * 23h). */
i16 closestDirection(i16 x1, i16 y1, i16 x2, i16 y2, i16 directions, i16 low, i16 high);
/* The larger of the x and y distances (10e0:0098). */
i16 mapDistance(i16 x1, i16 y1, i16 x2, i16 y2);
/* The largest of the x, y and z distances (10e0:00ca). */
i16 mapDistance(i16 x1, i16 y1, u8 z1, i16 x2, i16 y2, u8 z2);
/* The direction from x2,y2 to x1,y1, one of the 8 (10e0:0120). */
char direction(i16 x1, i16 y1, i16 x2, i16 y2);
/* The direction from x1,y1 to x2,y2, one of the 16 (10e0:01c7, intrinsic
 * CDh). */
char fineDirection(i16 x1, i16 y1, i16 x2, i16 y2);
/* The map chunk of point is in the fast area (10e0:0551). */
bool inFastArea(ItemPoint point);
#endif
