/* Code121 (file name unknown; No Regret's 13c0:0000-0e11, No Remorse's
 * 13d0:0000-0e6c; the addresses are No Regret's): the load and save gump and
 * its slots, the games' differences as each game's values (loadsave_game.h).
 * See loadsave.h. */
#include <cctype>
#include "gamestate.h"
#include <cstring>
#include "loadsave.h"
#include "bmp.h"
#include "gamefile.h"
#include "cafont.h"
#include "dispatch.h"
#include "fade.h"
#include "filespec.h"
#include "kdaemon.h"
#include "msgbox.h"
#include "diskfree.h"
#include "savegame.h"
#include "spmgr.h"
#include "u9mouse.h"
#include "screen.h"

i16 &selectedSlot = gameGlobal<i16>(0);

u16 &savedDelay = gameGlobal<u16>();

u16 &savedRate = gameGlobal<u16>();

namespace {
/* Returns key; what it filtered is gone. */
u8 filterKey(u8 key)
{
    const u8 filtered = key;

    return filtered;
}
}

LoadSaveGump::LoadSaveGump(u8 isLoading) : NewGump(NewGumpId(), nullptr, EVENT_COMMAND)
{
    jumpToBlack();
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    if (LOAD_SAVE_PICTURE) {
        backdrop = gameBackground;
        BmpFile bmp;
        if (isLoading != 0U) {
            bmp.open(makePath(nullptr, "static", "load.bmp", nullptr));
        } else {
            bmp.open(makePath(nullptr, "static", "save.bmp", nullptr));
        }
        bmp.draw(backdrop);
        bmp.getPalette(palette.data);
        bmp.close();
    }
    savedRate = keyDaemon->rate;
    savedDelay = repeatDelay;
    keyDaemon->setRate(10);
    repeatDelay = 20;
    bounds.set(0, 0, drawViewport->right() - drawViewport->left(), drawViewport->bottom() - drawViewport->top());
    this->loading = isLoading;
    i16 x = static_cast<i16>(SCREEN_WIDTH - 417) >> 1;
    i16 y = static_cast<i16>(SCREEN_HEIGHT - 348) >> 1;
    x += SLOTS_X;
    y += SLOTS_Y;
    for (i16 i = 0; i < 11; i++) {
        SlotGump *slot = new SlotGump(i, isLoading, this);

        slot->bounds.moveTo(x, y);
        y += 27;
    }
    if (LOAD_SAVE_PICTURE) {
        palette.data[255].red = 0;
        palette.data[255].green = 0;
        palette.data[255].blue = 0;
        palette.data[0].red = 0;
        palette.data[0].green = 0;
        palette.data[0].blue = 0;
    } else {
        palette.load(makePath(nullptr, staticDirectory, "misc.pal", nullptr), 0);
    }
    palette.apply();
    paint(1);
    gameMouse->pushShape(LOAD_SAVE_POINTER);
    gameMouse->show();
    addAcceptedArguments(-1, 0);
}

LoadSaveGump::~LoadSaveGump()
{
    keyDaemon->setRate(savedRate);
    setRepeatDelay(savedDelay);
    jumpToBlack();
    gameMouse->hide();
    drawViewport->clear(0, 0, 0, SCREEN_RIGHT, 100);
}

void LoadSaveGump::paintSelf(i16 /*x*/, i16 /*y*/)
{
    if (LOAD_SAVE_PICTURE) {
        drawViewport->blit(backdrop);
        return;
    }
    const GumpShape topLeft((loading * 5) + 0x40);
    const GumpShape topRight((loading * 5) + 0x41);
    const GumpShape bottomLeft((loading * 5) + 0x42);
    const GumpShape bottomRight((loading * 5) + 0x43);
    const i16 x = static_cast<i16>(SCREEN_WIDTH - 417) >> 1;
    const i16 y = static_cast<i16>(SCREEN_HEIGHT - 348) >> 1;

    drawViewport->drawShape(x, y, gumpShapes->shapeFrame(topLeft.shape, topLeft.frame));
    drawViewport->drawShape(x + 208, y, gumpShapes->shapeFrame(topRight.shape, topRight.frame));
    drawViewport->drawShape(x, y + 174, gumpShapes->shapeFrame(bottomLeft.shape, bottomLeft.frame));
    drawViewport->drawShape(x + 208, y + 174, gumpShapes->shapeFrame(bottomRight.shape, bottomRight.frame));
}

void LoadSaveGump::handleKeyboard(NewGumpEvent *event)
{
    SlotGump *slot;

    if (event->argument == ASCII_ESCAPE) {
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 0L, this, parent, 0L)));
        close();
        return;
    }
    switch (event->argument) {
    case '2':
    case KEYCODE_SCAN | SCAN_DOWN:
        if (selectedSlot < 10) {
            slot = static_cast<SlotGump *>(children.findByInstanceId(selectedSlot + 1));
            slot->select();
            return;
        }
        break;
    case '8':
    case KEYCODE_SCAN | SCAN_UP:
        if (selectedSlot != 0) {
            slot = static_cast<SlotGump *>(children.findByInstanceId(selectedSlot - 1));
            slot->select();
            return;
        }
        break;
    case '\r':
        slot = static_cast<SlotGump *>(children.findByInstanceId(selectedSlot));
        slot->activate();
        break;
    }
}

void LoadSaveGump::handleCommand(NewGumpEvent *event)
{
    const i16 slot = event->sender->id.instance();

    if (loading == 0U) {
        if (diskFree() < static_cast<s32>(500000L)) {
            /* The command after the box has closed. */
            openModalGump(new MessageBoxGump(NewGumpId(), nullptr,
                                             "ERROR:\n\nUnable to save game.\n\nPlease free up 5 Megs of drive space.",
                                             guiColors->face),
                          static_cast<u32>(-1), [this](s32 /*result*/) {
                              sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 0L, this, parent, 0L)));
                          });
        } else {
            setSaveSlot(slot);
            sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 1L, this, parent, 0L)));
        }
    } else if (strlen(saveDescription(slot)) > 0) {
        setSaveSlot(slot);
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 1L, this, parent, 0L)));
    } else {
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 0L, this, parent, 0L)));
    }
    close();
}

SlotGump::SlotGump(i16 index, u8 isLoading, NewGump *parentGump)
    : NewGump(NewGumpId(GUMP_BUTTON, index), parentGump, EVENT_LEFT_BUTTON), GumpShape(0x44)
{
    bounds = dimensions();
    editing = 0;
    this->loading = isLoading;
    text = std::make_unique_for_overwrite<char[]>(0x20);
    memset(text.get(), 0, 0x20);
    addAcceptedArguments(-1, 0);
}

/* text frees itself. */
SlotGump::~SlotGump() = default;

void SlotGump::handleKeyboard(NewGumpEvent *event)
{
    if (editing == 0U) {
        parent->handleKeyboard(event);
        return;
    }
    const u8 key = event->argument;

    if (key == ASCII_ESCAPE) {
        editing = 0;
        release();
        mouseCtl->show();
        eventMask &= ~EVENT_KEY;
    } else if (key == '\r') {
        if (length != 0) {
            setSaveDescription(id.instance(), text.get());
            sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 0L, this, parent, 0L)));
            editing = 0;
            length = 0;
            release();
            mouseCtl->show();
            eventMask &= ~EVENT_KEY;
        }
    } else if (key == '\b') {
        if (length != 0) {
            length--;
        }
        text[length] = 0;
    } else if ((isalnum(key) != 0) || key == ' ' || key == '.' || key == '?' || key == '!' || key == ',' ||
               key == '/' || key == '"' || key == '\'') {
        if (length < 25 && event->argument < KEYCODE_SCAN) {
            text[length++] = toupper(filterKey(key));
            text[length] = '_';
            CAFont font(0xe);
            const i16 width = font.textWidth(text.get());

            text[length] = 0;
            if (static_cast<i16>(bounds.width() - 5) < width) {
                length--;
                text[length] = 0;
            }
        }
    }
    paint(0);
}

void SlotGump::handleLeftButton(NewGumpEvent *event)
{
    if (editing == 0 && event->isMousePress()) {
        NewGump *previousSlot = parent->children.findByInstanceId(selectedSlot);

        selectedSlot = id.instance();
        previousSlot->paint(0);
        paint(0);
        activate();
    }
}

void SlotGump::paintSelf(i16 x, i16 y)
{
    if (LOAD_SAVE_PICTURE) {
        Rect area(&bounds);

        area.moveTo(x, y);
        drawViewport->blit(gameBackground, area.left, area.top, area.right, area.bottom, area.left, area.top);
    } else {
        draw(x, y);
    }
    if (id.instance() == selectedSlot) {
        drawViewport->rectangle(SELECTED_SLOT_COLOUR, x, y, bounds.right - bounds.left + x,
                                bounds.bottom - bounds.top + y);
    } else if (LOAD_SAVE_PICTURE) {
        drawViewport->rectangle(0xfd, x, y, bounds.right - bounds.left + x, bounds.bottom - bounds.top + y);
    }
    if (editing != 0U) {
        CAFont font(0xe);

        text[length] = '_';
        font.draw(x + 4, y + 12, "%s", text.get());
        text[length] = 0;
    } else {
        CAFont font(0xe);

        font.draw(x + 4, y + 12, "%s", saveDescription(id.instance()));
    }
}

void SlotGump::activate()
{
    if (loading != 0U) {
        if (strlen(saveDescription(id.instance())) != 0U) {
            setSaveSlot(id.instance());
            sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 0L, this, parent, 0L)));
        }
    } else {
        editing = 1;
        memset(text.get(), 0, 0x20);
        strcpy(text.get(), saveDescription(id.instance()));
        length = strlen(text.get());
        eventMask |= EVENT_KEY;
        text[length] = '_';
        capture(this);
        mouseCtl->hide();
        paint(0);
    }
}

void SlotGump::select()
{
    NewGump *previousSlot = parent->children.findByInstanceId(selectedSlot);

    selectedSlot = id.instance();
    previousSlot->paint(0);
    paint(0);
}
