/* WEZZY.C (13e0:0000-1a3d, No Remorse only). See wezzy.h.
 *
 * The weasel greets the player with a movie, then shows the wares a level
 * of stuff.dat offers. Buy keeps a ware in purchases and takes its cost from
 * credits; Exit asks for the confirmation, and yes delivers the purchases,
 * no drops them and gives the credits back. Each answer has its movie. */
#include "gamestate.h"
#include <cstring>
#include "wezzy.h"
#include "basecam.h"
#include "cafont.h"
#include "camera.h"
#include "crusys.h"
#include "cycle.h"
#include "dispatch.h"
#include "dsfxman.h"
#include "dtable.h"
#include "file.h"
#include "filespec.h"
#include "flicplay.h"
#include "gamefile.h"
#include "gamepal.h"
#include "gumpshap.h"
#include "item.h"
#include "npc.h"
#include "options.h"
#include "movie.h"
#include "kprocess.h"
#include "random.h"
#include "shapebtn.h"
#include "type.h"
#include "u9mouse.h"
#include "viewport.h"
#include "world.h"
#include "sounds.h"

namespace {
void setOtherGreeting();
void clearOtherGreeting();
u8 otherGreetingSet();
i16 pickupShape(u16 shape);

/* The weasel greets with one of 18d-18f when it is set; nothing sets it
 * (1478:5ee2). */
u8 &otherGreeting = gameGlobal<u8>(0);
/* The movies of the weasel (static\flics). */
const char *const FIRST_GREETING[] = {"17a"};
const char *const GREETINGS[] = {"18a", "18b", "18c"};
const char *const OTHER_GREETINGS[] = {"18d", "18e", "18f"};
const char *const LEAVING_MOVIES[] = {"19c", "19d"};
const char *const CONFIRM_MOVIES[] = {"21a", "21b"};
const char *const CANCELLED_MOVIES[] = {"21e", "21f"};
const char *const BOUGHT_MOVIES[] = {"21c", "21d"};
[[maybe_unused]] const char *const UNUSED_MOVIES[] = {"20a", "20b"};
const char *const POOR_MOVIES[] = {"20c", "20d"};
}
u8 &weaselVisited = gameGlobal<u8>(0);

void showWeaselShop(u16 level)
{
    /* After the shop has closed, the intrinsic's result 0. */
    openModalGump(new WeaselGump(level), static_cast<u32>(-1), [](s32 /*result*/) {
        gameCamera->repaint();
        /* Sets otherGreeting to itself (the original's). */
        if (otherGreetingSet() != 0U) {
            setOtherGreeting();
        } else {
            clearOtherGreeting();
        }
        setRestResult(0);
    });
}

namespace {
void setOtherGreeting()
{
    otherGreeting = 1;
}

void clearOtherGreeting()
{
    otherGreeting = 0;
}

u8 otherGreetingSet()
{
    return otherGreeting;
}
}

/* The cheats show the wares of level 0. */
WeaselGump::WeaselGump(u16 newLevel) : NewGump(NewGumpId(0), nullptr, 0)
{
    credits = controlledNpc.credits();
    pending = 0;
    delivered = 0;
    state = WEASEL_GREETING;
    savedLevel = -1;
    if (cheatsEnabled != 0U) {
        newLevel = 0;
    }
    level = newLevel;
    current = 0;
    unknown140 = 0xffff;
    unknown13e = 0xffff;
    unknown38 = 0;
    memset(purchases, 0, sizeof purchases);
    purchaseCount = 0;
    eventMask |= EVENT_EXECUTE | EVENT_KEY;
    bounds.set(0, 0, 210, 437);
    bounds.moveTo(static_cast<i16>(drawViewport->right() - drawViewport->left() + 1 - RECT_WIDTH(bounds)) >> 1,
                  (static_cast<i16>(drawViewport->bottom() - drawViewport->top() + 1 - RECT_HEIGHT(bounds)) >> 1) - 15);
    ShapeButtonGump *button = new ShapeButtonGump(WEASEL_YES, this, 0x10);

    button->bounds.moveTo(17, 237);
    button = new ShapeButtonGump(WEASEL_NO, this, 0xf);
    button->bounds.moveTo(114, 237);
    button = new ShapeButtonGump(WEASEL_BUY, this, 0x1c);
    button->bounds.moveTo(20, 280);
    button = new ShapeButtonGump(WEASEL_PREVIOUS, this, 0xd);
    button->bounds.moveTo(15, 212);
    button = new ShapeButtonGump(WEASEL_NEXT, this, 0xe);
    button->bounds.moveTo(137, 212);
    button = new ShapeButtonGump(WEASEL_BLANK, this, 0x1a);
    button->bounds.moveTo(76, 212);
    button = new ShapeButtonGump(WEASEL_AMMO, this, 0x1b);
    button->bounds.moveTo(19, 320);
    button = new ShapeButtonGump(WEASEL_WEAPONS, this, 0x53);
    button->bounds.moveTo(19, 320);
    button->hide();
    button = new ShapeButtonGump(WEASEL_EXIT, this, 0x1d);
    button->bounds.moveTo(45, 367);
    loadCatalog(level);
    showQuestion();
    step();
    addAcceptedArguments(-1, 0);
    freezeWorld();
    hide();
}

WeaselGump::~WeaselGump()
{
    thawWorld();
    unknown0c75();
}

void WeaselGump::showQuestion()
{
    NewGump *button;

    button = children.findByInstanceId(WEASEL_BUY);
    button->hide();
    button = children.findByInstanceId(WEASEL_EXIT);
    button->hide();
    button = children.findByInstanceId(WEASEL_PREVIOUS);
    button->hide();
    button = children.findByInstanceId(WEASEL_NEXT);
    button->hide();
    button = children.findByInstanceId(WEASEL_BLANK);
    button->hide();
    button = children.findByInstanceId(WEASEL_AMMO);
    button->hide();
    button = children.findByInstanceId(WEASEL_WEAPONS);
    button->hide();
    button = children.findByInstanceId(WEASEL_YES);
    button->paintState >>= 2;
    button = children.findByInstanceId(WEASEL_NO);
    button->paintState >>= 2;
    paint(0);
}

/* WEASEL_WEAPONS stays hidden, even while the ammunition is shown (the
 * original's). */
void WeaselGump::showBrowsing()
{
    NewGump *button;

    button = children.findByInstanceId(WEASEL_YES);
    button->hide();
    button = children.findByInstanceId(WEASEL_NO);
    button->hide();
    button = children.findByInstanceId(WEASEL_BUY);
    button->paintState >>= 2;
    button = children.findByInstanceId(WEASEL_EXIT);
    button->paintState >>= 2;
    button = children.findByInstanceId(WEASEL_PREVIOUS);
    button->paintState >>= 2;
    button = children.findByInstanceId(WEASEL_NEXT);
    button->paintState >>= 2;
    button = children.findByInstanceId(WEASEL_BLANK);
    button->paintState >>= 2;
    button = children.findByInstanceId(WEASEL_AMMO);
    button->paintState >>= 2;
    paint(0);
    pending = 0;
}

/* The frame is four gump shapes, 16h-19h. The question, or the ware with its
 * name, cost and counts, is written in font 6. */
void WeaselGump::paintSelf(i16 x, i16 y)
{
    const GumpShape top(0x16);
    const GumpShape upper(0x17);
    const GumpShape lower(0x18);
    const GumpShape bottom(0x19);

    top.draw(x + 3, y);
    upper.draw(x, y + 71);
    lower.draw(x + 5, y + 204);
    bottom.draw(x + 9, y + 362);
    CAFont font(6);

    switch (state) {
    case WEASEL_CONFIRM:
        font.draw(x + 25, y + 90, "Are you sure you");
        font.draw(x + 25, y + 98, "want to buy this?");
        break;
    case WEASEL_BROWSING: {
        const char *name = pickupName(items[current].shape);
        const u16 shape = items[current].shape;
        const i16 icon = pickupShape(shape);

        if ((name != nullptr) && (icon != 0)) {
            GumpShape image(icon);

            image.setFrame(pickupFrame(shape));
            image.draw(bounds.left + 108, bounds.top + 131);
            font.draw(bounds.left + 25, bounds.top + 163, "%s", name);
            font.draw(bounds.left + 25, bounds.top + 173, "Cost:%u", items[current].cost);
            font.draw(bounds.left + 25, bounds.top + 183, "Purchased:%02u", purchasedCount(shape));
            font.draw(bounds.left + 25, bounds.top + 193, "Owned:%02u", ownedCount(shape));
        } else {
            halt();
        }
        font.draw(x + 29, y + 56, "Credits:%d", credits);
        break;
    }
    case WEASEL_ANYTHING_ELSE:
        font.draw(x + 25, y + 90, "Do you want");
        font.draw(x + 25, y + 98, "anything else?");
        break;
    }
}

/* The clips of the GL-303 are sold only in the ammunition of level 5 on,
 * the AR-7's of level 6 on; a level's own wares (savedLevel -1) never have
 * them. */
void WeaselGump::loadCatalog(u16 newLevel)
{
    i16 i;

    level = newLevel;
    memset(items, 0, sizeof items);
    File file(makePath(nullptr, staticDirectory, "stuff.dat", nullptr), 0, 0);

    file.seek(static_cast<ui16>(level * 0x142), 0);
    file.read(&count, 2);
    file.read(items, static_cast<ui16>(count * sizeof(WeaselItem)));
    file.close();
    if (savedLevel < 5) {
        for (i = 0; i < count; i++) {
            if (items[i].shape == SHAPE_GL_303_CLIP) {
                items[i].shape = 0;
            }
        }
    }
    if (savedLevel < 6) {
        for (i = 0; i < count; i++) {
            if (items[i].shape == SHAPE_AR_7_CLIP) {
                items[i].shape = 0;
            }
        }
    }
    for (current = 0; items[current].shape == 0U; current++) {
        ;
    }
}

void WeaselGump::unknown0c70() {}

void WeaselGump::unknown0c75() {}

void WeaselGump::clampItem()
{
    paint(0);
    if (current >= count) {
        current = count - 1;
    }
    if (current < 0) {
        current = 0;
    }
}

/* Not with the noweasel option. The world is stopped and the keys go to the
 * player; the camera is redrawn after it. */
void WeaselGump::playMovie(const char *const *names, i16 movieCount)
{
    if (noWeasel != 0U) {
        return;
    }
    unknown0e51();
    gameMouse->hide();
    dispatcher->eventMask &= ~EVENT_KEYBOARD;
    char *name = new char[0x80];

    strcpy(name, names[randomNumber(movieCount)]);
    /* The shop after the movie. */
    openMovie(MovieRequest{.name = name,
                           .centred = false,
                           .x = static_cast<i16>(flicVideoMode == MODE_13H ? 80 : 200),
                           .y = static_cast<i16>(bounds.top + 54),
                           .background = MovieBackground::GREY,
                           .holdMusic = true},
              [name](bool /*cancelled*/) {
                  delete[] name;
                  gamePalette->apply();
                  gameCamera->repaint();
                  addDirtyRect(drawViewport->bounds());
                  cycleProcess->paused = 0;
                  dispatcher->eventMask |= EVENT_KEYBOARD;
                  gameMouse->show();
              });
}

void WeaselGump::unknown0e51() {}

/* UNCALLED: nothing calls it. */
void WeaselGump::unknown0e56() {}

void WeaselGump::step()
{
    sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 0L, this, this, 0L)));
}

void WeaselGump::handleCommand(NewGumpEvent *event)
{
    if (event->sender == this) {
        switch (state) {
        /* Each state's after its movie. */
        case WEASEL_GREETING: {
            paint(1);
            auto greeted = [this]() {
                state = WEASEL_BROWSING;
                paintState >>= 2;
                showBrowsing();
                clampItem();
                paint(0);
            };
            if (otherGreeting != 0U) {
                playMovie(OTHER_GREETINGS, 3);
            } else if (weaselVisited != 0U) {
                playMovie(GREETINGS, 3);
            } else {
                playMovie(FIRST_GREETING, 1);
                whenDone([greeted]() {
                    weaselVisited = 1;
                    greeted();
                });
                break;
            }
            whenDone(greeted);
            break;
        }
        case WEASEL_CLOSING:
            close();
            break;
        case WEASEL_LEAVING:
            playMovie(LEAVING_MOVIES, 2);
            whenDone([this]() {
                state = WEASEL_ANYTHING_ELSE;
                showQuestion();
            });
            break;
        case WEASEL_CONFIRM:
            playMovie(CONFIRM_MOVIES, 2);
            whenDone([this]() { showQuestion(); });
            break;
        case WEASEL_CANCELLED:
            playMovie(CANCELLED_MOVIES, 2);
            whenDone([this]() {
                state = WEASEL_ANYTHING_ELSE;
                showQuestion();
            });
            break;
        case WEASEL_BOUGHT:
            playMovie(BOUGHT_MOVIES, 2);
            whenDone([this]() {
                state = WEASEL_ANYTHING_ELSE;
                showQuestion();
            });
            break;
        case WEASEL_POOR:
            playMovie(POOR_MOVIES, 2);
            whenDone([this]() {
                state = WEASEL_BROWSING;
                clampItem();
            });
            break;
        case WEASEL_BROWSING:
            break;
        }
    } else {
        const u8 button = event->sender->id.instance();

        choose(button);
    }
}

void WeaselGump::execute(NewGumpEvent * /*unused*/) {}

void WeaselGump::choose(i16 button)
{
    NewGump *other;

    switch (button) {
    case WEASEL_YES:
        if (state == WEASEL_ANYTHING_ELSE) {
            state = WEASEL_BROWSING;
            showBrowsing();
            clampItem();
        } else if (state == WEASEL_CONFIRM) {
            deliver();
            delivered = 1;
            state = WEASEL_BOUGHT;
            step();
        }
        break;
    case WEASEL_NO:
        if (state == WEASEL_ANYTHING_ELSE) {
            if (delivered != 0U) {
                state = WEASEL_CLOSING;
                step();
            } else {
                close();
            }
        } else if (state == WEASEL_CONFIRM) {
            credits = controlledNpc.credits();
            memset(purchases, 0, sizeof purchases);
            purchaseCount = 0;
            state = WEASEL_CANCELLED;
            step();
        }
        break;
    case WEASEL_BUY:
        buy();
        break;
    case WEASEL_EXIT:
        if (savedLevel >= 0) {
            level = savedLevel;
            loadCatalog(level);
            savedLevel = -1;
        }
        unknown0c70();
        if (pending != 0U) {
            state = WEASEL_CONFIRM;
            step();
        } else {
            credits = controlledNpc.credits();
            state = WEASEL_LEAVING;
            step();
        }
        break;
    case WEASEL_PREVIOUS:
        previous();
        paint(0);
        break;
    case WEASEL_NEXT:
        next();
        paint(0);
        break;
    case WEASEL_WEAPONS:
        if (savedLevel >= 0) {
            other = children.findByInstanceId(WEASEL_AMMO);
            other->paintState >>= 2;
            other = children.findByInstanceId(WEASEL_WEAPONS);
            other->hide();
            level = savedLevel;
            loadCatalog(level);
            savedLevel = -1;
            paint(0);
        }
        break;
    case WEASEL_AMMO:
        if (savedLevel < 0) {
            other = children.findByInstanceId(WEASEL_WEAPONS);
            other->paintState >>= 2;
            other = children.findByInstanceId(WEASEL_AMMO);
            other->hide();
            savedLevel = level;
            loadCatalog(1);
        }
        paint(0);
        break;
    }
}

/* Escape leaves or answers no; the arrows step through the wares, Enter and
 * space buy, Tab switches between the ammunition and the level's wares. Y,
 * J and O answer yes, N no. */
void WeaselGump::handleKeyboard(NewGumpEvent *event)
{
    if (event->argument == 0x1b) {
        if (state == WEASEL_BROWSING) {
            choose(WEASEL_EXIT);
        } else if (state == WEASEL_CONFIRM || state == WEASEL_ANYTHING_ELSE) {
            choose(WEASEL_NO);
        } else {
            close();
        }
        return;
    }
    switch (event->argument) {
    case KEYCODE_SCAN | SCAN_LEFT:
        if (state == WEASEL_BROWSING) {
            choose(WEASEL_PREVIOUS);
        }
        break;
    case KEYCODE_SCAN | SCAN_RIGHT:
        if (state == WEASEL_BROWSING) {
            choose(WEASEL_NEXT);
        }
        break;
    case '\r':
    case ' ':
        if (state == WEASEL_BROWSING) {
            choose(WEASEL_BUY);
        }
        break;
    case '\t':
        if (state == WEASEL_BROWSING) {
            if (level == 1) {
                choose(WEASEL_WEAPONS);
            } else {
                choose(WEASEL_AMMO);
            }
        }
        break;
    case 'Y':
    case 'y':
    case 'J':
    case 'j':
    case 'O':
    case 'o':
        if (state == WEASEL_ANYTHING_ELSE || state == WEASEL_CONFIRM || state == WEASEL_CANCELLED ||
            state == WEASEL_BOUGHT) {
            choose(WEASEL_YES);
        }
        break;
    case 'N':
    case 'n':
        if (state == WEASEL_ANYTHING_ELSE || state == WEASEL_CONFIRM || state == WEASEL_CANCELLED ||
            state == WEASEL_BOUGHT) {
            choose(WEASEL_NO);
        }
        break;
    }
}

/* UNCALLED: nothing calls it. */
bool WeaselGump::isPurchased()
{
    const u16 shape = items[current].shape;

    for (i16 i = 0; i < WEASEL_PURCHASES; i++) {
        if (purchases[i] == shape) {
            return true;
        }
    }
    return false;
}

i16 WeaselGump::purchasedCount(u16 shape)
{
    i16 purchased = 0;

    for (i16 i = 0; i < purchaseCount; i++) {
        if (purchases[i] == shape) {
            purchased++;
        }
    }
    return purchased;
}

/* 1 or 0 of a weapon, the quality of another item. */
i16 WeaselGump::ownedCount(u16 shape)
{
    const i16 family = typeFlags.infos[shape].family;

    if (family == FAMILY_WEAPON) {
        return static_cast<i16>(controlledNpc.findItem(shape) != 0);
    }
    const Item item = controlledNpc.findItem(shape);

    if (item.isValid()) {
        return item.quality();
    }
    return 0;
}

bool WeaselGump::canBuy(u16 shape)
{
    i16 limit = 10;
    i16 held = 0;

    for (i16 i = 0; i < purchaseCount; i++) {
        if (purchases[i] == shape) {
            held++;
        }
    }
    const Item item = controlledNpc.findItem(shape);

    if (item.isValid()) {
        if (typeFlags.infos[shape].family != FAMILY_WEAPON) {
            held += item.quality();
        } else {
            held++;
        }
    }
    if (typeFlags.infos[shape].family == FAMILY_AMMO) {
        limit = 20;
    } else if (typeFlags.infos[shape].family == FAMILY_WEAPON) {
        limit = 1;
    }
    if (shape == SHAPE_SPIDER_BOMB) {
        limit = 20;
    }
    if (held >= limit) {
        return false;
    }
    return true;
}

/* The ware is made and tried on the NPC with canPickUp, then destroyed;
 * sound B0h says no. */
void WeaselGump::buy()
{
    const s32 cost = items[current].cost;

    if (credits - cost >= 0) {
        Item item;

        item.create(items[current].shape, 0);
        item.popToCoords(0, 0, 0);
        if (canPickUp(item.id)) {
            if (canBuy(items[current].shape)) {
                pending = 1;
                credits -= cost;
                purchases[purchaseCount++] = items[current].shape;
                paint(0);
            } else {
                playEffect(SOUND_WEASEL_REFUSES);
            }
        } else {
            playEffect(SOUND_WEASEL_REFUSES);
        }
        item.destroy();
    } else {
        playEffect(SOUND_WEASEL_REFUSES);
    }
}

/* The NPC's card keeps the low word of credits (the original's). */
void WeaselGump::deliver()
{
    for (i16 i = 0; i < WEASEL_PURCHASES; i++) {
        if (purchases[i] != 0U) {
            Item item;

            item.create(purchases[i], 0);
            if (item.isValid()) {
                item.popToCoords(0, 0, 0);
                pickUp(item.id, 0);
            }
        }
    }
    const Item card = controlledNpc.findItem(SHAPE_CREDITS);

    if (card.isValid()) {
        card.setFamilyQuality(credits);
    }
    memset(purchases, 0, sizeof purchases);
    purchaseCount = 0;
}

void WeaselGump::next()
{
    do {
        if (current < static_cast<i16>(count - 1)) {
            current++;
        } else {
            current = 0;
        }
    } while (items[current].shape == 0U);
    clampItem();
}

void WeaselGump::previous()
{
    do {
        if (current != 0) {
            current--;
        } else {
            current = count - 1;
        }
    } while (items[current].shape == 0U);
    clampItem();
}

namespace {
/* The gump shape that holds the frame pickupFrame gives shape: 3 for the
 * weapons and the credits, 5 for the ammunition, the inventory items and
 * the keycard, 4 for the bombs, 0 for the rest. */
i16 pickupShape(u16 shape)
{
    const ShapeInfo info = typeFlags.info(shape);
    i16 result;

    if (shape == SHAPE_KEYCARD) {
        result = 5;
    } else if (shape == SHAPE_CREDITS) {
        result = 3;
    } else if (info.family == FAMILY_WEAPON) {
        result = 3;
    } else if (info.family == FAMILY_AMMO) {
        result = 5;
    } else if (info.family == FAMILY_BOMB) {
        result = 4;
    } else if (info.family == FAMILY_INVENTORY) {
        result = 5;
    } else {
        result = 0;
    }
    return result;
}
}
