/* Code117 (file name unknown; No Regret's 13a0:0000-0953, No Remorse's
 * 13a8:0000-0a09; the addresses are No Regret's). See dragitem.h. No
 * Remorse's inventory slots are its value (dragitem_game.h). */
#include "dragitem.h"
#include "gamestate.h"
#include "dragitem_game.h"
#include "camera.h"
#include "collide.h"
#include "crusys.h"
#include "dispatch.h"
#include "gravity.h"
#include "itemutil.h"
#include "npc.h"
#include "type.h"

DraggedItem &draggedItem = gameGlobal<DraggedItem>();

namespace {

/* mapPointAt and screenToMap into an ItemPoint, whose members, packed as the
 * map files hold them, have no aligned address to pass. Both write their
 * results before they read them. */
void mapPointAt(i16 x, i16 y, ItemPoint *point, Item *item, u8 *inside)
{
    u16 mapX;
    u16 mapY;
    u8 z;
    ::mapPointAt(x, y, &mapX, &mapY, &z, item, inside);
    point->set(mapX, mapY, z);
}

void screenToMap(i16 x, i16 y, ItemPoint *point)
{
    u16 mapX;
    u16 mapY;
    ::screenToMap(x, y, &mapX, &mapY);
    point->x = mapX;
    point->y = mapY;
}

}

DraggedItem::DraggedItem()
{
    ghost.id = 0;
    item.id = 0;
    shape = 0;
    frame = 0;
    dragsItem = 0;
    roundPlace = 0;
    liftByZ = 0;
    unknown0d = 0;
    unknown0b = 0;
    grab = Point(0, 0);
}

/* UNCALLED: nothing calls it. */
void DraggedItem::setShape(u16 newShape, u16 newFrame)
{
    NewGump *top = modals[modalTop];

    this->shape = newShape;
    this->frame = newFrame;
    if (this->shape == 0) {
        dragsItem = 1;
        return;
    }
    guiDispatcher->enqueue(&lvalue(NewGumpEvent(EVENT_BROADCAST, this->shape, this->frame, 0x88, top, top, 0L)));
    showGhost();
}

/* UNCALLED: nothing calls it. */
void DraggedItem::showGhost()
{
    u8 z;

    if (ghost.isValid()) {
        ghost.destroy();
    }
    if (dragsItem != 0U) {
        if (ghost.isValid()) {
            ghost.id = 0;
            return;
        }
    } else {
        ItemPoint point(originX, originY, z = originZ);

        if (liftByZ != 0U) {
            point.x += z << 1;
            point.y += z << 1;
        }
        if (roundPlace != 0U) {
            point.x |= 0xf;
            point.y |= 0xf;
        }
        if (!ghost.legalCreateAtPoint(shape, frame, &point)) {
            ghost.id = 0;
        }
    }
}

void DraggedItem::unknown01e9() {}

bool DraggedItem::dropOnMap(i16 x, i16 y)
{
    bool dropped = false;
    ItemPoint point;

    under = 0;
    if (dragsItem != 0U) {
        if (item.isValid()) {
            Sweep sweep;

            unknown01e9();
            item.push();
            mapPoint(x, y, &point);
            if (sweep.fits(item.shape(), &point, 0)) {
                item.popToCoords(point.x, point.y, point.z);
                dropped = true;
                dropX = x;
                dropY = y;
            } else {
                item.pop();
            }
        }
    } else {
        if (ghost.isValid()) {
            ghost.destroy();
        }
        mapPointAt(x, y, &point, nullptr, nullptr);
        ghost.create(shape, frame);
        ghost.popToCoords(point.x, point.y, point.z);
        dropped = true;
    }
    return dropped;
}

/* UNCALLED: nothing calls it. */
bool DraggedItem::dropInContainer(u16 container, i16 x, i16 y)
{
    bool dropped = false;

    dropX = dropY = -1;
    if (dragsItem != 0U) {
        if (item.isValid()) {
            if (DRAG_USES_SLOT_7) {
                controlledNpc.clearSlot(7);
            }
            if (Container(container).canAdd(item.id)) {
                if (((item.flags() & ITEM_CONTAINED) == 0) || item.container() != container) {
                    unknown01e9();
                }
                item.push();
                item.popToEnd(container);
                item.setGumpPosition(x, y);
                dropped = true;
            }
        }
    } else {
        if (ghost.isValid()) {
            ghost.destroy();
        }
        ghost.create(shape, frame);
    }
    return dropped;
}

/* UNCALLED: nothing calls it. */
void DraggedItem::release()
{
    setQuality();
    fall(&ghost);
    ghost.id = 0;
}

/* UNCALLED: nothing calls it. */
void DraggedItem::setQuality() const
{
    const i16 family = ghost.family();

    if (ghost.shape() != 0x4fe && family != FAMILY_CONTAINER && family != FAMILY_GENERIC) {
        if (family == FAMILY_QUANTITY || family == FAMILY_REAGENT) {
            ghost.setQ(1);
        } else {
            ghost.setQ(0);
        }
        /* The original tests these families and does nothing. */
        if (family == FAMILY_WEAPON || family == FAMILY_EGG || family == FAMILY_MONSTER_EGG ||
            family == FAMILY_TELEPORT_EGG) {
            /* empty in the original */
        }
    }
}

void DraggedItem::pickUp(u16 pickedItem)
{
    dropX = dropY = -1;
    this->item.id = 0;
    if (DRAG_USES_SLOT_7) {
        controlledNpc.clearSlot(7);
    }
    if (pickedItem != 0U) {
        const u16 container = Item(pickedItem).container();

        /* The original tests for container 100h and does nothing. */
        if ((container != 0U) && container == 0x100) {
            /* empty in the original */
        }
        this->item = Item(pickedItem);
    }
}

void DraggedItem::drop()
{
    if (item.isValid()) {
        i16 family;
        const ShapeInfo info = typeFlags.infos[item.shape()];

        if ((family = info.family) == FAMILY_QUANTITY || family == FAMILY_REAGENT) {
            if (dropX >= 0 && dropY >= 0) {
                item.push();
                Item target;

                if (itemAt(&target.id, dropX, dropY)) {
                    if (target.shape() == item.shape()) {
                        if ((target.flags() & ITEM_CONTAINED) != 0) {
                            item.popToContainer(target.container());
                            item.setGumpPosition(target.gumpX(), target.gumpY());
                        } else {
                            item.popToPoint(lvalue(target.point()));
                        }
                        Item(item.id).absorb(target.id);
                    } else {
                        item.pop();
                    }
                } else {
                    item.pop();
                }
            }
        }
        const u16 container = item.container();

        /* No Regret's tests for container 100h here and does nothing. */
        if (DRAG_USES_SLOT_7 && (container != 0U) && container < 0x100) {
            /* Dropped on an NPC: the item in slot 7 goes into the pack (slot
             * 6). */
            Item slot7(controlledNpc.equipment(7));

            if ((Item(controlledNpc.equipment(6)).isValid()) && slot7.isValid()) {
                controlledNpc.clearSlot(7);
                slot7.push();
                slot7.popToEnd(controlledNpc.equipment(6));
                slot7.setGumpPosition(0, 0);
            }
        }
        if (((item.flags() & ITEM_CONTAINED) == 0) && (worldActive != 0U)) {
            fall(&item);
        }
        item.id = 0;
    }
}

void DraggedItem::mapPoint(i16 x, i16 y, ItemPoint *point)
{
    Npc underNpc(0);
    u8 inside;

    mapPointAt(x, y, point, &underNpc, &inside);
    this->under = underNpc.id;
    screenToMap(x - grab.x, y - grab.y, point);
    point->x += point->z << 1;
    point->y += point->z << 1;
    if (underNpc.isValid() && (inside == 0U)) {
        const ShapeInfo info = typeFlags.infos[item.shape()];
        i16 width;
        i16 depth;
        i16 height;

        info.halfBox(width, depth, height);
        mapPointAt(x - grab.x - width + depth, y - grab.y - (static_cast<i16>(width + depth) >> 2), point, nullptr,
                   nullptr);
        info.box(width, depth, height);
        point->x += width;
        point->y += depth;
    }
}
