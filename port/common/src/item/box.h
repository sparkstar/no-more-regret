/* Code44 (1158:0000-01c7, file name unknown): a box between two map points,
 * 10 bytes. The frameHit module follows it in the segment (hittest.h). */
#ifndef PORT_BOX_H
#define PORT_BOX_H
#include "common.h"

class Box {
public:
    i16 x1;
    i16 y1;
    u8 z1;
    i16 x2;
    i16 y2;
    u8 z2;

    /* UNCALLED: nothing calls it. */
    void set(i16 newX1, i16 newY1, u8 newZ1, i16 newX2, i16 newY2, u8 newZ2); /* 1158:0000 */
    /* Nothing calls it (1158:0031). */
    /* UNCALLED: nothing calls it. */
    bool overlaps(Box *box) const;
    /* UNCALLED: nothing calls it. */
    void moveBy(i16 dx, i16 dy, u8 dz); /* 1158:018e */
};
ASSERT_SIZE(Box, 10);
#endif
