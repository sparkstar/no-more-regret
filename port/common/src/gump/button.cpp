/* 1308:0000-0689, file name unknown: the abstract button and its input
 * policies, with the four implicit destructors and four vtables the compiler
 * emits. See button.h. */
#include "button.h"
#include "gamestate.h"
#include "systimer.h"

u16 &buttonState = gameGlobal<u16>(0);
void ButtonBaseGump::handleRightButton(NewGumpEvent *event)
{
    handleLeftButton(event);
}

void ButtonBaseGump::handleLeftButton(NewGumpEvent *event)
{
    u32 mode;
    if (event->isMousePress() && buttonState == 0) {
        if (enabled == 0U) {
            return;
        }
        capture();
        eventMask |= EVENT_MOTION;
        buttonState = BUTTON_HELD | BUTTON_INSIDE;
        if (event->kind == EVENT_RIGHT_BUTTON) {
            buttonState |= BUTTON_RIGHT;
        }
        showPressed();
        paint(0);
        return;
    }
    if (event->isMouseRelease() && buttonState != 0) {
        mode = ((buttonState & BUTTON_RIGHT) != 0) ? 2L : 1L;
        buttonState = 0;
        release();
        eventMask &= ~EVENT_MOTION;
        if (eventPointInBounds(event)) {
            sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, mode, this, parent, 0L)));
        }
    }
}

void ButtonBaseGump::handleMouseMotion(NewGumpEvent *event)
{
    if (eventPointInBounds(event)) {
        if ((~buttonState & BUTTON_INSIDE) != 0) {
            showPressed();
            paint(0);
            buttonState |= BUTTON_INSIDE;
            return;
        }
    } else {
        if ((buttonState & BUTTON_INSIDE) != 0) {
            showReleased();
            paint(0);
            buttonState &= ~BUTTON_INSIDE;
        }
    }
}

ButtonGump::ButtonGump(u8 code, NewGump *parentGump) : ButtonBaseGump(code, parentGump) {}

void ButtonGump::handleLeftButton(NewGumpEvent *event)
{
    ButtonBaseGump::handleLeftButton(event);
    if (event->isMouseRelease() && buttonState == 0) {
        showReleased();
        paint(0);
    }
}

/* UNCALLED: nothing calls it. */
CyclingButtonGump::CyclingButtonGump(u8 code, NewGump *parentGump) : ButtonBaseGump(code, parentGump) {}

AutoRepeatButtonGump::AutoRepeatButtonGump(u8 code, NewGump *parentGump) : ButtonGump(code, parentGump) {}

void AutoRepeatButtonGump::handleLeftButton(NewGumpEvent *event)
{
    u32 mode;
    if (event->isMousePress() && buttonState == 0) {
        if (enabled == 0U) {
            return;
        }
        deadline = systemTimer->ticks + static_cast<s32>(10L);
        capture();
        eventMask |= EVENT_EXECUTE | EVENT_MOTION;
        buttonState |= BUTTON_HELD | BUTTON_INSIDE;
        if (event->kind == EVENT_RIGHT_BUTTON) {
            buttonState |= BUTTON_RIGHT;
        }
        showPressed();
        paint(0);
        if ((buttonState & BUTTON_INSIDE) != 0) {
            mode = ((buttonState & BUTTON_RIGHT) != 0) ? 2L : 1L;
            sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, mode, this, parent, 0L)));
            return;
        }
    } else if (event->isMouseRelease() && buttonState != 0) {
        buttonState = 0;
        release();
        eventMask &= ~(EVENT_EXECUTE | EVENT_MOTION);
        showReleased();
        paint(0);
    }
}
void AutoRepeatButtonGump::handleMouseMotion(NewGumpEvent *event)
{
    ButtonBaseGump::handleMouseMotion(event);
    execute(event);
}
void AutoRepeatButtonGump::execute(NewGumpEvent * /*unused*/)
{
    u32 current;
    u32 mode;
    if ((buttonState & BUTTON_INSIDE) != 0) {
        current = systemTimer->ticks;
        if (deadline < current) {
            deadline = current + static_cast<s32>(10L);
            mode = ((buttonState & BUTTON_RIGHT) != 0) ? 2L : 1L;
            sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, mode, this, parent, 0L)));
        }
    }
}
