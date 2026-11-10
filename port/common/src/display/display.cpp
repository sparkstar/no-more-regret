/* DISPLAY.C (Code53, No Regret's 11a0, No Remorse's 1188; No Regret's
 * addresses): the pickup display. See display.h. */
#include "display.h"
#include "gamestate.h"
#include "display_game.h"
#include "basecam.h"
#include "cafont.h"
#include "dtable.h"
#include "systimer.h"
#include "item.h"
#include "itemdata.h"
#include "type.h"

void agePickups();
void pickupInfo(u16 id, const char **name, u16 *shape, u16 *frame);

u8 &pickupsFrozen = gameGlobal<u8>(0);
Pickup (&pickups)[PICKUPS] = gameGlobal<Pickup[PICKUPS]>(); /* 1480:8ec8, No Remorse's 1478:835a */
/* The screen under each line, kept to erase it (1480:8f90). */
Viewport *(&pickupViews)[PICKUPS] = gameGlobal<Viewport *[PICKUPS]>();

PickupProcess::PickupProcess()
{
    lastTime = systemTimer->time();
    makeSystem();
}

void PickupProcess::run()
{
    const i16 now = systemTimer->time();

    if (now - lastTime < 60) {
        return;
    }
    lastTime = now;
    agePickups();
}

void initPickups()
{
    clearPickups();
    new PickupProcess;
    for (i16 i = 0; i < PICKUPS; i++) {
        pickupViews[i] = new Viewport(VIEWPORT_LARGE, 0, 0, 199, 27);
    }
}

void closePickups()
{
    for (Viewport *&view : pickupViews) {
        delete view;
        view = nullptr;
    }
    for (Pickup &pickup : pickups) {
        delete pickup.gump;
        pickup.gump = nullptr;
        pickup.name = nullptr;
    }
}

void clearPickups()
{
    /* The port's: the shapes of the lines shown, which the original's memset
     * left at a map's jump. */
    for (const Pickup &pickup : pickups) {
        delete pickup.gump;
    }
    memset(static_cast<void *>(pickups), 0, sizeof(pickups));
}

void drawPickups()
{
    if (pickupsFrozen != 0U) {
        return;
    }
    RectLink *link = nullptr;
    CAFont font(13);
    CAFont countFont(12);
    GumpShape background(2);
    background.paintMode = 1;
    while (dirtyRects.forward(reinterpret_cast<GumpLink **>(&link))) {
        for (i16 i = 0; i < PICKUPS; i++) {
            if (pickups[i].name != nullptr) {
                Pickup *pickup = &pickups[i];
                if (pickup->redraw == 0U) {
                    const Rect dirty = link->bounds;
                    if (pickup->area.overlaps(Rect(dirty.left, dirty.top, dirty.right, dirty.bottom))) {
                        pickup->redraw = 1;
                    }
                }
            }
        }
    }
    for (i16 line = 0; line < PICKUPS; line++) {
        if ((pickups[line].name != nullptr) && (pickups[line].redraw != 0U)) {
            Pickup *pickup = &pickups[line];
            pickupViews[line]->blit(drawViewport, pickup->area.left, pickup->area.top, pickup->area.right,
                                    pickup->area.bottom);
            const i16 width = pickup->area.width() - 1;
            for (i16 x = 0; x < width; x += 28) {
                background.draw(pickup->area.left + x, pickup->area.top);
            }
            pickup->gump->draw(pickup->area.left + 33, pickup->area.top + 14);
            if (pickup->count > 1) {
                countFont.draw(pickup->area.left + 53, pickup->area.top + 20, "%d", pickup->count);
            }
            font.draw(pickup->area.left + 66, pickup->area.top + 10, "%s", pickup->name);
            addDirtyRect(pickup->area);
        }
    }
}

void erasePickups()
{
    if (pickupsFrozen == 0) {
        for (i16 i = 0; i < PICKUPS; i++) {
            if ((pickups[i].name != nullptr) && (pickups[i].redraw != 0U)) {
                Pickup *pickup = &pickups[i];
                drawViewport->blit(pickupViews[i], 0, 0, pickup->area.right - pickup->area.left,
                                   pickup->area.bottom - pickup->area.top, pickup->area.left, pickup->area.top);
                pickup->redraw = 0;
            }
        }
    }
}

/* Counts down the lines and frees those whose time ran out (11a0:0414). */
void agePickups()
{
    for (i16 i = 0; i < PICKUPS; i++) {
        if (pickups[i].name != nullptr) {
            pickups[i].timeLeft--;
            if (pickups[i].timeLeft == 0) {
                addDirtyRect(pickups[i].area);
                pickups[i].name = nullptr;
                delete pickups[i].gump;
                pickups[i].gump = nullptr;
            }
        }
    }
}

void addPickup(u16 id)
{
    const Item item(id);

    if (item.isValid()) {
        const char *name;
        u16 shape;
        u16 frame;
        pickupInfo(item.id, &name, &shape, &frame);
        i16 i;
        for (i = 0; i < PICKUPS; i++) {
            if (pickups[i].name != nullptr) {
                const u16 lineShape = pickups[i].shape;
                const u16 lineFrame = pickups[i].frame;
                if (lineShape == shape && lineFrame == frame) {
                    /* Compares the gump shape with the credits' item shape,
                     * so more credits count as one (the original's). */
                    if (shape == SHAPE_CREDITS) {
                        pickups[i].count += item.familyQuality();
                    } else {
                        pickups[i].count++;
                    }
                    pickups[i].redraw = 1;
                    pickups[i].timeLeft = 4;
                    return;
                }
            }
        }
        for (i = 0; i < PICKUPS; i++) {
            if (pickups[i].name == nullptr) {
                Pickup *pickup = &pickups[i];
                const char *newName;
                u16 newShape;
                u16 newFrame;
                pickupInfo(item.id, &newName, &newShape, &newFrame);
                pickup->name = newName;
                pickup->gump = new GumpShape(newShape);
                pickup->gump->setFrame(newFrame);
                pickup->shape = newShape;
                pickup->frame = newFrame;
                pickup->area = pickup->gump->dimensions();
                CAFont font(13);
                pickup->area.right += font.textWidth(newName) + 15;
                i16 columns = pickup->area.right / 28;
                columns += PICKUP_SPARE_COLUMNS;
                pickup->area.right = columns * 28;
                pickup->area.bottom = 28;
                /* The same check as for the credits below, which cannot
                 * fail here. */
                pickup->count = 1;
                if (pickup->count == 0) {
                    pickup->count = 1;
                }
                pickup->area.moveTo(4, (i * 32) + 4);
                pickup->timeLeft = 4;
                pickup->redraw = 1;
                if (item.shape() == SHAPE_CREDITS) {
                    pickup->count = item.familyQuality();
                    if (pickup->count == 0) {
                        pickup->count = 1;
                    }
                }
                return;
            }
        }
    }
}

/* The gump shape, frame and name that show item (11a0:070f). */
void pickupInfo(u16 id, const char **name, u16 *shape, u16 *frame)
{
    if (id == 0) {
        halt();
    }
    const Item item(id);
    *shape = 0;
    *frame = 0;
    *name = nullptr;
    if (item.isValid()) {
        if (item.shape() == SHAPE_KEYCARD) {
            *shape = 5;
            *frame = item.frame() + 13;
            *name = "KEYCARD";
            return;
        }
        if (item.shape() == SHAPE_CREDITS) {
            *shape = 3;
            *frame = 13;
            *name = "CREDITS";
            return;
        }
        if (item.family() == FAMILY_WEAPON) {
            *shape = 3;
            *frame = weaponFrame(itemShape[item.id]);
            *name = weaponName(itemShape[item.id]);
            return;
        }
        if (item.family() == FAMILY_AMMO) {
            *shape = 5;
            *frame = ammoFrame(itemShape[item.id]);
            *name = ammoName(itemShape[item.id]);
            return;
        }
        if (item.family() == FAMILY_BOMB) {
            *shape = 4;
            *frame = bombFrame(itemShape[item.id]);
            *name = bombName(itemShape[item.id]);
            return;
        }
        if (item.family() == FAMILY_INVENTORY) {
            *shape = 5;
            *frame = inventoryFrame(itemShape[item.id]);
            *name = inventoryName(itemShape[item.id]);
            return;
        }
        halt();
    }
}
