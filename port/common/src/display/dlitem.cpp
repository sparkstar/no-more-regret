/* DLITEM.C (Code55): the items in the display list. See dlitem.h. */
#include "dlitem.h"
#include "gamestate.h"
#include "animprim.h"
#include "basecam.h"
#include "camera.h"
#include "crusys.h"
#include "dispatch.h"
#include "gamepal.h"
#include "hittest.h"
#include "item.h"
#include "itemdata.h"
#include "npc.h"
#include "shaphand.h"
#include "type.h"
#include "viewport.h"
#include "wpncache.h"

u8 &showItemBoxes = gameGlobal<u8>(0);
u8 &unknown3631 = gameGlobal<u8>(0);
/* hit tests the box instead of the frame; nothing sets it. */
u8 &hitByBox = gameGlobal<u8>(0);
u8 &skipIgnored = gameGlobal<u8>(1);
/* The item redrawItem put off. */
u16 &pendingItem = gameGlobal<u16>(0);
u8 &updateSet = gameGlobal<u8>(0);
u8 &dirtySet = gameGlobal<u8>(0);
u16 *&itemNodes = gameGlobal<u16 *>(nullptr);
ScreenArea &updateArea = gameGlobal<ScreenArea>();
ScreenArea &dirtyArea = gameGlobal<ScreenArea>();

/* The boxes overlap when their projections on the three screen axes (x-z,
 * y-z and x-y) do. Then the node lying wholly beyond the other on an axis,
 * or else the one with the greater sum, is drawn later. */
i16 DL_ItemNode::order(u16 other)
{
    if (kind != NODE_ITEM) {
        return 0;
    }
    Item otherItem;
    i16 otherWidth;
    i16 otherDepth;
    i16 otherHeight;
    otherItem = ItemRef((static_cast<DL_ItemNode *>(nodeOf(other)))->item);
    const i16 otherX = itemX[otherItem.id] >> 1;
    const i16 otherY = itemY[otherItem.id] >> 1;
    const i16 otherZ = itemZ[otherItem.id];
    if ((itemFlags[otherItem.id] & ITEM_FLIPPED) != 0) {
        halfBox(otherDepth, otherWidth, otherHeight, &typeFlags.infos[itemShape[otherItem.id]]);
    } else {
        halfBox(otherWidth, otherDepth, otherHeight, &typeFlags.infos[itemShape[otherItem.id]]);
    }
    bool overlap;
    if (static_cast<i16>(this->x - this->z - this->width - this->height) >= static_cast<i16>(otherX - otherZ) ||
        static_cast<i16>(this->x - this->z) <= static_cast<i16>(otherX - otherZ - otherWidth - otherHeight)) {
        overlap = false;
    } else if (static_cast<i16>(this->y - this->z - this->depth - this->height) >= static_cast<i16>(otherY - otherZ) ||
               static_cast<i16>(this->y - this->z) <= static_cast<i16>(otherY - otherZ - otherDepth - otherHeight)) {
        overlap = false;
    } else if (static_cast<i16>(screenX - this->width) >= static_cast<i16>(otherX - otherY + otherDepth) ||
               static_cast<i16>(screenX + this->depth) <= static_cast<i16>(otherX - otherY - otherWidth)) {
        overlap = false;
    } else {
        overlap = true;
    }
    if (overlap) {
        if (static_cast<i16>(this->x - this->width) >= otherX) {
            return 1;
        }
        if (this->x <= static_cast<i16>(otherX - otherWidth)) {
            return 2;
        }
        if (static_cast<i16>(this->y - this->depth) >= otherY) {
            return 1;
        }
        if (this->y <= static_cast<i16>(otherY - otherDepth)) {
            return 2;
        }
        if (this->z >= static_cast<i16>(otherZ + otherHeight) && static_cast<i16>(this->z + this->height) > otherZ) {
            return 1;
        }
        if (static_cast<i16>(this->z + this->height) <= otherZ && this->z < static_cast<i16>(otherZ + otherHeight)) {
            return 2;
        }
    }
    if (overlap) {
        if (static_cast<i16>(this->x - this->width + this->y - this->depth + this->z + this->z) >
            static_cast<i16>(otherX - otherWidth + otherY - otherDepth + otherZ + otherZ)) {
            return 1;
        }
        if (static_cast<i16>(this->x - this->width + this->y - this->depth + this->z + this->z) <
            static_cast<i16>(otherX - otherWidth + otherY - otherDepth + otherZ + otherZ)) {
            return 2;
        }
    }
    return 0;
}

void DL_ItemNode::draw()
{
    Item nodeItem;
    ShapeInfo *info;
    i16 halfWidth;
    i16 halfDepth;
    i16 halfHeight;

    nodeItem = ItemRef(this->item);
    const u16 shape = itemShape[nodeItem.id];
    info = &typeFlags.infos[shape];
    if (info->editor) {
        return;
    }
    const u16 frame = nodeItem.frame();
    ShapeFrame *image = cachedShapeFrame(shape, frame);
    if (image != nullptr) {
        const u16 nodeFlags = itemFlags[nodeItem.id];
        if ((nodeFlags & ITEM_FLIPPED) != 0) {
            if ((nodeFlags & ITEM_INVISIBLE) != 0) {
                drawView.drawShapeMirroredTranslucent(screenX - scrollX, screenY - scrollY, image,
                                                      paletteTables->translucent.get());
            } else if (info->translucent) {
                drawView.drawShapeMirroredRemapped(screenX - scrollX, screenY - scrollY, image, paletteTables->remap);
            } else {
                drawView.drawShapeMirrored(screenX - scrollX, screenY - scrollY, image);
            }
        } else if ((nodeFlags & ITEM_INVISIBLE) != 0) {
            drawView.drawShapeTranslucent(screenX - scrollX, screenY - scrollY, image,
                                          paletteTables->translucent.get());
        } else if (info->translucent) {
            drawView.drawShapeRemapped(screenX - scrollX, screenY - scrollY, image, paletteTables->remap);
        } else {
            drawView.drawShape(screenX - scrollX, screenY - scrollY, image);
        }
    }
    if (nodeItem.isSilencer() &&
        ((controlledNpc.isInCombat() != 0U) || controlledNpc.lastAnimSet() == ANIM_UNREADY_SMALL)) {
        Npc npc(nodeItem.id);
        Npc weapon(0);
        weapon = Item(npc.activeWeapon());
        if (weapon.isValid() && hasWeaponOverlay(weapon.shape(), npc.lastAnimSet())) {
            u16 overlayShape;
            u16 overlayFrame;
            char dx;
            char dy;
            weaponOverlay(weapon.shape(), npc.lastAnimSet(), npc.direction(), npcs[npc.id].animFrame, &overlayShape,
                          &dx, &dy, &overlayFrame);
            if ((overlayShape != 0U) || (overlayFrame != 0U)) {
                image = cachedShapeFrame(overlayShape, overlayFrame);
                drawView.drawShape(screenX - scrollX + dx, screenY - scrollY + dy, image);
            }
        }
    }
    if (showItemBoxes != 0U) {
        if ((itemFlags[nodeItem.id] & ITEM_FLIPPED) != 0) {
            halfBox(halfDepth, halfWidth, halfHeight, &typeFlags.infos[shape]);
        } else {
            halfBox(halfWidth, halfDepth, halfHeight, &typeFlags.infos[shape]);
        }
        const i16 cornerX = screenX - scrollX;
        const i16 cornerY = screenY - scrollY;
        Viewport *view = &drawView;
        view->line(cornerX, cornerY, cornerX + halfDepth, cornerY - (halfDepth >> 1), 15);
        view->line(cornerX, cornerY, cornerX - halfWidth, cornerY - (halfWidth >> 1), 15);
        view->line(cornerX, cornerY, cornerX, cornerY - halfHeight, 15);
        view->line(cornerX + halfDepth, cornerY - (halfDepth >> 1), cornerX + halfDepth,
                   cornerY - (halfDepth >> 1) - halfHeight, 15);
        view->line(cornerX - halfWidth, cornerY - (halfWidth >> 1), cornerX - halfWidth,
                   cornerY - (halfWidth >> 1) - halfHeight, 15);
        view->line(cornerX, cornerY - halfHeight, cornerX + halfDepth, cornerY - (halfDepth >> 1) - halfHeight, 15);
        view->line(cornerX, cornerY - halfHeight, cornerX - halfWidth, cornerY - (halfWidth >> 1) - halfHeight, 15);
        view->line(cornerX - halfWidth + halfDepth, cornerY - (halfWidth >> 1) - (halfDepth >> 1) - halfHeight,
                   cornerX + halfDepth, cornerY - (halfDepth >> 1) - halfHeight, 15);
        view->line(cornerX - halfWidth + halfDepth, cornerY - (halfWidth >> 1) - (halfDepth >> 1) - halfHeight,
                   cornerX - halfWidth, cornerY - (halfWidth >> 1) - halfHeight, 15);
    }
}

void DL_ItemNode::place()
{
    i16 left;
    i16 top;
    i16 right;
    i16 bottom;
    Item nodeItem;
    ShapeFrame *image;
    FrameRect frame;

    nodeItem = ItemRef(this->item);
    x = itemX[nodeItem.id] >> 1;
    y = itemY[nodeItem.id] >> 1;
    z = itemZ[nodeItem.id];
    screenX = x - y;
    screenY = (x >> 1) + (y >> 1) - z;
    if ((itemFlags[nodeItem.id] & ITEM_FLIPPED) != 0) {
        halfBox(depth, width, height, &typeFlags.infos[itemShape[nodeItem.id]]);
    } else {
        halfBox(width, depth, height, &typeFlags.infos[itemShape[nodeItem.id]]);
    }
    image = cachedShapeFrame(itemShape[nodeItem.id], nodeItem.frame());
    if (image != nullptr) {
        shapeCache->frameBounds(itemShape[nodeItem.id], nodeItem.frame(), &frame, 0, 0);
        if ((itemFlags[nodeItem.id] & ITEM_FLIPPED) != 0) {
            left = screenX - frame.right;
            top = screenY + frame.top;
            right = screenX - frame.left;
            bottom = screenY + frame.bottom;
        } else {
            left = screenX + frame.left;
            top = screenY + frame.top;
            right = screenX + frame.right;
            bottom = screenY + frame.bottom;
        }
        if (nodeItem.isSilencer() &&
            ((controlledNpc.isInCombat() != 0U) || controlledNpc.lastAnimSet() == ANIM_UNREADY_SMALL)) {
            Npc weapon(0);
            weapon = Item(controlledNpc.activeWeapon());
            if (weapon.isValid() && hasWeaponOverlay(weapon.shape(), controlledNpc.lastAnimSet())) {
                u16 overlayShape;
                u16 overlayFrame;
                char dx;
                char dy;
                weaponOverlay(weapon.shape(), controlledNpc.lastAnimSet(), controlledNpc.direction(),
                              npcs[controlledNpc.id].animFrame, &overlayShape, &dx, &dy, &overlayFrame);
                if ((overlayShape != 0U) || (overlayFrame != 0U)) {
                    FrameRect overlay;
                    shapeCache->frameBounds(overlayShape, overlayFrame, &overlay, 0, 0);
                    const i16 overlayX = screenX + dx;
                    const i16 overlayY = screenY + dy;
                    overlay.left += overlayX;
                    overlay.top += overlayY;
                    overlay.right += overlayX;
                    overlay.bottom += overlayY;
                    /* The frame bounds are unsigned; these compare them as
                     * signed. */
                    if (static_cast<i16>(overlay.left) < left) {
                        left = overlay.left;
                    }
                    if (static_cast<i16>(overlay.right) > right) {
                        right = overlay.right;
                    }
                    if (static_cast<i16>(overlay.top) < top) {
                        top = overlay.top;
                    }
                    if (static_cast<i16>(overlay.bottom) > bottom) {
                        bottom = overlay.bottom;
                    }
                }
            }
        }
    } else {
        left = 0;
        top = 0;
        right = 1;
        bottom = 1;
    }
    bounds = Rect(left, top, right, bottom);
}

void DL_ItemNode::detach() const
{
    Item nodeItem;

    nodeItem = this->item;
    nodeItem.andFlags(~ITEM_DRAWN);
    itemNodes[nodeItem.id] = 0;
    clearItemBit(screenItems, this->item);
    clearItemBit(movedItems, this->item);
}

/* The node of item among the cells left..right, top..bottom. */
/* UNCALLED: nothing calls it. */
u16 findItemNode(u16 item, i16 left, i16 top, i16 right, i16 bottom)
{
    i16 row;
    i16 column;
    DL_Node *cell;
    DL_ItemNode *entry;

    for (row = top; row <= bottom; row++) {
        for (column = left; column <= right; column++) {
            if (column >= 0 && column < GRID_COLUMNS && row >= 0 && row < GRID_ROWS) {
                cell = nodeOf(gridCells[row][column]);
                for (u16 edge = cell->edgesIn; edge != 0U; edge = edgeOf(edge)->nextIn) {
                    const u16 over = edgeOf(edge)->first;
                    entry = static_cast<DL_ItemNode *>(nodeOf(over));
                    if (entry->kind == NODE_ITEM && entry->item == item) {
                        return over;
                    }
                }
            }
        }
    }
    return 0;
}

/* The original returns nothing when the item is not drawn or the number is out
 * of range after the halt. No Remorse's original keeps no map from items to
 * nodes and looks for the node in the cells the item's box covers, then in all
 * of them; the port keeps No Regret's map in both games. */
u16 itemNode(u16 item)
{
    Item entry;

    entry = ItemRef(item);
    if ((itemFlags[entry.id] & ITEM_DRAWN) != 0) {
        if (item > 0 && item < 0x2800) {
            return itemNodes[item];
        }
        haltMessage("Invalid referent to DL_ItemNode::find");
    }
    /* The original ends without a value for an item not drawn: AX still
     * holds the item's number doubled, the index into itemFlags. */
    return static_cast<u16>(entry.id << 1);
}

void updateItem(u16 item)
{
    ItemRef entry(item);
    Rect area(0, 0, 0, 0);

    if (pendingItem == item) {
        pendingItem = 0;
        return;
    }
    if ((entry.flags() & ITEM_DRAWN) != 0) {
        const u16 node = itemNode(item);
        if (node != 0U) {
            itemNodes[item] = 0;
            area = nodeOf(node)->bounds;
            area.left = area.left - scrollX;
            area.top = area.top - scrollY;
            area.right = area.right - scrollX;
            area.bottom = area.bottom - scrollY;
            if ((dirtySet != 0U) && area.touches(screenRect) && !area.touches(dirtyArea)) {
                flushDirty();
            }
            deleteNode(node);
            if (area.touches(screenRect)) {
                clipRect(&area, &screenRect);
                const i16 left = area.left;
                const i16 top = area.top;
                const i16 right = area.right;
                const i16 bottom = area.bottom;
                if (worldActive == 0U) {
                    drawViewport->clear(0x58, left, top, right, bottom);
                }
                if (mapDrawing != 0U) {
                    addDirtyRect(Rect(left, top, right, bottom));
                }
                ShapeInfo *info = &typeFlags.infos[entry.shape()];
                if ((worldActive == 0U) || info->bag || item == 1) {
                    gameCamera->invalidate(Rect(left, top, right, bottom), 0);
                } else if (dirtySet != 0U) {
                    mergeRect(&dirtyArea, &area);
                } else {
                    dirtyArea.left = area.left;
                    dirtyArea.top = area.top;
                    dirtyArea.right = area.right;
                    dirtyArea.bottom = area.bottom;
                    dirtySet = 1;
                }
                if (updateSet != 0U) {
                    mergeRect(&updateArea, &area);
                } else {
                    updateArea.left = area.left;
                    updateArea.top = area.top;
                    updateArea.right = area.right;
                    updateArea.bottom = area.bottom;
                    updateSet = 1;
                }
            }
        }
        entry.andFlags(~ITEM_DRAWN);
    }
}

void redrawItem(u16 item)
{
    if (pendingItem != 0U) {
        updateNodes();
    }
    pendingItem = item;
}

void updateNodes()
{
    DL_ItemNode *entry;
    Rect view(0, 0, 0, 0);
    Rect area(0, 0, 0, 0);

    if (pendingItem == 0U) {
        return;
    }
    ItemRef item(pendingItem);
    if ((dirtySet != 0U) && typeFlags.infos[item.shape()].bag) {
        flushDirty();
    }
    const i16 left = viewX;
    const i16 top = viewY;
    view = Rect(left, top, left + (GRID_COLUMNS * GRID_CELL) - 1, top + (GRID_ROWS * GRID_CELL) - 1);
    const u16 node = nodeOffset(new DL_ItemNode);
    if (node != 0U) {
        itemNodes[pendingItem] = node;
        entry = static_cast<DL_ItemNode *>(nodeOf(node));
        entry->kind = NODE_ITEM;
        entry->covers = static_cast<u8>(((item.flags() & ITEM_INVISIBLE) != 0) ||
                                        (typeFlags.infos[item.shape()].translucent != 0U));
        entry->attach(pendingItem);
        if (entry->bounds.touches(view)) {
            area = entry->bounds;
            area.left = area.left - scrollX;
            area.top = area.top - scrollY;
            area.right = area.right - scrollX;
            area.bottom = area.bottom - scrollY;
            if ((dirtySet != 0U) && area.touches(screenRect) && !area.touches(dirtyArea)) {
                flushDirty();
            }
            if (mapDrawing != 0U) {
                addDirtyRect(Rect(area.left, area.top, area.right, area.bottom));
            }
            item.orFlags(ITEM_DRAWN);
            markChanged(node);
            if (linkNewNode(node) == 0U) {
                itemNodes[pendingItem] = 0;
                item.andFlags(~ITEM_DRAWN);
                unlinkNode(node);
                delete entry;
            } else if ((item.flags() & ITEM_DRAWN) != 0) {
                if ((entry->flags & NODE_COVERED) != 0) {
                    changeArea(&entry->bounds);
                }
                if (area.touches(screenRect)) {
                    clipRect(&area, &screenRect);
                    if (dirtySet != 0U) {
                        mergeRect(&dirtyArea, &area);
                    } else {
                        dirtyArea.left = area.left;
                        dirtyArea.top = area.top;
                        dirtyArea.right = area.right;
                        dirtyArea.bottom = area.bottom;
                        dirtySet = 1;
                    }
                    Rect screen(entry->bounds.left - scrollX, entry->bounds.top - scrollY,
                                entry->bounds.right - scrollX, entry->bounds.bottom - scrollY);
                    screen.intersect(drawViewport->bounds());
                    if (updateSet != 0U) {
                        mergeRect(&updateArea, &lvalue(Rect(&screen)));
                    } else {
                        updateArea = ScreenArea(&screen);
                        updateSet = 1;
                    }
                }
            }
        } else {
            itemNodes[pendingItem] = 0;
            delete entry;
        }
    }
    pendingItem = 0;
}

void flushDirty()
{
    if (dirtySet != 0U) {
        drawArea = dirtyArea;
        drawView = *drawViewport;
        drawView.setBounds(Rect(dirtyArea.left, dirtyArea.top, dirtyArea.right, dirtyArea.bottom));
        relink = 1;
        drawChanged();
        dirtySet = 0;
    }
}

/* UNCALLED: nothing calls it. */
void invalidateItems(Rect area)
{
    Rect clipped(area.left, area.top, area.right, area.bottom);

    clipRect(&clipped, &screenRect);
    if (dirtySet != 0U) {
        mergeRect(&dirtyArea, &clipped);
    } else {
        dirtyArea.left = clipped.left;
        dirtyArea.top = clipped.top;
        dirtyArea.right = clipped.right;
        dirtyArea.bottom = clipped.bottom;
        dirtySet = 1;
    }
    if (updateSet != 0U) {
        mergeRect(&updateArea, &clipped);
    } else {
        updateArea.left = clipped.left;
        updateArea.top = clipped.top;
        updateArea.right = clipped.right;
        updateArea.bottom = clipped.bottom;
        updateSet = 1;
    }
}

void paintUpdate()
{
    if (updateSet != 0U) {
        guiDispatcher->paint(Rect(updateArea.left, updateArea.top, updateArea.right, updateArea.bottom));
        updateSet = 0;
    }
}

bool DL_ItemNode::hit(i16 pointX, i16 pointY)
{
    const Npc nodeItem(this->item);
    ShapeInfo *info;
    u16 shape;

    shape = itemShape[nodeItem.id];
    info = &typeFlags.infos[shape];
    if ((skipIgnored != 0U) && info->ignore) {
        return false;
    }
    if (hitByBox != 0U) {
        i16 halfWidth;
        i16 halfDepth;
        i16 halfHeight;
        pointX -= screenX;
        pointY -= screenY;
        if ((itemFlags[nodeItem.id] & ITEM_FLIPPED) != 0) {
            halfBox(halfDepth, halfWidth, halfHeight, info);
        } else {
            halfBox(halfWidth, halfDepth, halfHeight, info);
        }
        if (-halfWidth <= pointX && pointX <= halfDepth &&
            static_cast<i16>(-halfHeight - (halfWidth >> 1) - (halfDepth >> 1)) <= pointY && pointY <= 0) {
            i16 side = pointY + (pointX >> 1);
            if (side <= 0 && static_cast<i16>(-halfWidth - halfHeight) <= side) {
                side = pointY - (pointX >> 1);
                if (side <= 0 && static_cast<i16>(-halfDepth - halfHeight) <= side) {
                    return true;
                }
            }
        }
    } else if (bounds.touches(Rect(pointX, pointY))) {
        Point origin;
        Point at;
        origin = Point(screenX, screenY);
        ShapeFrame *frame = cachedShapeFrame(itemShape[nodeItem.id], nodeItem.frame());
        if (frame != nullptr) {
            if ((nodeItem.flags() & ITEM_FLIPPED) != 0) {
                at = Point(screenX - (pointX - screenX), pointY);
            } else {
                at = Point(pointX, pointY);
            }
            return frameHit(frame, origin, at);
        }
    }
    return false;
}

/* skipIgnored; nothing calls it. */
/* UNCALLED: nothing calls it. */
u8 ignoredSkipped()
{
    return skipIgnored;
}
