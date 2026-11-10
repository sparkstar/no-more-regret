/* BARK.C (1390:0000-0977, No Remorse only). See bark.h. */
#include <cstdlib>
#include "gamestate.h"
#include <cstring>
#include "bark.h"
#include "cafont.h"
#include "dispatch.h"
#include "format.h"
#include "item.h"
#include "itemutil.h"
#include "kernel.h"
#include "shaphand.h"
#include "screen.h"

namespace {
i16 &barkCount = gameGlobal<i16>(0); /* the barks up (1478:54d4) */
/* Nothing reads it (1478:54d6). */
[[maybe_unused]] char unknown54d6[] = " @";
/* The places the barks take on the screen (1478:8aa6). */
Rect (&barkSlots)[BARK_SLOTS] = gameGlobal<Rect[BARK_SLOTS]>();
}

/* UNCALLED: nothing calls it. */
BarkGump::BarkGump(NewGump *newParent, u16 newItem, char *newText, u8 click, i16 time)
    : ItemRelativeGump(2, newParent, Rect(0, 0, 0, 0), newItem, IRGUMP_ITEM_MOVING, 0, 0, time)
{
    if (click != 0U) {
        eventMask |= EVENT_LEFT_BUTTON;
    }
    text = nullptr;
    fontIndex = 4;
    CAFont barkFont(fontIndex);
    bounds = Rect(0, 0, 320, barkFont.glyphHeight(' ') * 10);
    page = 0xffff;
    if (newText != nullptr) {
        newText = formatText(newText, fontIndex, &bounds, 1);
        length = strlen(newText) + 1;
        text = new char[length];
        strcpy(text, newText);
    }
    const Item it(newItem);
    u16 width;
    u16 height;
    s16 xOffset;
    s16 yOffset;
    shapeCache->frameMetrics(it.shape(), it.frame(), &width, &height, &xOffset, &yOffset);
    it.screenPosition(&screenX, &screenY);
    itemArea = Rect(screenX - xOffset, screenY - yOffset, screenX - xOffset + width, screenY - yOffset + height);
    screenY -= static_cast<i16>(height);
    unknown64 = 0;
    slot = -1;
    barkCount++;
    parent->children.raiseChild(this);
    if (newText != nullptr) {
        place();
        advance();
    }
}

BarkGump::~BarkGump()
{
    if (text != nullptr) {
        delete[] text;
    }
    releaseSlot();
    barkCount--;
}

void BarkGump::place()
{
    const i16 width = bounds.right - bounds.left + 1;
    const i16 height = bounds.bottom - bounds.top + 1;
    const i16 maxX = SCREEN_WIDTH - width;
    const i16 maxY = SCREEN_HEIGHT - height;
    i16 bestX = 0;
    i16 bestY = 0;
    u16 best = 0xffff;
    bool free = true;
    i16 x;

    for (x = 0; x < maxX; x += 8) {
        for (i16 y = 0; y < maxY; y += 8) {
            /* The distance from the item to the middle of the place, in two
             * statements and with the middle unsigned: the original's code
             * sets its order. */
            i16 score = abs(static_cast<i16>(screenX - static_cast<u16>(x + (width >> 1))));

            score += abs(static_cast<i16>(screenY - static_cast<u16>(y + (height >> 1))));
            bounds.moveTo(x, y);
            {
                Rect *other = barkSlots;

                for (i16 i = 0; i < barkCount; i++, other++) {
                    if (bounds.intersects(*other)) {
                        score += 3000;
                        free = false;
                    }
                }
            }
            if (bounds.intersects(itemArea)) {
                score += 500;
            }
            if (static_cast<ui16>(score) < best && free) {
                best = score;
                bestX = x;
                bestY = y;
            }
            free = true;
        }
    }
    bounds.moveTo(bestX, bestY);
    slot = -1;
    for (i16 i = 0; i < BARK_SLOTS; i++) {
        if ((barkSlots[i].left == 0) && (barkSlots[i].top == 0) && (barkSlots[i].right == 0) &&
            (barkSlots[i].bottom == 0)) {
            slot = i;
            barkSlots[slot] = bounds;
            break;
        }
    }
    if (slot == -1) {
        halt();
    }
    if (slot < 0) {
        halt();
    }
    const i16 atX = Item(item).x();
    const i16 atY = Item(item).y();
    i16 px;
    i16 py;

    mapToScreen(atX, atY, &px, &py);
    py -= Item(item).z();
    dx = bounds.left - px;
    dy = bounds.top - py;
}

void BarkGump::handleCommand(NewGumpEvent * /*unused*/)
{
    advance();
}

void BarkGump::releaseSlot()
{
    if (slot == -1) {
        return;
    }
    barkSlots[slot].left = 0;
    barkSlots[slot].top = 0;
    barkSlots[slot].right = 0;
    barkSlots[slot].bottom = 0;
    slot = -1;
}

void BarkGump::close()
{
    advance();
}

void BarkGump::advance()
{
    if (text == nullptr) {
        return;
    }
    if (page == 0xffff) {
        page = 0;
    } else {
        page += strlen(text + page) + 1;
        if (page >= length) {
            ItemRelativeGump::close();
            return;
        }
    }
    if (notifier != 0U) {
        WaitProcess *process = static_cast<WaitProcess *>(findProcess(notifier));

        process->restart();
    }
    char *p = text + page;

    while (*p++ != 0) {
        if (*p == static_cast<char>(0xff) || *p == 0) {
            *p = 0;
            break;
        }
    }
    pushPaintState(3);
    invalidate();
    paintState >>= 2;
    CAFont barkFont(fontIndex);
    barkFont.bounds(&bounds, 30, 30, text + page);
    releaseSlot();
    place();
    paint(0);
}

void BarkGump::update(i16 show)
{
    ItemRelativeGump::update(show);
    barkSlots[slot] = bounds;
}

void BarkGump::handleLeftButton(NewGumpEvent *event)
{
    if (!event->isMouseRelease()) {
        advance();
        return;
    }
    event->target = parent;
    parent->handleLeftButton(event);
}

void BarkGump::paintSelf(i16 x, i16 y)
{
    CAFont barkFont(fontIndex);

    /* formatText left no '%' in the text. */
    barkFont.draw(x, y + barkFont.lineHeight(), "%s", text + page);
}

void BarkGump::slot17()
{
    ItemRelativeGump *gump = nullptr;
    NewGump *world = modals[0];

    while (world->children.findNextByClassId(GUMP_ITEM_RELATIVE, reinterpret_cast<NewGump **>(&gump)) != nullptr) {
        if (gump->layer == 2) {
            gump->notifier = 0;
            gump->close();
            gump = nullptr;
        }
    }
    /* The original then tests this, and does nothing either way. */
}

void BarkGump::slot16() {}
