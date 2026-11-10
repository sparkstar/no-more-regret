/* ..\CACHE\SHAPHAND.C (Code17; No Regret's 1080, No Remorse's 1070; the
 * addresses are No Regret's): the shapes in the cache. A shape's
 * block holds its ShapeHeader and frame table and lives in the handle table
 * at the shape's number; each frame is read into a block of its own when it is
 * drawn. A frame table entry holds the frame block's data address, or with
 * FRAME_ON_DISK set the frame's position in the shape file; FRAME_KEPT is
 * kept when the frame block moves. No Regret uses ShapeCacheHandler; No
 * Remorse the ShapeHandler, which copies the shapes from a swap file on the
 * CD into the shape file (No Regret never makes one; SHAPES_FROM_SWAP,
 * shaphand_game.h). */
#ifndef PORT_SHAPHAND_H
#define PORT_SHAPHAND_H
#include "common.h"
#include "cachehnd.h"
#include "flexedit.h"
#include "shape.h"

/* The handlers' numbers. */
constexpr int SHAPE_HANDLER = 0;
constexpr int FRAME_HANDLER = 4;
/* The number of shapes, and the handles they take. */
constexpr int SHAPES = 0x800;
/* ShapeFrameEntry::offset */
constexpr u32 FRAME_ON_DISK = (static_cast<u32>(0x80000000L));
constexpr s32 FRAME_KEPT = (static_cast<s32>(0x40000000L));

/* The start of a frame's block, before the frame: its shape and frame, and
 * the frame table entry it replaced. */
struct FrameBlock {
    u16 shape;
    u16 frame;
    u32 entry;
};
ASSERT_SIZE(FrameBlock, 8);

/* Where a frame is drawn (frameBounds). */
struct FrameRect {
    u16 left, top, right, bottom;
};
ASSERT_SIZE(FrameRect, 8);

/* The game's shape file (static\shapes.flx). */
class ShapeFlex : public EditableFlex {
public:
    ShapeFlex(); /* 1080:0000 */
};
ASSERT_SIZE(ShapeFlex, 0x10);

/* The shape blocks (handler SHAPE_HANDLER). */
class ShapeCacheHandler : public CacheHandler {
public:
    /* Takes the handles and opens the shape file when open is set. */
    explicit ShapeCacheHandler(u8 open); /* 1080:0090 */
    ~ShapeCacheHandler() override = default;
    /* The size of a frame and the offset of its origin; shape 1 for a
     * missing frame (1080:0116). */
    void frameMetrics(u16 shape, u16 frame, u16 *width, u16 *height, s16 *xOffset, s16 *yOffset);
    /* The shape's size and frame count; shape 0's for a missing shape
     * (1080:0187). */
    void shapeHeader(u16 shape, u16 *width, u16 *height, u16 *frames);
    /* Where the frame lands when its origin is at (x,y) (1080:0224). */
    void frameBounds(u16 shape, u16 frame, FrameRect *bounds, s16 x, s16 y);
    /* Reads a frame from position entry of the shape file into a new frame
     * block and puts the block in the shape's frame table (1080:0288). */
    FrameBlock *loadFrame(u16 shape, u16 frame, u32 data, u32 entry);
    /* A frame, read when it is not in the cache; 0 when there is no such
     * frame. A frame that cannot be read gives missingFrame when the shape
     * was in the cache, 0 when the shape was just read (1080:04ce). */
    ShapeFrame *getFrame(u16 shape, u16 frame);
    /* As getFrame, with the frame's size; no references (1080:0768). */
    /* UNCALLED: nothing calls it. */
    ShapeFrame *getFrame(u16 shape, u16 frame, u16 *size);
    /* The frame blocks of the shape at block point back at it (1080:07e9). */
    bool check(u32 block) override;
    /* Frees the shape's frames and the shape (1080:08e2). */
    void freeShape(u16 shape);
    void freeFrame(u32 data, u16 frame); /* 1080:09a8 */
};
ASSERT_SIZE(ShapeCacheHandler, 4);

/* A ShapeCacheHandler that fills the shape file from a swap file: No
 * Remorse's shapeCache, never made in No Regret. */
class ShapeHandler : public ShapeCacheHandler {
public:
    u8 unknown04; /* no references */
    /* The size of the shape file after createSwap. */
    s32 swapSize;

    ShapeHandler();           /* 1080:0a29 */
    ~ShapeHandler() override; /* 1080:0e0f */
    /* As ShapeCacheHandler::getFrame, with 0 and missingFrame the other way
     * round for a frame that cannot be read; a shape missing from the shape
     * file is copied from the swap file (1080:0eb7). */
    ShapeFrame *getFrame(u16 shape, u16 frame);
    /* Empties the entries of the shapes the game does not keep and cuts the
     * shape file to swapSize (1080:12de). */
    void trimSwap() const;
    /* Makes the shape file from the swap file (1080:15cf). */
    void createSwap();
    void closeSwap() const; /* 1080:1b0a */
};
ASSERT_SIZE(ShapeHandler, 9);

/* The frame blocks (handler FRAME_HANDLER). */
class ShapeFrameCacheHandler : public CacheHandler {
public:
    /* BCC 3.1 made the implicit destructor of a class with virtual functions
     * virtual when it has work to do, and a delete through this class called the
     * object's own. */
    ~ShapeFrameCacheHandler() override = default;
    ShapeFrameCacheHandler(); /* 1080:13e1 */
    /* Points the shape's frame table at the moved frame (1080:1428). */
    void relocated(u32 data) override;
    /* Puts the frame's file position back in the table (1080:14cc). */
    void evicted(u32 data, u16 handle) override;
};
ASSERT_SIZE(ShapeFrameCacheHandler, 4);

/* All shapes are in the shape file: none is copied from the swap file. */
extern u8 &allShapes;
/* closeSwap trims the shape file instead of making it anew. */
extern u8 &keepSwap;
/* trimSwap keeps the NPCs' shapes (No Remorse's). */
extern u8 &keepNpcShapes;

void initShapeCache();  /* 1080:154b */
void closeShapeCache(); /* 1080:1593 */
/* Deletes the shapes' file and the frames' handler, which closeShapeCache
 * leaves (the port's, for gameRelease). */
void closeShapeFiles();
#include "shaphand_game.h"
/* The shapes' handler: No Regret's a ShapeCacheHandler, No Remorse's a
 * ShapeHandler (SHAPES_FROM_SWAP). */
extern ShapeCacheHandler *&shapeCache;
/* shapeCache->getFrame as the original's callers make it: ShapeHandler's,
 * whose getFrame hides the base's, in No Remorse (the port's; the static
 * comparison sees the getFrame it calls). */
ShapeFrame *cachedShapeFrame(u16 shape, u16 frame);
#endif
