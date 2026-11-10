/* The order in which the port's events come: the clock's ticks (line 0,
 * timer.cpp), the keyboard's scan codes (line 1, keyboard.cpp) and the
 * mouse's events (line 12, mouse.cpp), by the rules of the PC's two interrupt
 * controllers, which the game's kernel was written for; and the clock's hold
 * (clock.h), the original's interrupt flag.
 *
 * A line's request waits until the hold is off, the line is on (unmasked)
 * and no line of its priority or higher is in service; then its handler
 * runs held, as the processor entered an interrupt gate, and the hold goes
 * after it. The line stays in service until an end, which the handlers
 * send. */
#include "clock.h"
#include "gamememory.h"
#include "pcstate.h"
#include "host.h"
#include "intctl.h"
#include "pc.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace {

/* The interrupt flag and what clockPushHold kept of it. */
bool interruptFlag = true;
std::vector<bool> savedFlags;

/* The masks as the original's BIOS left them: the ticks, the keys, the
 * cascade and the diskette on the first controller, the mouse on the
 * second. */
u8 masks[2] = {0xb8, 0xef};
bool requested[16];
bool inService[16];

/* A line in service blocks itself and the lines of lower priority. */
bool blocked(u8 irq)
{
    for (u8 line = 0; line <= irq; line++) {
        if (inService[line]) {
            return true;
        }
    }
    return irq >= 8 && inService[2];
}

}

void requestIrq(u8 irq)
{
    requested[irq] = true;
}

bool interruptsEnabled()
{
    return interruptFlag;
}

bool irqUnmasked(u8 irq)
{
    if (irq < 8) {
        return (masks[0] & 1 << irq) == 0;
    }
    return ((masks[0] & 4) == 0) && ((masks[1] & 1 << (irq - 8)) == 0);
}

void deliverInterrupts()
{
    for (;;) {
        if (!interruptFlag) {
            return;
        }
        int irq = -1;
        for (u8 line = 0; line < 16 && irq < 0; line++) {
            if (line != 2 && requested[line] && irqUnmasked(line) && !blocked(line)) {
                irq = line;
            }
        }
        if (irq < 0) {
            return;
        }
        requested[irq] = false;
        inService[irq] = true;
        if (irq >= 8) {
            inService[2] = true;
        }
        interruptFlag = false;
        if (irq == 0) {
            timerLine();
        } else if (irq == 1) {
            keyboardLine();
        } else if (irq == 12) {
            mouseLine();
        }
        interruptFlag = true;
    }
}

void clockHold()
{
    const SystemSide systemSide;
    interruptFlag = false;
}

void clockRelease()
{
    const SystemSide systemSide;
    interruptFlag = true;
}

void clockPushHold()
{
    const SystemSide systemSide;
    savedFlags.push_back(interruptFlag);
}

void clockPopHold()
{
    const SystemSide systemSide;
    if (savedFlags.empty()) {
        return;
    }
    interruptFlag = savedFlags.back();
    savedFlags.pop_back();
}

/* INT 3 without a debugger, and the other interrupts the game raises. */
void hostDebugBreak()
{
    const SystemSide systemSide;
}

void maskIrq(u8 irq)
{
    masks[irq >= 8] |= static_cast<u8>(1 << (irq & 7));
}

void unmaskIrq(u8 irq)
{
    masks[irq >= 8] &= static_cast<u8>(~(1 << (irq & 7)));
}

/* The non-specific end of interrupt, to the slave too for its lines: in
 * each controller the line of the highest priority in service leaves it. */
void endInterrupt(u8 irq)
{
    auto end = [](u8 controller) {
        for (u8 line = controller * 8; line < controller * 8 + 8; line++) {
            if (inService[line]) {
                inService[line] = false;
                return;
            }
        }
    };
    if (irq >= 8) {
        end(1);
    }
    end(0);
}

u16 irqMask()
{
    return static_cast<u16>(~(masks[1] << 8 | masks[0]) & 0xffff);
}

InterruptState interruptState()
{
    InterruptState state{interruptFlag, savedFlags, {masks[0], masks[1]}, {}, {}};
    std::copy(std::begin(requested), std::end(requested), state.requested.begin());
    std::copy(std::begin(inService), std::end(inService), state.inService.begin());
    return state;
}

void putInterruptState(const InterruptState &state)
{
    interruptFlag = state.interruptFlag;
    savedFlags = state.savedFlags;
    masks[0] = state.masks[0];
    masks[1] = state.masks[1];
    std::copy(state.requested.begin(), state.requested.end(), std::begin(requested));
    std::copy(state.inService.begin(), state.inService.end(), std::begin(inService));
}
