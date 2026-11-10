/* DLITEM.C (Code55): the items in the display list. Each item on the map has a
 * DL_ItemNode, found through itemNodes in No Regret and in the grid cells its
 * box covers in No Remorse; the nodes are drawn with the item's shape and
 * ordered by their boxes. */
#ifndef PORT_DLITEM_H
#define PORT_DLITEM_H
#include "common.h"
#include "dlist.h"

class DL_ItemNode : public DL_Node {
public:
    u16 item;
    /* The item's point on the screen, before the scroll. */
    i16 screenX, screenY;
    /* The item's map point, halved in x and y. */
    i16 x, y, z;
    /* The item's box (ShapeInfo::box; a flipped item swaps width and
     * depth). */
    i16 width, depth, height;

    DL_ItemNode() = default;
    /* The order of this node and other: 0 when the boxes do not overlap,
     * 1 when this one is drawn first, 2 when other is. */
    i16 order(u16 other);
    /* The frame, the weapon over the controlled NPC and with showItemBoxes
     * the box. */
    void draw() override;
    /* The frame's pixels, or the box with hitByBox. */
    bool hit(i16 pointX, i16 pointY) override;
    void release() override { detach(); }
    /* Sets the screen point, the box and bounds from the item. */
    void place();
    /* Takes item and places the node; the argument goes through a
     * temporary. */
    void attach(u16 number)
    {
        item = number;
        place();
    }
    /* Takes the node from the item. */
    void detach() const;
};
ASSERT_SIZE(DL_ItemNode, 0x2a);

/* A part of the screen to draw again, empty until set. Being a Rect with a
 * constructor, it is in BSS and the static constructor clears it; a Rect is
 * assigned to it through a ScreenArea temporary. */
struct ScreenArea : public Rect {
    ScreenArea() : Rect(0, 0, 0, 0) {}
    explicit ScreenArea(Rect *other) : Rect(other) {}
    ScreenArea(s16 areaLeft, s16 areaTop, s16 areaRight, s16 areaBottom)
        : Rect(areaLeft, areaTop, areaRight, areaBottom)
    {
    }
};
ASSERT_SIZE(ScreenArea, 8);

/* Cheat: the items are drawn with their boxes. */
extern u8 &showItemBoxes;
/* Only read. */
extern u8 &unknown3631;
/* hit skips the shapes with ShapeInfo::ignore; a new game sets it. */
extern u8 &skipIgnored;
/* The part of the view to update and of the screen to draw again, and
 * whether each is set. */
extern ScreenArea &updateArea;
extern u8 &updateSet;
extern ScreenArea &dirtyArea;
extern u8 &dirtySet;
/* The node of each item (No Regret only). */
extern u16 *&itemNodes;

/* The display node of item. Its message names it
 * DL_ItemNode::find. */
u16 itemNode(u16 item);
/* Moves item's node to the item's place. */
void updateItem(u16 item);
void redrawItem(u16 item);
/* Rebuilds the node of pendingItem and draws the changed nodes. */
void updateNodes();
void flushDirty();
/* Paints updateArea over the view. */
void paintUpdate();

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
u16 findItemNode(u16 item, i16 left, i16 top, i16 right, i16 bottom);
/* UNCALLED: nothing calls it. */
void invalidateItems(Rect area);
/* UNCALLED: nothing calls it. */
u8 ignoredSkipped();
#endif
