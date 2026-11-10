/* SYSTEM\JOYSTICK.ASM (12d0:0000-00fc) in C++: joystick A of the PC's game
 * port (201h), the platform's (input.h's inputJoystick: the port's PC has
 * none, the gamepad is an input of the bindings). The original starts the
 * port's four one-shots and polls it until each axis's bit drops, timing
 * each by the interval timer's counter: a one-shot lasts 24.2 us and 11 ns
 * an ohm of the stick's 0-100 kOhm, 29 to 1341 of the timer's counts.
 * Joystick B's two never answer (the port has none), nor A's without a
 * joystick; after 10000h polls, about a microsecond each, the reading gives
 * up on the axes left and drops them from joystickAxes for good. The time a
 * reading takes passes on the clock (the game's ticks may come in it, as in
 * the original, which reads with the interrupts on), and an axis whose time
 * crosses a tick keeps its last value: the counter the original subtracts
 * has started again. Two axes that end at once are timed at once (the
 * original's second poll is a microsecond on). */
#include "joystick.h"
#include "gamestate.h"
#include "clock.h"
#include "input.h"

u32 &timerPeriod = gameGlobal<u32>(0xffff);
u8 &joystickAxes = gameGlobal<u8>(0xf);
u32 &joystickFirstX = gameGlobal<u32>(0);
u32 &joystickFirstY = gameGlobal<u32>(0);
u8 &joystickButtons = gameGlobal<u8>(0);

namespace {

/* The axis times of the last reading. */
u32 &joystickX = gameGlobal<u32>(0);
u32 &joystickY = gameGlobal<u32>(0);

const u32 TIMER_HZ = 1193182;
const u32 NEVER = ~0U;
/* 10000h polls of the port, a microsecond each. */
const u32 TIMEOUT = static_cast<u32>(0x10000ULL * TIMER_HZ / 1000000);

/* The one-shot's time for the stick at value (-32768 at the left or the
 * top, 32767 at the right or the bottom), in the timer's counts. */
u32 countsOf(i16 value)
{
    const std::uint64_t ohms = static_cast<std::uint64_t>(value + 32768) * 100000 / 65535;
    return static_cast<u32>((24200 + 11 * ohms) * TIMER_HZ / 1000000000);
}

/* readJoystick (12d0:0048): the axes left in joystickAxes timed into
 * joystickX and joystickY, and the buttons. */
void readJoystick()
{
    i16 x = 0;
    i16 y = 0;
    u8 buttons = 0;
    const bool pad = inputJoystick(x, y, buttons);
    u8 left = joystickAxes;
    if (left != 0U) {
        const u32 start = clockTickPhase();
        u32 passed = 0;
        const u32 ends[2] = {pad ? countsOf(x) : NEVER, pad ? countsOf(y) : NEVER};
        for (;;) {
            /* The next of X and Y to end, X first when they end at once. */
            int axis = -1;
            for (int k = 0; k < 2; k++) {
                if (((left & 1 << k) != 0) && ends[k] != NEVER && (axis < 0 || ends[k] < ends[axis])) {
                    axis = k;
                }
            }
            if (axis < 0 || ends[axis] >= TIMEOUT) {
                break;
            }
            clockPassCounts(ends[axis] - passed);
            passed = ends[axis];
            const s32 time = static_cast<s32>(clockTickPhase() - start);
            if (time >= 0) {
                (axis == 0 ? joystickX : joystickY) = static_cast<u32>(time);
            }
            left &= static_cast<u8>(~(1 << axis));
            if (left == 0U) {
                break;
            }
        }
        if (left != 0U) {
            clockPassCounts(TIMEOUT - passed);
            joystickAxes ^= left;
        }
    }
    joystickButtons = pad ? static_cast<u8>(buttons & 0xf) : 0;
}

}

/* 12d0:00c3: two readings, the first kept apart; the shorter of each axis
 * stays in joystickX and joystickY. */
void sampleJoystick()
{
    readJoystick();
    joystickFirstX = joystickX;
    joystickFirstY = joystickY;
    readJoystick();
    if (joystickFirstX < joystickX) {
        joystickX = joystickFirstX;
    }
    if (joystickFirstY < joystickY) {
        joystickY = joystickFirstY;
    }
}
