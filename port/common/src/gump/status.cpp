/* Code58 (No Regret's 11c8, No Remorse's 11b0, file name unknown; the
 * addresses are No Regret's): the status display, the games' differences as
 * each game's values (status_game.h). See status.h. */
#include "status.h"
#include "gamestate.h"
#include "status_game.h"
#include <cstdio>
#include "basecam.h"
#include "cafont.h"
#include "control.h"
#include "dtable.h"
#include "item.h"
#include "itemdata.h"
#include "mousectl.h"
#include "npc.h"
#include "type.h"
#include "u9mouse.h"
#include "input.h"
#include "shapes.h"
#include "screen.h"

u8 &statusShown = gameGlobal<u8>(0);

/* Nothing reads it (1480:36b5). */
u8 &unknown36b5 = gameGlobal<u8>(0);
StatusPart (&statusParts)[STATUS_PARTS] = gameGlobal<StatusPart[STATUS_PARTS]>(); /* 1480:9038 */

/* The screen under each part, kept to erase it (1480:9079). */
Viewport *(&statusViews)[STATUS_PARTS] = gameGlobal<Viewport *[STATUS_PARTS]>();

/* The frame of each part's background, gump shape 1 (1480:36ab). */
u16 (&statusFrames)[STATUS_PARTS] = gameGlobal<u16[STATUS_PARTS]>({0, 1, 0, 2, 3});

void closeStatus()
{
    for (i16 i = 0; i < STATUS_PARTS; i++) {
        delete statusParts[i].background;
        statusParts[i].background = nullptr;
        delete statusViews[i];
        statusViews[i] = nullptr;
    }
}

void initStatus()
{
    /* Clears four of the five parts (the original's). */
    memset(static_cast<void *>(statusParts), 0, 4 * sizeof(StatusPart));
    statusShown = 0;
    if (statusShown != 0U) {
        hideStatusGump();
        showStatusGump();
    }
    for (i16 i = 0; i < STATUS_PARTS; i++) {
        StatusPart *part = &statusParts[i];
        /* The width is the old one, not yet the background's (the
         * original's). */
        const i16 gap = static_cast<i16>(SCREEN_WIDTH - part->area.width()) / 5;
        part->changed = 1;
        part->background = new GumpShape(1);
        part->background->setFrame(statusFrames[i]);
        part->background->paintMode = STATUS_PAINT_MODE;
        part->area = part->background->dimensions();
        const i16 x = (gap >> 3) + (gap * i);
        const i16 y = 450;
        part->area.moveTo(x, y);
        statusViews[i] =
            new Viewport(VIEWPORT_LARGE, 0, 0, part->area.right - part->area.left, part->area.bottom - part->area.top);
    }
}

namespace {
/* The name, gump shape and frame that show item, all 0 when it has none
 * (11c8:000c). A copy of DISPLAY.C's pickupInfo without the halts. */
void statusInfo(u16 id, const char **name, u16 *shape, u16 *frame)
{
    *shape = 0;
    *frame = 0;
    *name = nullptr;
    if (id == 0) {
        return;
    }
    const Item item(id);
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
    }
}
}

void drawStatus()
{
    if (statusShown == 0U) {
        return;
    }
    RectLink *link = nullptr;
    while (dirtyRects.forward(reinterpret_cast<GumpLink **>(&link))) {
        for (i16 i = 0; i < STATUS_PARTS; i++) {
            StatusPart *part = &statusParts[i];
            if (part->changed == 0U) {
                const Rect dirty = link->bounds;
                if (part->area.overlaps(Rect(dirty.left, dirty.top, dirty.right, dirty.bottom))) {
                    part->changed = 1;
                }
            }
        }
    }
    for (i16 index = 0; index < STATUS_PARTS; index++) {
        if (statusParts[index].changed != 0U) {
            StatusPart *part = &statusParts[index];
            statusViews[index]->blit(drawViewport, part->area.left, part->area.top, part->area.right,
                                     part->area.bottom);
            if (index == STATUS_WEAPON) {
                if (controlledActor == 1) {
                    const u16 weapon = Npc(controlledActor).activeWeapon();
                    if (weapon != 0U) {
                        part->background->draw(part->area.left, part->area.top);
                    }
                }
            } else if (index == STATUS_AMMO) {
                if (controlledActor == 1) {
                    const Item weapon = Npc(controlledActor).activeWeapon();
                    if ((weapon.id != 0U) && (ammoShape(itemShape[weapon.id]) != 0U)) {
                        part->background->draw(part->area.left, part->area.top);
                    }
                }
            } else if (controlledActor == 1 || index == STATUS_HEALTH) {
                part->background->draw(part->area.left, part->area.top);
            }
            if (controlledActor != 1 && index != STATUS_HEALTH) {
                statusParts[index].changed = 0;
            } else {
                switch (index) {
                case STATUS_WEAPON: {
                    const i16 x = (part->area.width() >> 1) + part->area.left;
                    const i16 y = (part->area.height() >> 1) + part->area.top;
                    const Item weapon = controlledNpc.activeWeapon();
                    if (weapon.isValid()) {
                        const u16 frame = weaponFrame(itemShape[weapon.id]);
                        GumpShape icon(3);
                        icon.setFrame(frame);
                        icon.draw(x, y);
                    }
                    if (STATUS_NAMES) {
                        u16 unused;
                        const char *name;
                        statusInfo(weapon.id, &name, &unused, &unused);
                        if (name != nullptr) {
                            CAFont font(18);
                            const i16 width = font.textWidth(name);
                            font.draw(part->area.right - (width + 4), part->area.top + 20, "%s", name);
                        }
                    }
                    break;
                }
                case STATUS_ITEM: {
                    const i16 x = (part->area.width() >> 1) + part->area.left;
                    const i16 y = (part->area.height() >> 1) + part->area.top;
                    const Item item = controlledNpc.activeItem3();
                    if (item.isValid()) {
                        u16 frame;
                        i16 shape;
                        if (itemShape[item.id] == SHAPE_CREDITS) {
                            frame = 13;
                            shape = 3;
                        } else if (item.family() == FAMILY_BOMB) {
                            frame = bombFrame(itemShape[item.id]);
                            shape = 4;
                        } else {
                            frame = inventoryFrame(itemShape[item.id]);
                            shape = 5;
                        }
                        if (frame == 3 && shape == 5) {
                            if (item.frame() != 0U) {
                                frame = 18;
                            }
                        } else if (frame == 5 && shape == 5) {
                            if (item.frame() != 0U) {
                                frame = 17;
                            }
                        } else if (frame == 7 && shape == 5) {
                            if (item.frame() != 0U) {
                                frame = 19;
                            }
                        }
                        GumpShape icon(shape);
                        icon.setFrame(frame);
                        if (STATUS_NAMES) {
                            [[maybe_unused]] const Rect size = icon.dimensions();
                            icon.draw(x - 28, y);
                        } else {
                            icon.draw(x, y);
                        }
                        const i16 count = item.quality();
                        if (count > 1) {
                            if (STATUS_NAMES) {
                                char text[20];
                                snprintf(text, sizeof(text), "%d", count);
                                CAFont countFont(12);
                                const i16 width = countFont.textWidth(text);
                                countFont.draw(part->area.right - (width + 4), part->area.top + 20, "%s", text);
                            } else {
                                CAFont countFont(12);
                                countFont.draw(part->area.left + 75, part->area.top + 18, "%d", count);
                            }
                        }
                        u16 unused;
                        const char *name = nullptr;
                        if (STATUS_NAMES) {
                            statusInfo(item.id, &name, &unused, &unused);
                        }
                        if (STATUS_NAMES && name != nullptr) {
                            CAFont font(18);
                            i16 width = font.textWidth(name);
                            font.draw(part->area.right - (width + 4), part->area.top + 4, "%s", name);
                            if (item.frame() == 1) {
                                if (item.shape() == SHAPE_DATA_LINK) {
                                    width = font.textWidth("MESSAGE");
                                    font.draw(part->area.right - (width + 4), part->area.top + 10, "MESSAGE");
                                } else {
                                    width = font.textWidth("ACTIVE");
                                    font.draw(part->area.right - (width + 4), part->area.top + 10, "ACTIVE");
                                }
                            }
                        }
                    }
                    break;
                }
                case STATUS_AMMO: {
                    const Item weapon = controlledNpc.activeWeapon();
                    if (weapon.isValid() && (ammoShape(itemShape[weapon.id]) != 0U)) {
                        CAFont font(STATUS_AMMO_FONT);
                        font.draw(part->area.left + STATUS_AMMO_X, part->area.top + STATUS_AMMO_Y, "%d",
                                  weapon.quality());
                        const Item ammo = controlledNpc.findAmmo();
                        if (ammo.isValid()) {
                            font.draw(part->area.left + STATUS_CLIPS_X, part->area.top + STATUS_AMMO_Y, "%d",
                                      ammo.quality());
                        } else {
                            font.draw(part->area.left + STATUS_CLIPS_X, part->area.top + STATUS_AMMO_Y, "0");
                        }
                    }
                    break;
                }
                case STATUS_HEALTH: {
                    Npc npc(controlledNpcNumber());
                    if (npc.hp() != 0U) {
                        const u16 shape = npc.shape();
                        if (!STATUS_NO_HEALTH_SHAPE || shape != SHAPE_FROZEN) {
                            u16 bar = npc.hp() * STATUS_BAR_LENGTH;
                            const u16 maxHp = npcTypes[npcType(Item(controlledActor).shape())].maxHp;
                            bar /= maxHp;
                            Rect rect;
                            rect.left = part->area.left + STATUS_BAR_X;
                            rect.top = part->area.top + STATUS_BAR_Y;
                            rect.right = rect.left + bar;
                            rect.bottom = rect.top + STATUS_BAR_HEIGHT;
                            drawViewport->clear(0x41, rect.left, rect.top, rect.right, rect.bottom);
                        }
                    }
                    break;
                }
                case STATUS_ENERGY: {
                    Npc npc(controlledNpcNumber());
                    if (npc.mana() != 0U) {
                        u32 bar = static_cast<u32>(npc.mana()) * STATUS_BAR_LENGTH;
                        bar /= 10000;
                        Rect rect;
                        rect.left = part->area.left + STATUS_BAR_X;
                        rect.top = part->area.top + STATUS_BAR_Y;
                        rect.right = rect.left + bar;
                        rect.bottom = rect.top + STATUS_BAR_HEIGHT;
                        drawViewport->clear(0xf5, rect.left, rect.top, rect.right, rect.bottom);
                    }
                    break;
                }
                }
            }
            if (statusParts[index].changed != 0U) {
                addDirtyRect(part->area);
            }
        }
    }
}

/* Intrinsics 137h and 9Fh (11c8:0000, 0006). */
void hideStatusGump()
{
    hideStatus();
}

void showStatusGump()
{
    showStatus();
}

void eraseStatus()
{
    if (statusShown == 0U) {
        return;
    }
    for (i16 i = 0; i < STATUS_PARTS; i++) {
        if (statusParts[i].changed != 0U) {
            StatusPart *part = &statusParts[i];
            drawViewport->blit(statusViews[i], 0, 0, part->area.right - part->area.left,
                               part->area.bottom - part->area.top, part->area.left, part->area.top);
            part->changed = 0;
        }
    }
}

void setStatusShown(u8 shown)
{
    if (shown != 0U) {
        showStatus();
    } else {
        hideStatus();
    }
}

void showStatus()
{
    statusShown = 1;
    updateStatus(STATUS_WEAPON);
    updateStatus(STATUS_AMMO);
    updateStatus(STATUS_ITEM);
    updateStatus(STATUS_HEALTH);
    updateStatus(STATUS_ENERGY);
    inputMouseYRange(0, 439);
    gameMouse->moveTo(320, 240);
}

void hideStatus()
{
    statusShown = 0;
    inputMouseYRange(0, SCREEN_BOTTOM);
    gameMouse->moveTo(320, 240);
}

void updateStatus(i16 part)
{
    if (part <= STATUS_ENERGY && part >= 0) {
        statusParts[part].changed = 1;
    }
}
