/* IRGUMP.C (13b8:0000-09ad, No Remorse only). See irgump.h. */
#include "irgump.h"
#include "camera.h"
#include "dispatch.h"
#include "gumpshap.h"
#include "item.h"
#include "itemutil.h"
#include "kernel.h"
#include "npc.h"

/* UNCALLED: nothing calls it. */
GumpNotifyProcess::GumpNotifyProcess(u16 seconds, NewGump *gump, u16 newItem, i16 newType, u8 flag)
    : WaitProcess(static_cast<ui16>(seconds * 60), 0)
{
    NewGump *g;
    i16 i;

    setItem(newItem);
    setType(newType);
    status &= ~1;
    unknown72 = flag;
    for (i = -1, g = gump; g != nullptr; g = g->parent) {
        i++;
    }
    depth = i;
    for (g = gump; i >= 0; g = g->parent) {
        path[i--] = g->id;
    }
    waiting = 1;
}

void GumpNotifyProcess::onKill()
{
    if (waiting != 0U) {
        ItemRelativeGump *gump = static_cast<ItemRelativeGump *>(guiDispatcher->find(path, depth));

        if (gump != nullptr) {
            gump->notifier = 0;
            gump->close();
        }
    }
}

void GumpNotifyProcess::closeGump()
{
    NewGump *gump = guiDispatcher->find(path, depth);

    if (gump != nullptr) {
        waiting = 0;
        gump->close();
    }
}

void GumpNotifyProcess::restart()
{
    waiting = 1;
    WaitProcess::restart();
}

/* UNCALLED: nothing calls it. */
ItemRelativeGump::ItemRelativeGump(u16 newLayer, NewGump *newParent, Rect area, u16 newItem, i16 newMode, i16 newDx,
                                   i16 newDy, u16 seconds)
    : DialogGump(NewGumpId(), newParent, area, guiColors->face)
{
    /* The loop sets it: with all 256 taken, the halt does not come back. */
    u8 instance = 0;
    NewGump *child;
    char used[256];
    i16 i;

    memset(used, 0, sizeof used);
    child = nullptr;
    if (newParent != nullptr) {
        while (newParent->children.findNextByClassId(GUMP_ITEM_RELATIVE, &child) != nullptr) {
            used[child->id.instance()] = 1;
        }
    }
    for (i = 0; i < 256; i++) {
        if (used[i] == 0) {
            instance = i;
            break;
        }
    }
    if (i == 256) {
        halt();
    }
    id = NewGumpId(GUMP_ITEM_RELATIVE, instance);
    layer = newLayer;
    item = newItem;
    dx = newDx;
    dy = newDy;
    mode = newMode;
    switch (mode) {
    case IRGUMP_ITEM:
    case IRGUMP_ITEM_MOVING:
        point.x = Item(item).x();
        point.y = Item(item).y();
        point.z = Item(item).z();
        break;
    case IRGUMP_NPC:
        point.x = controlledNpc.x();
        point.y = controlledNpc.y();
        point.z = controlledNpc.z();
        break;
    case IRGUMP_CAMERA:
        point.x = cameraX();
        point.y = cameraY();
        point.z = cameraZ();
        dy += point.z;
    }
    ticks = seconds << 2;
    if (seconds != 0U) {
        GumpNotifyProcess *process = new GumpNotifyProcess(seconds, this, item, 0x200, 0);

        process->setItem(item);
        process->resume();
        notifier = process->pid;
    } else {
        notifier = 0;
    }
}

/* The search for another gump of the item finds nothing it uses (the
 * original's). */
ItemRelativeGump::~ItemRelativeGump()
{
    ItemRelativeGump *other = nullptr;

    if (parent != nullptr) {
        while (parent->children.findNextByClassId(GUMP_ITEM_RELATIVE, reinterpret_cast<NewGump **>(&other)) !=
               nullptr) {
            if (other->item == item) {
                break;
            }
        }
    }
}

void ItemRelativeGump::close()
{
    if (notifier != 0U) {
        GumpNotifyProcess *process = static_cast<GumpNotifyProcess *>(findProcess(notifier));

        if ((static_cast<u8>(process != nullptr)) != 0U) {
            process->waiting = 0;
            process->terminate(process->result);
        }
        notifier = 0;
    }
    NewGump::close();
}

/* UNCALLED: nothing calls it. */
void updateItemGumps(u16 item, i16 show)
{
    ItemRelativeGump *gump = nullptr;
    NewGump *top = modals[modalTop];

    while (top->children.findNextByClassId(GUMP_ITEM_RELATIVE, reinterpret_cast<NewGump **>(&gump)) != nullptr) {
        if ((item == 0U) || gump->item == item) {
            gump->update(show);
        }
    }
    /* Walks the item's contents and does nothing with them (the original's). */
    const Item container(item);
    if (container.isValid()) {
        for (Item content(container.contents()); content.isValid(); content = content.next()) {
            ;
        }
    }
}

/* UNCALLED: nothing calls it. */
void raiseItemGumps()
{
    GumpLink *previous = nullptr;
    ItemRelativeGump *gump = nullptr;
    NewGump *top = modals[modalTop];

    while (top->children.backward(reinterpret_cast<NewGump **>(&gump))) {
        if (gump->id.classId() == GUMP_ITEM_RELATIVE && gump->layer == 2) {
            previous = gump->previous;
            top->children.raiseChild(gump);
            /* Raising the first gump ends the walk (the original's). */
            if (previous == nullptr) {
                break;
            }
            gump = static_cast<ItemRelativeGump *>(previous->next);
        }
    }
}

/* UNCALLED: nothing calls it. */
void closeItemGumps(u16 item)
{
    ItemRelativeGump *gump = nullptr;
    NewGump *top = modals[modalTop];

    while (top->children.findNextByClassId(GUMP_ITEM_RELATIVE, reinterpret_cast<NewGump **>(&gump)) != nullptr) {
        if (gump->item == item) {
            gump->close();
            gump = nullptr;
        }
    }
}

void ItemRelativeGump::update(i16 show)
{
    const Item self(item);

    if (show == 0) {
        pushPaintState(3);
        invalidate();
        return;
    }
    switch (mode) {
    case IRGUMP_ITEM_MOVING:
        point.x = Item(item).x();
        point.y = Item(item).y();
        point.z = Item(item).z();
        break;
    case IRGUMP_NPC:
        point.x = controlledNpc.x();
        point.y = controlledNpc.y();
        point.z = controlledNpc.z();
        break;
    case IRGUMP_CAMERA:
        paintState >>= 2;
        paint(0);
        return;
    }
    {
        i16 x = 0;
        i16 y = 0;

        mapToScreen(point.x, point.y, &x, &y);
        y -= point.z;
        bounds.moveTo(x + dx, y + dy);
    }
    paintState >>= 2;
    paint(0);
}

void ItemRelativeGump::handleBothButtons(NewGumpEvent * /*unused*/) {}

void ItemRelativeGump::handleRightButton(NewGumpEvent * /*unused*/) {}

/* UNCALLED: nothing calls it. */
void ItemRelativeGump::unknown0979() {}

/* UNCALLED: nothing calls it. */
void ItemRelativeGump::unknown097e() {}
