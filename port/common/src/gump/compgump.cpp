/* Code115 (1390:0000-0aa2, No Regret's addresses; file name unknown). See
 * compgump.h. The waits of the typing are the game's (compgump_game.h). */
#include "gamestate.h"
#include <cstring>
#include "compgump.h"
#include "compgump_game.h"
#include "language.h"
#include "termgump.h"
#include "basecam.h"
#include "cafont.h"
#include "camera.h"
#include "crusys.h"
#include "dispatch.h"
#include "dsfxman.h"
#include "gametime.h"
#include "gumpshap.h"
#include "joydrv.h"
#include "kernel.h"
#include "mousectl.h"
#include "u9mouse.h"
#include "world.h"
#include "systimer.h"
#include "input.h"
#include "sounds.h"

/* The first page is being typed: draw the frame. */
u8 &typingPage = gameGlobal<u8>(0);

/* Not called. */
/* UNCALLED: nothing calls it. */
void ComputerGump::unused0000() {}

void showComputerText(const char *text)
{
    freezeWorld();
    /* The world goes on after the gump has closed, the intrinsic's result
     * 0. */
    auto closed = [](s32 /*result*/) {
        thawWorld();
        setRestResult(0);
    };
    if (gameLanguage == Language::ENGLISH) {
        openModalGump(new ComputerGump(text), static_cast<u32>(-1), closed);
    } else {
        /* The other languages' layout. */
        openModalGump(new TerminalGump(text), static_cast<u32>(-1), closed);
    }
}

namespace {
/* The waits of the computer's text after a character and after a '*'
 * (compgump_game.h). */
void computerCharacterWait()
{
    if (CHARACTER_WAITS_A_TICK) {
        /* A wait of the steps, the last of the gump's execute: nothing
         * comes after it. */
        openTimerDelay(1, []() { return false; });
    } else {
        openClockDelay(CHARACTER_WAIT, []() { return false; });
    }
}

void computerPauseWait()
{
    openClockDelay(PAUSE_WAIT, []() { return false; });
}
}

/* ComputerGump::execute's waits, for TerminalGump (termgump.h). */
void terminalCharacterWait()
{
    computerCharacterWait();
}

void terminalPauseWait()
{
    computerPauseWait();
}

ComputerGump::ComputerGump(const char *source) : NewGump(NewGumpId(), nullptr, 0)
{
    savedFlag = animationsHeld;
    animationsHeld = 1;
    setText(source);
    length = 0;
    unused3a = 0;
    column = row = 0;
    pageStart = 0;
    pageFull = 0;
    typingPage = 1;
    fast = 0;
    if (joystickReady != 0U) {
        do {
            pollJoystick();
        } while ((joystickState & JOYSTICK_BUTTONS) != 0);
    }
    eventMask |= EVENT_EXECUTE | EVENT_LEFT_BUTTON | EVENT_KEY;
    addAcceptedArguments(-1, 0);
    GumpShape image(0x1e);
    bounds = image.dimensions();
    image.frame = 1;
    bounds.bottom += image.height();
    bounds.moveTo(static_cast<i16>(drawViewport->right() - drawViewport->left() + 1 - RECT_WIDTH(bounds)) >> 1,
                  static_cast<i16>(drawViewport->bottom() - drawViewport->top() + 1 - RECT_HEIGHT(bounds)) >> 1);
    gameMouse->hide();
    inputMousePlace(0, 0);
    worldActive = 0;
    paint(0);
}

ComputerGump::~ComputerGump()
{
    animationsHeld = savedFlag;
    worldActive = 1;
    gameMouse->show();
    gameCamera->repaint();
    gameCamera->run();
    flushKeys();
    gumpKeys.clear();
}

void ComputerGump::paintSelf(i16 x, i16 y)
{
    GumpShape image(0x1e);

    if (typingPage != 0U) {
        image.draw(x, y);
        image.frame = 1;
        image.draw(x, y + 160);
    }
    if (unused3a == 0U) {
        typeText(x, y);
        if (pageFull != 0U) {
            fast = 0;
            CAFont font(6);
            font.draw(x + 41, (row * 9) + y + 38, "<MORE>");
            return;
        }
        typingPage = 0;
    }
}

void ComputerGump::setText(const char *source)
{
    u16 i;
    i16 space;
    i16 run;

    text = std::make_unique_for_overwrite<char[]>(strlen(source) + 1);
    strcpy(text.get(), source);
    i = 0;
    space = -1;
    run = 0;
    while (text[i] != 0) {
        if (text[i] == ' ') {
            space = i;
        }
        if (text[i] == '^') {
            run = 0;
            space = -1;
        }
        if (run >= TEXT_COLUMNS && space >= 0) {
            text[space] = '^';
            run = i - space;
            space = -1;
        }
        if (text[i] != 1) {
            run++;
        }
        i++;
    }
}

void ComputerGump::typeText(s16 x, s16 y)
{
    i16 i;

    column = 0;
    row = 0;
    CAFont font(6);
    for (i = pageStart; length > i; i++) {
        if (text[i] == '^') {
            if ((fast == 0U) && (typingPage == 0U)) {
                Rect area(0, 0, 0, 0);
                area.right = font.glyphWidth('[') - 1;
                area.bottom = font.glyphHeight('[') - 1;
                area.moveTo((column * 8) + x + 41, (row * 9) + y + 38);
                drawViewport->clear(0, area.left, area.top, area.right, area.bottom);
                getClipRect();
                addDirtyRect(area);
            }
            column = 0;
            row++;
        } else if (text[i] != 1 && text[i] != '*') {
            if ((fast == 0U) && ((typingPage != 0U) || static_cast<i16>(length - 1) == i)) {
                Rect area(0, 0, 0, 0);
                if (typingPage == 0U) {
                    area.right = font.glyphWidth(text[i]) - 1;
                    area.bottom = font.glyphHeight(text[i]) - 1;
                    area.moveTo((column * 8) + x + 41, (row * 9) + y + 38);
                    drawViewport->clear(0, area.left, area.top, area.right, area.bottom);
                    getClipRect();
                    addDirtyRect(area);
                }
                font.putChar((column * 8) + x + 41, (row * 9) + y + 38, text[i]);
            }
            column++;
        }
        if (column >= TEXT_COLUMNS) {
            column = 0;
            row++;
        }
        if (row == TEXT_ROWS) {
            pageFull = 1;
            typingPage = 1;
        }
    }
    if ((pageFull == 0U) && (text[length] != 0)) {
        length++;
        if (fast == 0U) {
            font.putChar((column * 8) + x + 41, (row * 9) + y + 38, '[');
            if (typingPage == 0U) {
                Rect area(0, 0, 0, 0);
                area.right = font.glyphWidth('[') - 1;
                area.bottom = font.glyphHeight('[') - 1;
                area.moveTo((column * 8) + x + 41, (row * 9) + y + 38);
                addDirtyRect(area);
            }
        }
    }
}

void ComputerGump::handleLeftButton(NewGumpEvent *event)
{
    if (event->isMousePress()) {
        if (text[length] != 0) {
            fast = 1;
            return;
        }
        close();
    }
}

void ComputerGump::execute(NewGumpEvent *event)
{
    if (joystickReady != 0U) {
        pollJoystick();
        if ((joystickState & JOYSTICK_BUTTONS) != 0) {
            handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, ASCII_ESCAPE)));
            return;
        }
    }
    if (text[length] != 0) {
        if (fast != 0U) {
            while ((pageFull == 0U) && (text[length] != 0)) {
                typeText(event->x, event->y);
            }
            fast = 0;
            typingPage = 1;
        }
        paint(1);
        if (pageFull == 0U) {
            if (text[static_cast<i16>(length - 1)] == '*') {
                computerPauseWait();
                return;
            }
            if (text[static_cast<i16>(length - 1)] != '\n') {
                stopEffect(SOUND_TERMINAL_TYPING);
                playEffect(SOUND_TERMINAL_TYPING);
            }
            computerCharacterWait();
            return;
        }
    } else {
        paint(1);
    }
}

void ComputerGump::handleKeyboard(NewGumpEvent *event)
{
    if (pageFull != 0U) {
        pageFull = 0;
        column = 0;
        row = 0;
        pageStart = length;
        return;
    }
    if (event->argument != ASCII_ESCAPE && event->argument != '\r') {
        return;
    }
    if (text[length] != 0) {
        if (event->argument != ASCII_ESCAPE) {
            return;
        }
        fast = 1;
        return;
    }
    close();
}
