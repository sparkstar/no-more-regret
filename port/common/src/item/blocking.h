/* Code4 (1018, file name unknown): what a shape is blocked by and what blocks
 * a box of the map. */
#ifndef PORT_BLOCKING_H
#define PORT_BLOCKING_H
#include "common.h"
#include "type.h"

/* The kinds of blocking a shape has: 8000h always, 1 when it is solid,
 * 20h when it is damaging (1018:0000). */
u16 blockMask(ShapeInfo *info);
/* The blocking kinds of the items whose boxes meet the box from left,top to
 * right,bottom and low to high; first gets the first solid one. FFFFh when
 * a bound is negative (1018:00fd). */
u16 blockersAt(i16 left, i16 top, i16 right, i16 bottom, u8 low, u8 high, u16 *first);

/* Nothing calls these, in the original either. */
u16 blockersAtPoint(u16 x, u16 y, u8 z);
u16 blockersInCell(i16 column, i16 row, i16 level);
bool restsOnBlocker(u16 id);
#endif
