/* The mouse of the port's PC (pc.h, mousedrv.h): the driver (INT 33h) and
 * the game's calls of it, the module at 11d0:0546, in the original's order,
 * which are the mouse of the platform's input (input.h).
 *
 * A mouse event waits for the mouse's line (12, interrupts.cpp): a place
 * (the scripted input's) or the mouse's motion in mickeys (the window's),
 * and the buttons. The line moves the cursor, by the mickeys at the speed
 * AX=0Fh set (mickeys per 8 pixels; the game's 3 across and 16 down), what
 * is left of a pixel kept for the next motion, inside its limits; sets the
 * buttons; and calls the handler the game installed when the event is in
 * its mask, with the events in AX, the buttons in BX and the place in CX,
 * DX: the game's is mousecb.asm's (kernel.cpp's mouseCallback), which
 * passes them on to KERNEL.C's mouseEvent unless mouseHeld is set. The
 * driver does not double fast motion (AX=13h), which the game does not ask
 * for. */
#include "input.h"
#include "gamememory.h"
#include "pcstate.h"
#include "intctl.h"
#include "mousedrv.h"
#include "passdump.h"
#include "pc.h"

#include <cstring>
#include <deque>

MouseDriver mouseDriver;

namespace {

/* The handler the driver calls and the events it wants (AX=0Ch, 14h). */
void (*handler)(u16 events, u16 buttons, u16 x, u16 y) = nullptr;
u16 handlerMask = 0;
/* The mickeys per eight pixels (AX=0Fh), which the scripted input's places
 * do not use, and the mickeys times 8 not yet a pixel. */
u16 speedX = 8, speedY = 16;
s32 leftX = 0, leftY = 0;

struct Pending {
    bool motion;
    i16 x, y;
    u16 buttons;
};
std::deque<Pending> pending;

/* The pixels of mickeys at speed, the rest kept in left. */
i16 pixels(s32 &left, i16 mickeys, u16 speed)
{
    left += mickeys * 8;
    const s32 whole = left / speed;
    left -= whole * speed;
    return static_cast<i16>(whole);
}

i16 limit(i16 value, i16 low, i16 high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

void keepInside()
{
    mouseDriver.x = limit(mouseDriver.x, mouseDriver.left, mouseDriver.right);
    mouseDriver.y = limit(mouseDriver.y, mouseDriver.top, mouseDriver.bottom);
}

/* AX=14h: the new handler and mask for the old ones. */
void swapHandler(void (**function)(u16, u16, u16, u16), u16 *mask)
{
    auto oldFunction = handler;
    const u16 oldMask = handlerMask;
    handler = *function;
    handlerMask = *mask;
    *function = oldFunction;
    *mask = oldMask;
}

u8 mousePresent = 0;
u16 oldMouseMask = 0;
void (*oldMouseHandler)(u16, u16, u16, u16) = nullptr;

}

/* AX=0: the cursor in the middle of the virtual screen of 640x200, hidden,
 * no buttons and no handler. */
void resetMouseDriver()
{
    mouseDriver = {320, 100, 0, 639, 0, 199, 0};
    handler = nullptr;
    handlerMask = 0;
    speedX = 8;
    speedY = 16;
    leftX = leftY = 0;
}

void mouseInput(i16 x, i16 y, u16 buttons)
{
    pending.push_back({false, x, y, buttons});
    requestIrq(12);
}

void mouseMotion(i16 mickeysX, i16 mickeysY, u16 buttons)
{
    pending.push_back({true, mickeysX, mickeysY, buttons});
    requestIrq(12);
}

/* The mouse's line: one event, and the line's end. The events' bits (the
 * driver's AX): moved, then each of the left, right and middle buttons down
 * and up. */
void mouseLine()
{
    if (!pending.empty()) {
        const Pending event = pending.front();
        pending.pop_front();
        const i16 x = mouseDriver.x;
        const i16 y = mouseDriver.y;
        const u16 before = mouseDriver.buttons;
        if (event.motion) {
            mouseDriver.x = static_cast<i16>(mouseDriver.x + pixels(leftX, event.x, speedX));
            mouseDriver.y = static_cast<i16>(mouseDriver.y + pixels(leftY, event.y, speedY));
        } else {
            mouseDriver.x = event.x;
            mouseDriver.y = event.y;
        }
        keepInside();
        mouseDriver.buttons = event.buttons;
        u16 events = 0;
        if (mouseDriver.x != x || mouseDriver.y != y) {
            events |= 1;
        }
        for (int button = 0; button < 3; button++) {
            const u16 bit = static_cast<u16>(1 << button);
            if (((event.buttons & bit) != 0) && ((before & bit) == 0)) {
                events |= static_cast<u16>(2 << 2 * button);
            }
            if (((event.buttons & bit) == 0) && ((before & bit) != 0)) {
                events |= static_cast<u16>(4 << 2 * button);
            }
        }
        const bool handled = (handler != nullptr) && ((events & handlerMask) != 0);
        passDumpMouse(events, mouseDriver.buttons, mouseDriver.x, mouseDriver.y, handled);
        if (handled) {
            const GameSide gameSide;
            handler(events, mouseDriver.buttons, static_cast<u16>(mouseDriver.x), static_cast<u16>(mouseDriver.y));
        }
        if (!pending.empty()) {
            requestIrq(12);
        }
    }
    endInterrupt(12);
}

/* SYSTEM.C's reset of the driver (INT 33h AX=0). */
void inputMouseReset()
{
    const SystemSide systemSide;
    resetMouseDriver();
}

/* MOUSE.C's initMouse: the driver reset, the game's speed and handler. */
void inputMouseStart(void (*gameHandler)(u16 events, u16 buttons, u16 x, u16 y))
{
    const SystemSide systemSide;
    resetMouseDriver();
    mousePresent = 1;
    inputMousePlace(0, 0);
    speedX = 3;
    speedY = 0x10;
    oldMouseHandler = gameHandler;
    oldMouseMask = 0x3f;
    swapHandler(&oldMouseHandler, &oldMouseMask);
}

/* removeMouseHandler */
void inputMouseStop()
{
    const SystemSide systemSide;
    if (mousePresent != 0U) {
        swapHandler(&oldMouseHandler, &oldMouseMask);
        mousePresent = 0;
    }
}

/* AX=4 */
void inputMousePlace(i16 x, i16 y)
{
    const SystemSide systemSide;
    mouseDriver.x = x;
    mouseDriver.y = y;
    keepInside();
}

/* AX=7, AX=8: the driver takes the smaller as the low end. */
void inputMouseXRange(i16 low, i16 high)
{
    const SystemSide systemSide;
    mouseDriver.left = low < high ? low : high;
    mouseDriver.right = low < high ? high : low;
    keepInside();
}

void inputMouseYRange(i16 low, i16 high)
{
    const SystemSide systemSide;
    mouseDriver.top = low < high ? low : high;
    mouseDriver.bottom = low < high ? high : low;
    keepInside();
}

MouseState mouseState()
{
    MouseState state{mouseDriver,        handler,         handlerMask,  speedX, speedY, leftX, leftY,
                     mousePresent != 0U, oldMouseHandler, oldMouseMask, {}};
    for (const Pending &event : pending) {
        const auto *bytes = reinterpret_cast<const u8 *>(&event);
        state.pending.insert(state.pending.end(), bytes, bytes + sizeof event);
    }
    return state;
}

void putMouseState(const MouseState &state)
{
    mouseDriver = state.driver;
    handler = state.handler;
    handlerMask = state.handlerMask;
    speedX = state.speedX;
    speedY = state.speedY;
    leftX = state.leftX;
    leftY = state.leftY;
    mousePresent = state.present ? 1 : 0;
    oldMouseHandler = state.oldHandler;
    oldMouseMask = state.oldMask;
    pending.clear();
    for (std::size_t at = 0; at + sizeof(Pending) <= state.pending.size(); at += sizeof(Pending)) {
        Pending event{};
        std::memcpy(&event, state.pending.data() + at, sizeof event);
        pending.push_back(event);
    }
}
