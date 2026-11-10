/* Code117 (file name unknown): the item the mouse drags over
 * the map. WORLD.C picks it up, drops it and reads its fields. */
#ifndef PORT_DRAGITEM_H
#define PORT_DRAGITEM_H
#include "item.h"
#include "rect.h"

class DraggedItem {
public:
    /* An item of shape and frame drawn at the pointer, when not dragging
     * item. */
    Item ghost;
    Item item;
    u16 shape, frame;
    u8 dragsItem;
    u8 roundPlace; /* rounds the ghost's place to a step */
    u8 liftByZ;    /* lifts the ghost's place by its z */
    u16 unknown0b, unknown0d;
    u16 originX, originY; /* where the ghost is shown */
    u8 originZ;
    Point grab; /* the pointer's place on the item */
    /* Where item was dropped on the screen; -1 when it went into a
     * container. */
    i16 dropX, dropY;
    u16 under; /* the item under the drop point */

    DraggedItem();
    /* Drags a ghost of shape and frame, or item when shape is 0. Nothing
     * calls it. */
    /* UNCALLED: nothing calls it. */
    void setShape(u16 newShape, u16 newFrame);
    /* UNCALLED: nothing calls it. */
    void showGhost();
    void unknown01e9();
    /* Drops item, or the ghost, at screen point x,y; 1 when it went there. */
    bool dropOnMap(i16 x, i16 y);
    /* Drops item into container at x,y of its gump (No Remorse also
     * empties the controlled NPC's slot 7). Nothing calls it. */
    /* UNCALLED: nothing calls it. */
    bool dropInContainer(u16 container, i16 x, i16 y);
    /* Nothing calls it. */
    /* UNCALLED: nothing calls it. */
    void release();
    /* UNCALLED: nothing calls it. */
    void setQuality() const;
    /* Starts dragging item (No Remorse also empties the controlled NPC's
     * slot 7). */
    void pickUp(u16 pickedItem);
    /* Stops dragging item; an item of the same shape where it was dropped
     * takes it in (Item::absorb). In No Remorse, dropped on an NPC, it moves
     * the controlled NPC's slot 7 item into its pack (slot 6). */
    void drop();
    /* The map point for screen point x,y, on top of the item there. */
    void mapPoint(i16 x, i16 y, ItemPoint *point);
};
ASSERT_SIZE(DraggedItem, 0x1e);

extern DraggedItem &draggedItem;
#endif
