/* The shape format of the FLEX archives: a header, one table entry per frame,
 * then the frames. Frame offsets count from the start of the shape. */
#ifndef PORT_SHAPE_H
#define PORT_SHAPE_H
#include "viewport.h"
#include "x32code.h"
struct __attribute__((packed)) ShapeFrameEntry {
    u32 offset;
    s32 size;
};
/* The start of a shape, which SHAPHAND.C reads before the frame table. */
struct __attribute__((packed)) ShapeSize {
    u16 width, height; /* of the largest frame */
    i16 frameCount;
};
struct __attribute__((packed)) ShapeHeader : ShapeSize {
    ShapeFrameEntry frames[1]; /* frameCount entries */
};
ASSERT_FILE_SIZE(ShapeHeader, 0xe);
/* The size and the offsets are 32-bit fields: the 32-bit drawShape reads
 * them whole and SPMGR.C compares the size at 32 bits; most C code reads the
 * low words, and CDLOAD.C cuts them to 16 bits. */
struct __attribute__((packed)) ShapeFrame {
    u8 unknown00[4];
    s32 width;
    s32 height;
    s32 xOffset;
    s32 yOffset;
};
ASSERT_FILE_SIZE(ShapeFrameEntry, 0x8);
ASSERT_FILE_SIZE(ShapeSize, 0x6);
ASSERT_FILE_SIZE(ShapeFrame, 0x14);
/* drawShape (x32code.h) on the current draw target. */
inline void drawShape(i16 x, i16 y, ShapeFrame *frame)
{
    drawShape(drawViewport, x, y, frame);
}
inline void drawShapeRemapped(i16 x, i16 y, ShapeFrame *frame, u8 *table)
{
    drawShapeRemapped(drawViewport, x, y, frame, table);
}
#endif
